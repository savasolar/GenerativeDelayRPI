#pragma once

#include <atomic>
#include <thread>

// Pedalboard HAT controls: MCP3008 pots (/dev/spidev0.0) + FSW GPIO17 + LED GPIO27.
// Compiles on any Linux. Without hardware, open() fails and values stay at defaults.
// On Linux opens /dev/spidev0.0 and GPIO; if open fails, defaults stay.

class PedalboardControls
{
public:
    // Callbacks into the processor (setters / bypass toggle).
    struct Callbacks
    {
        void* user = nullptr;
        void (*setChaos)(void*, float) = nullptr;
        void (*setBloom)(void*, float) = nullptr;
        void (*setTimeMs)(void*, int) = nullptr;
        void (*setMix)(void*, float) = nullptr;
        void (*setStereo)(void*, float) = nullptr;
        void (*setEffectEnabled)(void*, bool) = nullptr; // true = effect on, LED on
        bool (*isEffectEnabled)(void*) = nullptr;
    };

    PedalboardControls() = default;
    ~PedalboardControls() { stop(); }

    void start(const Callbacks& cb);
    void stop();

    // True if SPI opened. GPIO may still have failed independently.
    bool potsOk() const { return potsOk_.load(); }
    bool gpioOk() const { return gpioOk_.load(); }

private:
    void threadMain();
    int  readAdc(int channel); // 0..1023, or -1 on error
    void applyPots();
    void pollFootswitch();
    void setLed(bool on);

    Callbacks cb_{};
    std::atomic<bool> running_{false};
    std::atomic<bool> potsOk_{false};
    std::atomic<bool> gpioOk_{false};
    std::thread worker_;

    int spiFd_ = -1;
    int chipFd_ = -1;
    int fswFd_ = -1;
    int ledFd_ = -1;

    bool lastFswPressed_ = false;
    int  debounceStable_ = 0;
    bool debounceState_ = false;
};
