#pragma once

#include <cstdint>
#include <cstddef>

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

    struct ScanResult
    {
        char ssid[33] = {};
        int16_t rssiDbm = -127;
        bool secured = false;
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
    bool savedPasswordForSsid(
        const char *ssid,
        char *password,
        size_t passwordSize);
    size_t savedProfileSsids(
        char ssids[][33],
        size_t maximumProfiles);
    bool forgetSavedProfile(const char *ssid);
    Info info();

    bool testCredentials(const char *ssid, const char *password);
    bool commitTestedCredentials();
    void revertCredentialTest();
    void forgetSavedCredentials();

    bool startScan();
    int scanComplete();
    size_t takeScanResults(ScanResult *results, size_t maximumResults);

    void setUpdateCallbacks(
        UpdateCallback startCallback,
        UpdateCallback finishCallback);
    bool updateInProgress();
}
