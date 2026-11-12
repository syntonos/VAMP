#pragma once

#include "controls.h"
#include "Control/adsr.h"
#include "Filters/svf.h"
#include "Synthesis/oscillator.h"
#include <cmath>

namespace vamp
{
struct SoundSettings
{
    int cutoff = 55;       // 0..100, logarithmic frequency mapping.
    int resonance = 15;    // 0..85 percent.
    int release_ms = 250;  // 20..2000 ms.
    int volume = 20;       // Conservative initial level, 0..80 percent.
    int waveform = 0;      // Saw, square, triangle, sine.
    int bend_cents = 0;    // Joystick X: +/-200 cents.
    int filter_cents = 0;  // Joystick Y: +/-2400 cents.
};

class Synth
{
  public:
    void Init(float sample_rate)
    {
        osc_.Init(sample_rate);
        osc_.SetAmp(1.0f);
        envelope_.Init(sample_rate);
        envelope_.SetTime(daisysp::ADSR_SEG_ATTACK, 0.01f);
        envelope_.SetTime(daisysp::ADSR_SEG_DECAY, 0.15f);
        envelope_.SetSustainLevel(0.65f);
        filter_.Init(sample_rate);
        filter_.SetDrive(0.1f);
        SetSettings(SoundSettings{});
        volume_ = volume_target_;
        cutoff_ = cutoff_target_;
        frequency_ = 261.625565f;
    }

    void Handle(NoteEvent event)
    {
        notes_.Apply(event);
        if(event.velocity)
        {
            // Soft retrigger avoids resetting an active envelope to zero.
            envelope_.Retrigger(false);
            velocity_target_ = notes_.Velocity();
        }
        else if(notes_.Gate())
            velocity_target_ = notes_.Velocity();
        // Keep the last velocity during release so the release tail is audible.
        UpdatePitch();
    }

    void Panic()
    {
        notes_.Clear();
        // Gate drops, using the normal release to avoid a discontinuity.
    }

    void SetSettings(const SoundSettings& s)
    {
        static constexpr uint8_t waves[4] = {
            daisysp::Oscillator::WAVE_POLYBLEP_SAW,
            daisysp::Oscillator::WAVE_POLYBLEP_SQUARE,
            daisysp::Oscillator::WAVE_POLYBLEP_TRI,
            daisysp::Oscillator::WAVE_SIN
        };
        osc_.SetWaveform(waves[Clamp(s.waveform, 0, 3)]);
        volume_target_ = Clamp(s.volume, 0, 80) * 0.01f;
        cutoff_target_ = Clamp(
            80.0f * std::pow(150.0f, Clamp(s.cutoff, 0, 100) * 0.01f)
                * std::pow(2.0f, Clamp(s.filter_cents, -2400, 2400) / 1200.0f),
            40.0f, 12000.0f);
        filter_.SetRes(Clamp(s.resonance, 0, 85) * 0.01f);
        envelope_.SetTime(daisysp::ADSR_SEG_RELEASE,
                          Clamp(s.release_ms, 20, 2000) * 0.001f);
        bend_ = Clamp(s.bend_cents, -200, 200) * 0.01f;
        UpdatePitch();
    }

    // Called for every audio sample; no I/O, allocation, or waiting here.
    float Process()
    {
        volume_ += 0.002f * (volume_target_ - volume_);
        cutoff_ += 0.002f * (cutoff_target_ - cutoff_);
        frequency_ += 0.004f * (frequency_target_ - frequency_);
        velocity_ += 0.002f * (velocity_target_ - velocity_);
        osc_.SetFreq(frequency_);
        filter_.SetFreq(cutoff_);
        const float amplitude = envelope_.Process(notes_.Gate());
        filter_.Process(osc_.Process());
        const float sample = filter_.Low() * amplitude * volume_ * velocity_;
        return Clamp(sample, -0.9f, 0.9f);
    }

    int Note() const { return notes_.Note(); }

  private:
    void UpdatePitch()
    {
        if(notes_.Gate())
            frequency_target_ = 440.0f
                * std::pow(2.0f, (notes_.Note() + bend_ - 69.0f) / 12.0f);
    }

    HeldNotes notes_;
    daisysp::Oscillator osc_;
    daisysp::Adsr envelope_;
    daisysp::Svf filter_;
    float volume_ = 0, volume_target_ = 0;
    float cutoff_ = 1000, cutoff_target_ = 1000;
    float frequency_ = 261.625565f, frequency_target_ = 261.625565f;
    float velocity_ = 0, velocity_target_ = 0;
    float bend_ = 0;
};
} // namespace vamp
