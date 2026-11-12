#include "board.h"
#include "controls.h"
#include "synth.h"
#include "util/oled_fonts.h"
#include <atomic>
#include <cmath>
#include <cstdio>

using namespace daisy;
using namespace vamp;

// User configuration. Pins are Daisy D numbers, not 40-pin header numbers.
constexpr bool kEnableOled = true;
constexpr uint8_t kOledAddress = 0x3C; // Change to 0x3D if the module requires it.
constexpr int kMidiChannel = 0;       // MIDI channel 1 on a keyboard.
constexpr int kEncoderDirection[4] = {1, 1, 1, 1}; // Set -1 to reverse a knob.

static_assert(ATOMIC_INT_LOCK_FREE == 2, "Audio communication must be lock-free");
static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "Panic flag must be lock-free");
DaisySeed hw;
Expanders expanders;
Display display;
MidiUartHandler midi;
Synth synth;
NoteQueue<128> note_queue;
std::atomic<bool> panic_requested{false};
std::atomic<int> sounding_note{-1};

// Only scalar atomic integers cross between foreground and the audio interrupt.
// DSP objects and HeldNotes belong exclusively to the audio callback after Init.
struct SharedSettings
{
    std::atomic<int> cutoff{55}, resonance{15}, release_ms{250}, volume{20};
    std::atomic<int> waveform{0}, bend_cents{0}, filter_cents{0};
} shared;

Button keys[16], encoder_push[4], joystick_push;
Quadrature encoders[4];
int octave_shift = 0;
float center_x = 0.5f, center_y = 0.5f;
float smooth_x = 0, smooth_y = 0;
bool controls_ok = false;

void QueueNote(NoteEvent event)
{
    if(!note_queue.Push(event)) panic_requested.store(true);
}

SoundSettings ReadSettings()
{
    SoundSettings s;
    s.cutoff = shared.cutoff.load();
    s.resonance = shared.resonance.load();
    s.release_ms = shared.release_ms.load();
    s.volume = shared.volume.load();
    s.waveform = shared.waveform.load();
    s.bend_cents = shared.bend_cents.load();
    s.filter_cents = shared.filter_cents.load();
    return s;
}

void AudioCallback(AudioHandle::InputBuffer /*in*/,
                   AudioHandle::OutputBuffer out, size_t size)
{
    NoteEvent event;
    if(panic_requested.exchange(false))
    {
        // Drop stale messages before releasing; foreground cannot run during ISR.
        while(note_queue.Pop(event)) {}
        synth.Panic();
    }
    else
    {
        for(size_t n = 0; n < 127 && note_queue.Pop(event); ++n)
            synth.Handle(event);
    }
    synth.SetSettings(ReadSettings());
    sounding_note.store(synth.Note(), std::memory_order_relaxed);
    for(size_t i = 0; i < size; ++i)
    {
        const float sample = synth.Process();
        out[0][i] = sample;
        out[1][i] = sample;
    }
}

void TurnEncoder(int index, int steps)
{
    steps *= kEncoderDirection[index];
    switch(index)
    {
        case 0: shared.cutoff.store(Clamp(shared.cutoff.load() + 2 * steps, 0, 100)); break;
        case 1: shared.resonance.store(Clamp(shared.resonance.load() + 2 * steps, 0, 85)); break;
        case 2: shared.release_ms.store(Clamp(shared.release_ms.load() + 25 * steps, 20, 2000)); break;
        case 3: shared.volume.store(Clamp(shared.volume.load() + 2 * steps, 0, 80)); break;
    }
}

float CenteredAxis(float value, float center)
{
    const float delta = value - center;
    const float normalized = delta / (delta > 0 ? (1.0f - center) : center);
    const float amount = std::fabs(normalized);
    if(amount < 0.06f) return 0;
    return std::copysign(Clamp((amount - 0.06f) / 0.94f, 0.0f, 1.0f), normalized);
}

void ScanControls(uint32_t now)
{
    uint16_t ports[2];
    if(controls_ok)
    {
        if(!expanders.Read(ports))
        {
            controls_ok = false;
            panic_requested.store(true);
        }
        else
        {
            for(int i = 0; i < 16; ++i)
            {
                if(keys[i].Update(IsLow(ports, kKeyPins[i]), now))
                    QueueNote({uint8_t(i), uint8_t(60 + 12 * octave_shift + i),
                               uint8_t(keys[i].Pressed() ? 100 : 0)});
            }
            for(int i = 0; i < 4; ++i)
            {
                const int step = encoders[i].Update(
                    IsLow(ports, kEncoderA[i]), IsLow(ports, kEncoderB[i]));
                if(step) TurnEncoder(i, step);
                if(encoder_push[i].Update(IsLow(ports, kEncoderPush[i]), now)
                   && encoder_push[i].Pressed())
                {
                    if(i == 0) shared.waveform.store((shared.waveform.load() + 1) % 4);
                    if(i == 1) octave_shift = Clamp(octave_shift - 1, -2, 2);
                    if(i == 2) octave_shift = Clamp(octave_shift + 1, -2, 2);
                    if(i == 3) panic_requested.store(true);
                }
            }
            if(joystick_push.Update(IsLow(ports, kJoystickPush), now)
               && joystick_push.Pressed()) panic_requested.store(true);
        }
    }
    smooth_x += 0.1f * (CenteredAxis(hw.adc.GetFloat(0), center_x) - smooth_x);
    smooth_y += 0.1f * (CenteredAxis(hw.adc.GetFloat(1), center_y) - smooth_y);
    shared.bend_cents.store(int(200 * smooth_x));
    shared.filter_cents.store(int(2400 * smooth_y));
}

