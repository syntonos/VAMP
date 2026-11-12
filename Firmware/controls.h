#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace vamp
{
template <typename T> T Clamp(T x, T low, T high)
{
    return x < low ? low : (x > high ? high : x);
}

// All bit numbers are GPA0..7 = 0..7, GPB0..7 = 8..15.
// Mapped from U6/U7 in vamp.kicad_pcb, revision USB3V3-9, version 7.
struct ExpanderPin
{
    uint8_t chip; // U6 = 0, U7 = 1
    uint8_t bit;
};
constexpr ExpanderPin kKeyPins[16] = {
    {0, 13}, {0, 11}, {0, 14}, {0, 12}, // SW1..4
    {0, 15}, {0, 0},  {0, 2},  {0, 1},  // SW5..8
    {1, 12}, {1, 14}, {1, 13}, {1, 15}, // SW9..12
    {1, 0},  {1, 1},  {1, 2},  {1, 3}   // SW13..16
};
constexpr ExpanderPin kEncoderA[4] = {{0, 10}, {0, 7}, {0, 4}, {1, 11}};
constexpr ExpanderPin kEncoderB[4] = {{0, 9}, {0, 6}, {0, 3}, {1, 10}};
constexpr ExpanderPin kEncoderPush[4] = {{0, 8}, {0, 5}, {1, 4}, {1, 9}};
constexpr ExpanderPin kJoystickPush = {1, 8};

inline bool IsLow(const uint16_t ports[2], ExpanderPin pin)
{
    return (ports[pin.chip] & (1u << pin.bit)) == 0;
}

// Time-based debounce: irregular foreground scheduling does not shorten it.
class Button
{
  public:
    bool Update(bool raw_pressed, uint32_t now_ms)
    {
        if(raw_pressed != candidate_)
        {
            candidate_ = raw_pressed;
            changed_at_ = now_ms;
        }
        if(candidate_ != pressed_ && uint32_t(now_ms - changed_at_) >= 5)
        {
            pressed_ = candidate_;
            return true;
        }
        return false;
    }
    bool Pressed() const { return pressed_; }

  private:
    bool pressed_ = false, candidate_ = false;
    uint32_t changed_at_ = 0;
};

// Gray-code decoding rejects invalid transitions; four edges make one detent.
class Quadrature
{
  public:
    void Init(bool a, bool b)
    {
        previous_ = uint8_t((a ? 2 : 0) | (b ? 1 : 0));
        edges_ = 0;
    }
    int Update(bool a, bool b)
    {
        static constexpr int8_t direction[16] = {
            0, -1, 1, 0, 1, 0, 0, -1,
            -1, 0, 0, 1, 0, 1, -1, 0
        };
        const uint8_t state = uint8_t((a ? 2 : 0) | (b ? 1 : 0));
        if((state ^ previous_) == 3)
            edges_ = 0;
        else
            edges_ += direction[(previous_ << 2) | state];
        previous_ = state;
        if(edges_ >= 4) { edges_ = 0; return 1; }
        if(edges_ <= -4) { edges_ = 0; return -1; }
        return 0;
    }

  private:
    uint8_t previous_ = 3;
    int edges_ = 0;
};

struct NoteEvent
{
    uint8_t id;       // Local keys 0..15; MIDI notes 16..143.
    uint8_t note;     // MIDI note number.
    uint8_t velocity; // Zero means release.
};

// One producer (main loop) and one consumer (audio interrupt).
// Release/acquire protects both slot contents and slot reuse.
template <size_t Capacity> class NoteQueue
{
  public:
    static_assert(Capacity > 1, "A queue needs at least two slots");
    bool Push(NoteEvent event)
    {
        const uint32_t write = write_.load(std::memory_order_relaxed);
        const uint32_t next = (write + 1) % Capacity;
        if(next == read_.load(std::memory_order_acquire)) return false;
        events_[write] = event;
        write_.store(next, std::memory_order_release);
        return true;
    }
    bool Pop(NoteEvent& event)
    {
        const uint32_t read = read_.load(std::memory_order_relaxed);
        if(read == write_.load(std::memory_order_acquire)) return false;
        event = events_[read];
        read_.store((read + 1) % Capacity, std::memory_order_release);
        return true;
    }

  private:
    NoteEvent events_[Capacity]{};
    std::atomic<uint32_t> write_{0}, read_{0};
};

// Last-note priority, keeping local-key identity separate from MIDI identity.
// Releasing one C4 cannot release a different C4 held by another source.
class HeldNotes
{
  public:
    void Apply(NoteEvent event)
    {
        for(size_t i = 0; i < count_; ++i)
        {
            if(notes_[i].id == event.id)
            {
                for(size_t j = i + 1; j < count_; ++j) notes_[j - 1] = notes_[j];
                --count_;
                break;
            }
        }
        if(event.velocity && count_ < 144) notes_[count_++] = event;
    }
    void Clear() { count_ = 0; }
    bool Gate() const { return count_ != 0; }
    int Note() const { return count_ ? notes_[count_ - 1].note : -1; }
    float Velocity() const
    {
        return count_ ? notes_[count_ - 1].velocity / 127.0f : 0.0f;
    }

  private:
    NoteEvent notes_[144]{};
    size_t count_ = 0;
};
} // namespace vamp
