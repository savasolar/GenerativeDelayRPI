#include "PedalboardControls.h"

#include "CtLog.h"
#include <cerrno>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#endif

namespace
{
constexpr const char* kSpiPath = "/dev/spidev0.0";
constexpr const char* kChipPath = "/dev/gpiochip0";
constexpr unsigned kFswOffset = 17; // BCM GPIO17
constexpr unsigned kLedOffset = 27; // BCM GPIO27
constexpr int kPollMs = 20;
constexpr int kDebounceTicks = 3; // ~60 ms at 20 ms poll
}

void PedalboardControls::start(const Callbacks& cb)
{
    stop();
    cb_ = cb;

#if !defined(__linux__)
    // Stub: leave defaults. Stereo fixed to 0 if callback present.
    if (cb_.setStereo && cb_.user)
        cb_.setStereo(cb_.user, 0.0f);
    if (cb_.setEffectEnabled && cb_.user)
        cb_.setEffectEnabled(cb_.user, false); // start bypassed
    potsOk_ = false;
    gpioOk_ = false;
    return;
#else
    // Start bypassed / LED off before opening devices.
    if (cb_.setEffectEnabled && cb_.user)
        cb_.setEffectEnabled(cb_.user, false);
    if (cb_.setStereo && cb_.user)
        cb_.setStereo(cb_.user, 0.0f);

//    spiFd_ = ::open(kSpiPath, O_RDWR);
//    if (spiFd_ >= 0)
//    {
//        uint8_t mode = SPI_MODE_0;
//        uint8_t bits = 8;
//        uint32_t speed = 1000000;
//        if (ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0
//            || ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0
//            || ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
//        {
//            ::close(spiFd_);
//            spiFd_ = -1;
//        }
//    }
//    potsOk_ = (spiFd_ >= 0);

    spiFd_ = ::open(kSpiPath, O_RDWR);
    if (spiFd_ < 0)
    {
        CT_LOG ("PB: SPI open " << kSpiPath << " FAILED: " << std::strerror (errno));
    }
    else
    {
        uint8_t mode = SPI_MODE_0;
        uint8_t bits = 8;
        uint32_t speed = 1000000;
        if (ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0
            || ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0
            || ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0)
        {
            CT_LOG ("PB: SPI config ioctl FAILED: " << std::strerror (errno));
            ::close(spiFd_);
            spiFd_ = -1;
        }
    }
    potsOk_ = (spiFd_ >= 0);
    CT_LOG ("PB: pots " << (potsOk_ ? "OK" : "UNAVAILABLE"));



    chipFd_ = ::open(kChipPath, O_RDONLY);
    if (chipFd_ < 0)
        CT_LOG ("PB: GPIO chip open " << kChipPath << " FAILED: " << std::strerror (errno));
    if (chipFd_ >= 0)
    {
        // Request FSW as input with pull-up (pressed = to GND = low).
        gpio_v2_line_request fswReq{};
        std::snprintf(fswReq.consumer, sizeof(fswReq.consumer), "ct-fsw");
        fswReq.num_lines = 1;
        fswReq.offsets[0] = kFswOffset;
        fswReq.config.flags = GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
//        if (ioctl(chipFd_, GPIO_V2_GET_LINE_IOCTL, &fswReq) >= 0)
//            fswFd_ = fswReq.fd;
        if (ioctl(chipFd_, GPIO_V2_GET_LINE_IOCTL, &fswReq) >= 0)
            fswFd_ = fswReq.fd;
        else
            CT_LOG ("PB: FSW GPIO17 request FAILED: " << std::strerror (errno));

        
        // Request LED as output, start low (off).
        gpio_v2_line_request ledReq{};
        std::snprintf(ledReq.consumer, sizeof(ledReq.consumer), "ct-led");
        ledReq.num_lines = 1;
        ledReq.offsets[0] = kLedOffset;
        ledReq.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
        ledReq.config.num_attrs = 1;
        ledReq.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
        ledReq.config.attrs[0].mask = 1; // line 0
        ledReq.config.attrs[0].attr.values = 0;
//        if (ioctl(chipFd_, GPIO_V2_GET_LINE_IOCTL, &ledReq) >= 0)
//            ledFd_ = ledReq.fd;
        if (ioctl(chipFd_, GPIO_V2_GET_LINE_IOCTL, &ledReq) >= 0)
            ledFd_ = ledReq.fd;
        else
            CT_LOG ("PB: LED GPIO27 request FAILED: " << std::strerror (errno));
    }
    gpioOk_ = (fswFd_ >= 0 && ledFd_ >= 0);
    CT_LOG ("PB: footswitch " << (fswFd_ >= 0 ? "OK" : "UNAVAILABLE")
            << ", LED " << (ledFd_ >= 0 ? "OK" : "UNAVAILABLE"));
    setLed(false);

    running_ = true;
    worker_ = std::thread([this] { threadMain(); });
    CT_LOG ("PB: control thread started, effect OFF (bypassed)");
#endif
}