void PollMidi()
{
    midi.Listen();
    for(int n = 0; n < 32 && midi.HasEvents(); ++n)
    {
        const MidiEvent event = midi.PopEvent();
        if(event.channel != kMidiChannel) continue;
        if(event.type == NoteOn || event.type == NoteOff)
            QueueNote({uint8_t(16 + event.data[0]), event.data[0],
                       uint8_t(event.type == NoteOff ? 0 : event.data[1])});
        if((event.type == ChannelMode
            && (event.cm_type == AllNotesOff || event.cm_type == AllSoundOff))
           || (event.type == ControlChange
               && (event.data[0] == 120 || event.data[0] == 123)))
            panic_requested.store(true);
    }
}

void DrawDisplay()
{
    if(!OledTransport::Healthy() || !OledTransport::Ready()) return;
    static const char* names[] = {"SAW", "SQR", "TRI", "SIN"};
    char text[24];
    display.Fill(false);
    display.SetCursor(0, 0);
    std::snprintf(text, sizeof(text), "VAMP %s Oct%+d %s",
                  names[shared.waveform.load()], octave_shift, controls_ok ? "" : "IO!");
    display.WriteString(text, Font_6x8, true);
    display.SetCursor(0, 8);
    std::snprintf(text, sizeof(text), "Cut%3d Res%2d",
                  shared.cutoff.load(), shared.resonance.load());
    display.WriteString(text, Font_6x8, true);
    display.SetCursor(0, 16);
    std::snprintf(text, sizeof(text), "Rel%4dms Vol%2d",
                  shared.release_ms.load(), shared.volume.load());
    display.WriteString(text, Font_6x8, true);
    display.SetCursor(0, 24);
    const int note = sounding_note.load(std::memory_order_relaxed);
    if(note < 0) std::snprintf(text, sizeof(text), "Ready - press a key");
    else std::snprintf(text, sizeof(text), "MIDI note %d", note);
    display.WriteString(text, Font_6x8, true);
    display.Update();
}

int main()
{
    hw.Init(); // libDaisy v9 detects Seed3 and initializes its audio codec.
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48); // One callback per millisecond.

    controls_ok = expanders.Init();
    uint16_t initial[2] = {0xFFFF, 0xFFFF};
    if(controls_ok) controls_ok = expanders.Read(initial);
    for(int i = 0; i < 4; ++i)
        encoders[i].Init(IsLow(initial, kEncoderA[i]), IsLow(initial, kEncoderB[i]));

    AdcChannelConfig channels[2];
    channels[0].InitSingle(seed::D15);
    channels[1].InitSingle(seed::D16);
    hw.adc.Init(channels, 2);
    hw.adc.Start();
    System::Delay(10);
    center_x = center_y = 0;
    for(int i = 0; i < 64; ++i)
    {
        center_x += hw.adc.GetFloat(0) / 64.0f;
        center_y += hw.adc.GetFloat(1) / 64.0f;
        System::Delay(1);
    }
    center_x = Clamp(center_x, 0.1f, 0.9f);
    center_y = Clamp(center_y, 0.1f, 0.9f);

    if(kEnableOled)
    {
        Display::Config c;
        auto& transport = c.driver_config.transport_config;
        transport.i2c_address = kOledAddress;
        transport.i2c_config.speed = I2CHandle::Config::Speed::I2C_400KHZ;
        transport.i2c_config.pin_config.scl = seed::D11;
        transport.i2c_config.pin_config.sda = seed::D12;
        display.Init(c);
        // Complete initialization commands before rendering the first frame.
        while(!OledTransport::Ready()) OledTransport::Service();
    }

    MidiUartHandler::Config midi_config;
    midi_config.transport_config.periph = UartHandler::Config::Peripheral::USART_1;
    midi_config.transport_config.rx = seed::D14;
    midi_config.transport_config.tx = seed::D13; // Unconnected on this board.
    midi.Init(midi_config);
    midi.StartReceive();

    synth.Init(hw.AudioSampleRate());
    hw.StartAudio(AudioCallback);
    uint32_t last_scan = System::GetNow(), last_display = last_scan;
    while(true)
    {
        PollMidi();
        const uint32_t now = System::GetNow();
        if(uint32_t(now - last_scan) >= 1)
        {
            last_scan = now;
            ScanControls(now);
        }
        if(kEnableOled && uint32_t(now - last_display) >= 100)
        {
            last_display = now;
            DrawDisplay();
        }
        if(kEnableOled) OledTransport::Service();
        // Solid LED = controls initialized; blinking LED = expander error.
        hw.SetLed(controls_ok || ((now / 250) & 1));
    }
}
