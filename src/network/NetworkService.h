#pragma once

#include <cstdint>

namespace ArrowLabNetwork
{
    struct Info
    {
        bool configured = false;
        bool connected = false;
        char hostname[24] = {};
        char ssid[33] = {};
        char ip[16] = {};
        char mac[18] = {};
        int16_t rssiDbm = -127;
    };

    // Starts a non-blocking station-mode Wi-Fi connection. The OTA service
    // becomes available automatically after the network connection succeeds.
    void begin(const char *hostname);

    // Must be called frequently from loop(). It never waits for Wi-Fi.
    void handle();

    bool configured();
    bool connected();
    Info info();
}