void PedalboardControls::stop()
{
    running_ = false;
    if (worker_.joinable())
        worker_.join();

    setLed(false);

    if (fswFd_ >= 0) { ::close(fswFd_); fswFd_ = -1; }
    if (ledFd_ >= 0) { ::close(ledFd_); ledFd_ = -1; }
    if (chipFd_ >= 0) { ::close(chipFd_); chipFd_ = -1; }
    if (spiFd_ >= 0) { ::close(spiFd_); spiFd_ = -1; }
    potsOk_ = false;
    gpioOk_ = false;
}

void PedalboardControls::threadMain()
{
    while (running_.load())
    {
        applyPots();
        pollFootswitch();
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
    }
}

int PedalboardControls::readAdc(int channel)
{
#if defined(__linux__)
    if (spiFd_ < 0 || channel < 0 || channel > 7)
        return -1;

    uint8_t tx[3] = { 0x01, (uint8_t)((0x08 | channel) << 4), 0x00 };
    uint8_t rx[3] = { 0, 0, 0 };
    spi_ioc_transfer tr{};
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 3;
    tr.speed_hz = 1000000;
    tr.bits_per_word = 8;
//    if (ioctl(spiFd_, SPI_IOC_MESSAGE(1), &tr) < 0)
//        return -1;
    if (ioctl(spiFd_, SPI_IOC_MESSAGE(1), &tr) < 0)
    {
        static bool logged = false;   // log once, not 50x/sec
        if (! logged) { CT_LOG ("PB: SPI read ch" << channel << " FAILED: " << std::strerror (errno)); logged = true; }
        return -1;
    }
    return ((rx[1] & 0x03) << 8) | rx[2];
#else
    (void)channel;
    return -1;
#endif
}

void PedalboardControls::applyPots()
{
    if (!cb_.user)
        return;

    const int c0 = readAdc(0);
    const int c1 = readAdc(1);
    const int c2 = readAdc(2);
    const int c3 = readAdc(3);
    if (c0 < 0 || c1 < 0 || c2 < 0 || c3 < 0)
        return;


    // Log only when a pot moves more than ~2% (20 of 1023 counts).
    static int last[4] = { -100, -100, -100, -100 };
    const int now[4] = { c0, c1, c2, c3 };
    bool moved = false;
    for (int i = 0; i < 4; ++i)
        if (std::abs (now[i] - last[i]) > 20) { moved = true; last[i] = now[i]; }
    if (moved)
        CT_LOG ("PB: pots raw=" << c0 << "," << c1 << "," << c2 << "," << c3
                << "  Chaos=" << juce::String (c0 / 1023.0f, 2)
                << " Bloom=" << juce::String (c1 / 1023.0f, 2)
                << " TimeMs=" << (80 + (int) std::lround (c2 / 1023.0f * 720))
                << " Mix=" << juce::String (c3 / 1023.0f, 2));

    

    auto norm = [](int v) {
        return std::clamp(v / 1023.0f, 0.0f, 1.0f);
    };

    if (cb_.setChaos)  cb_.setChaos(cb_.user, norm(c0));
    if (cb_.setBloom)  cb_.setBloom(cb_.user, norm(c1));
    if (cb_.setTimeMs) cb_.setTimeMs(cb_.user, 80 + (int)std::lround(norm(c2) * (800 - 80)));
    if (cb_.setMix)    cb_.setMix(cb_.user, norm(c3));
    if (cb_.setStereo) cb_.setStereo(cb_.user, 0.0f); // fixed unused
}

void PedalboardControls::pollFootswitch()
{
#if defined(__linux__)
    if (fswFd_ < 0 || !cb_.user || !cb_.setEffectEnabled || !cb_.isEffectEnabled)
        return;

    gpio_v2_line_values vals{};
    vals.mask = 1;
//    if (ioctl(fswFd_, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0)
//        return;

    if (ioctl(fswFd_, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals) < 0)
    {
        static bool logged = false;
        if (! logged) { CT_LOG ("PB: FSW read FAILED: " << std::strerror (errno)); logged = true; }
        return;
    }

        
        
    // Pressed = pulled to GND = bit clear.
    const bool pressed = ((vals.bits & 1) == 0);

    if (pressed == debounceState_)
        debounceStable_ = std::min(debounceStable_ + 1, kDebounceTicks + 1);
    else
    {
        debounceState_ = pressed;
        debounceStable_ = 0;
    }

    if (debounceStable_ == kDebounceTicks)
    {
        // Rising edge of press (idle high → pressed low).
        if (pressed && !lastFswPressed_)
        {
            const bool nowOn = !cb_.isEffectEnabled(cb_.user);
            cb_.setEffectEnabled(cb_.user, nowOn);
            setLed(nowOn);
            CT_LOG ("PB: footswitch -> effect " << (nowOn ? "ON" : "OFF (bypassed)"));
        }
        lastFswPressed_ = pressed;
    }
#else
    (void)0;
#endif
}

void PedalboardControls::setLed(bool on)
{
#if defined(__linux__)
    if (ledFd_ < 0)
        return;
    gpio_v2_line_values vals{};
    vals.mask = 1;
    vals.bits = on ? 1 : 0;
//    ioctl(ledFd_, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals);
    if (ioctl(ledFd_, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals) < 0)
        CT_LOG ("PB: LED write FAILED: " << std::strerror (errno));
#else
    (void)on;
#endif
}
