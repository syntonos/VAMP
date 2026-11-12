#pragma once

#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "hid/disp/oled_display.h"
#include <cstring>

namespace vamp
{
// Both expanders use address 0 and independent chip selects. SD_CS stays HIGH.
// At the default SPI1 kernel clock, /128 is deliberately below 1 MHz.
class Expanders
{
  public:
    bool Init()
    {
        using namespace daisy;
        cs_[0].Init(seed::D6, GPIO::Mode::OUTPUT);
        cs_[1].Init(seed::D5, GPIO::Mode::OUTPUT);
        sd_cs_.Init(seed::D7, GPIO::Mode::OUTPUT);
        reset_.Init(seed::D18, GPIO::Mode::OUTPUT);
        cs_[0].Write(true); cs_[1].Write(true); sd_cs_.Write(true);
        reset_.Write(false);
        System::Delay(2);
        reset_.Write(true);
        System::Delay(2);

        SpiHandle::Config c;
        c.periph = SpiHandle::Config::Peripheral::SPI_1;
        c.mode = SpiHandle::Config::Mode::MASTER;
        c.direction = SpiHandle::Config::Direction::TWO_LINES;
        c.nss = SpiHandle::Config::NSS::SOFT;
        c.baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_128;
        c.pin_config.sclk = seed::D8;
        c.pin_config.miso = seed::D9;
        c.pin_config.mosi = seed::D10;
        c.pin_config.nss = Pin(PORTX, 0);
        if(spi_.Init(c) != SpiHandle::Result::OK) return false;

        for(int chip = 0; chip < 2; ++chip)
        {
            // BANK=0, SEQOP=0; register pairs increment A -> B.
            if(!Write(chip, 0x0A, 0x00, 0x00) // IOCON A/B
               || !Write(chip, 0x00, 0xFF, 0xFF) // IODIR: all inputs
               || !Write(chip, 0x02, 0x00, 0x00) // IPOL: no inversion
               || !Write(chip, 0x04, 0x00, 0x00) // No interrupt outputs
               || !Write(chip, 0x0C, 0xFF, 0xFF)) // GPPU: enable pullups
                return false;
            uint8_t tx[4] = {0x41, 0x0C, 0, 0}, rx[4]{};
            if(!Transfer(chip, tx, rx) || rx[2] != 0xFF || rx[3] != 0xFF)
                return false;
            tx[1] = 0x0A; // Also reject a floating/stuck-high MISO line.
            if(!Transfer(chip, tx, rx) || rx[2] != 0 || rx[3] != 0)
                return false;
        }
        return true;
    }

    bool Read(uint16_t ports[2])
    {
        for(int chip = 0; chip < 2; ++chip)
        {
            uint8_t tx[4] = {0x41, 0x12, 0, 0}, rx[4]{};
            if(!Transfer(chip, tx, rx)) return false;
            ports[chip] = uint16_t(rx[2]) | (uint16_t(rx[3]) << 8);
        }
        return true;
    }

  private:
    bool Transfer(int chip, uint8_t* tx, uint8_t* rx)
    {
        cs_[chip].Write(false);
        const auto result = spi_.BlockingTransmitAndReceive(tx, rx, 4, 2);
        cs_[chip].Write(true);
        return result == daisy::SpiHandle::Result::OK;
    }
    bool Write(int chip, uint8_t reg, uint8_t a, uint8_t b)
    {
        uint8_t tx[4] = {0x40, reg, a, b}, rx[4]{};
        return Transfer(chip, tx, rx);
    }
    daisy::SpiHandle spi_;
    daisy::GPIO cs_[2], sd_cs_, reset_;
};

// Queue the OLED's commands and pixels in order; send at most 16 pixels per
// foreground iteration. A whole blocking frame would lose encoder edges.
class OledTransport
{
  public:
    using Config = daisy::SSD130xI2CTransport::Config;
    void Init(const Config& config)
    {
        State& s = Bus();
        s.address = config.i2c_address;
        s.ok = s.i2c.Init(config.i2c_config) == daisy::I2CHandle::Result::OK;
    }
    void SendCommand(uint8_t command) { Append(0x00, &command, 1); }
    void SendData(uint8_t* data, size_t size)
    {
        while(size)
        {
            const size_t count = size < 16 ? size : 16;
            Append(0x40, data, count);
            data += count;
            size -= count;
        }
    }
    static bool Ready() { return Bus().read == Bus().write; }
    static bool Healthy() { return Bus().ok; }
    static void Service()
    {
        State& s = Bus();
        if(Ready()) { s.read = s.write = 0; return; }
        const uint8_t length = s.queue[s.read++];
        if(s.ok)
            s.ok = s.i2c.TransmitBlocking(s.address, &s.queue[s.read], length, 2)
                == daisy::I2CHandle::Result::OK;
        s.read += length;
        if(Ready()) s.read = s.write = 0;
    }

  private:
    struct State
    {
        daisy::I2CHandle i2c;
        uint8_t queue[768]{};
        size_t read = 0, write = 0;
        uint8_t address = 0x3C;
        bool ok = false;
    };
    static State& Bus() { static State state; return state; }
    static void Append(uint8_t control, const uint8_t* data, size_t size)
    {
        State& s = Bus();
        if(!s.ok) return;
        if(s.write + size + 2 > sizeof(s.queue)) { s.ok = false; return; }
        s.queue[s.write++] = uint8_t(size + 1);
        s.queue[s.write++] = control;
        std::memcpy(&s.queue[s.write], data, size);
        s.write += size;
    }
};

class OledDriver : public daisy::SSD130xDriver<128, 32, OledTransport>
{
  public:
    void Update()
    {
        for(uint8_t page = 0; page < 4; ++page)
        {
            transport_.SendCommand(0xB0 + page);
            transport_.SendCommand(0x00);
            transport_.SendCommand(0x10); // Column zero on the SSD1306.
            transport_.SendData(&buffer_[128 * page], 128);
        }
    }
    bool UpdateFinished() { return OledTransport::Ready(); }
};
using Display = daisy::OledDisplay<OledDriver>;
} // namespace vamp
