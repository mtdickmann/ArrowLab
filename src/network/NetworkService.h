#pragma once

namespace ArrowLabNetwork
{
    // Starts a non-blocking station-mode Wi-Fi connection. The OTA service
    // becomes available automatically after the network connection succeeds.
    void begin(const char *hostname);

    // Must be called frequently from loop(). It never waits for Wi-Fi.
    void handle();

    bool configured();
    bool connected();
}
