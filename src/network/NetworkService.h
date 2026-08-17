#pragma once

#include <cstdint>

namespace ArrowLabNetwork
{
    using UpdateCallback = void (*)();

    enum class CredentialTestState : uint8_t
    {
        Idle = 0,
        Testing = 1,
        Succeeded = 2,
        Failed = 3
    };

    struct Info
    {
        bool configured = false;
        bool connected = false;
        bool savedCredentials = false;
        char hostname[24] = {};
        char ssid[33] = {};
        char ip[16] = {};
        char mac[18] = {};
        int16_t rssiDbm = -127;
        CredentialTestState credentialTest =
            CredentialTestState::Idle;
    };

    void begin(const char *hostname);
    void handle();

    bool configured();
    bool connected();
    bool hasSavedCredentials();
    Info info();

    bool testCredentials(const char *ssid, const char *password);
    bool commitTestedCredentials();
    void revertCredentialTest();
    void forgetSavedCredentials();

    void setUpdateCallbacks(
        UpdateCallback startCallback,
        UpdateCallback finishCallback);
    bool updateInProgress();
}
