#include "network/NetworkService.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

#if __has_include("NetworkSecrets.h")
#include "NetworkSecrets.h"
#define ARROWLAB_HAS_NETWORK_SECRETS 1
#else
#define ARROWLAB_HAS_NETWORK_SECRETS 0
#endif

namespace
{
    constexpr uint32_t CONNECTION_LOG_INTERVAL_MS = 10000;
    constexpr uint32_t CREDENTIAL_TEST_TIMEOUT_MS = 20000;
    constexpr char PREFERENCES_NAMESPACE[] = "arrowlab-net";
    constexpr char SSID_KEY[] = "ssid";
    constexpr char PASSWORD_KEY[] = "password";
    constexpr char PROFILE_NEXT_KEY[] = "pnext";
    constexpr size_t MAX_SAVED_PROFILES = 8;

    bool started = false;
    bool otaStarted = false;
    bool updating = false;
    bool savedCredentialsAvailable = false;
    bool activeProfileForgotten = false;
    uint32_t lastConnectionLog = 0;
    uint32_t credentialDeadline = 0;
    const char *deviceHostname = "arrowlab";
    ArrowLabNetwork::UpdateCallback updateStartCallback = nullptr;
    ArrowLabNetwork::UpdateCallback updateFinishCallback = nullptr;
    ArrowLabNetwork::CredentialTestState credentialState =
        ArrowLabNetwork::CredentialTestState::Idle;

    char activeSsid[33] = {};
    char activePassword[65] = {};
    char previousSsid[33] = {};
    char previousPassword[65] = {};
    char pendingSsid[33] = {};
    char pendingPassword[65] = {};

    void copyText(char *destination, size_t size, const char *source)
    {
        snprintf(destination, size, "%s", source != nullptr ? source : "");
    }

    void connectUsing(const char *ssid, const char *password)
    {
        WiFi.disconnect(false, false);
        delay(25);
        WiFi.begin(ssid, password);
    }

    bool loadSavedCredentials()
    {
        Preferences preferences;
        if (!preferences.begin(PREFERENCES_NAMESPACE, true)) return false;
        const String ssid = preferences.getString(SSID_KEY, "");
        const String password = preferences.getString(PASSWORD_KEY, "");
        preferences.end();

        if (ssid.isEmpty()) return false;
        copyText(activeSsid, sizeof(activeSsid), ssid.c_str());
        copyText(activePassword, sizeof(activePassword), password.c_str());
        return true;
    }

    void profileKey(char *key, size_t keySize, size_t index, char suffix)
    {
        snprintf(key, keySize, "p%u%c", static_cast<unsigned>(index), suffix);
    }

    bool saveProfile(
        Preferences &preferences,
        const char *ssid,
        const char *password)
    {
        if (ssid == nullptr || ssid[0] == '\0') return true;

        size_t profileIndex = MAX_SAVED_PROFILES;
        size_t emptyIndex = MAX_SAVED_PROFILES;
        for (size_t index = 0; index < MAX_SAVED_PROFILES; ++index) {
            char ssidKey[5];
            profileKey(ssidKey, sizeof(ssidKey), index, 's');
            const String savedSsid = preferences.getString(ssidKey, "");
            if (savedSsid == ssid) {
                profileIndex = index;
                break;
            }
            if (savedSsid.isEmpty() && emptyIndex == MAX_SAVED_PROFILES) {
                emptyIndex = index;
            }
        }

        if (profileIndex == MAX_SAVED_PROFILES) {
            profileIndex = emptyIndex < MAX_SAVED_PROFILES
                ? emptyIndex
                : preferences.getUChar(PROFILE_NEXT_KEY, 0)
                    % MAX_SAVED_PROFILES;
        }

        char ssidKey[5];
        char passwordKey[5];
        profileKey(ssidKey, sizeof(ssidKey), profileIndex, 's');
        profileKey(passwordKey, sizeof(passwordKey), profileIndex, 'p');
        const size_t written = preferences.putString(ssidKey, ssid);
        preferences.putString(
            passwordKey,
            password != nullptr ? password : "");
        preferences.putUChar(
            PROFILE_NEXT_KEY,
            static_cast<uint8_t>(
                (profileIndex + 1) % MAX_SAVED_PROFILES));
        return written > 0;
    }

    bool savePendingCredentials()
    {
        Preferences preferences;
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return false;

        // Preserve the network that was active before switching, including
        // credentials saved by older single-profile firmware.
        const bool previousStored =
            activeProfileForgotten
                ? true
                : saveProfile(
                    preferences,
                    activeSsid,
                    activePassword);
        const size_t ssidWritten =
            preferences.putString(SSID_KEY, pendingSsid);
        preferences.putString(PASSWORD_KEY, pendingPassword);
        const bool pendingStored =
            saveProfile(preferences, pendingSsid, pendingPassword);
        preferences.end();
        return previousStored && ssidWritten > 0 && pendingStored;
    }

    bool loadProfilePassword(
        const char *ssid,
        char *password,
        size_t passwordSize)
    {
        if (
            ssid == nullptr
            || ssid[0] == '\0'
            || password == nullptr
            || passwordSize == 0
        ) {
            return false;
        }

        if (
            !activeProfileForgotten
            && strcmp(ssid, activeSsid) == 0
        ) {
            copyText(password, passwordSize, activePassword);
            return true;
        }

        Preferences preferences;
        if (preferences.begin(PREFERENCES_NAMESPACE, true)) {
            for (size_t index = 0; index < MAX_SAVED_PROFILES; ++index) {
                char ssidKey[5];
                char passwordKey[5];
                profileKey(ssidKey, sizeof(ssidKey), index, 's');
                profileKey(passwordKey, sizeof(passwordKey), index, 'p');
                const String savedSsid = preferences.getString(ssidKey, "");
                if (savedSsid == ssid) {
                    const String savedPassword =
                        preferences.getString(passwordKey, "");
                    preferences.end();
                    copyText(password, passwordSize, savedPassword.c_str());
                    return true;
                }
            }
            preferences.end();
        }

#if ARROWLAB_HAS_NETWORK_SECRETS
        if (strcmp(ssid, ARROWLAB_WIFI_SSID) == 0) {
            copyText(password, passwordSize, ARROWLAB_WIFI_PASSWORD);
            return true;
        }
#endif
        return false;
    }

    void startOta()
    {
#if ARROWLAB_HAS_NETWORK_SECRETS
        ArduinoOTA.setHostname(deviceHostname);
        if (ARROWLAB_OTA_PASSWORD[0] != '\0') {
            ArduinoOTA.setPassword(ARROWLAB_OTA_PASSWORD);
        }
#endif
        ArduinoOTA.onStart([]() {
            updating = true;
            if (updateStartCallback != nullptr) updateStartCallback();
            Serial.printf(
                "AL_NET,OTA,START,%s\n",
                ArduinoOTA.getCommand() == U_FLASH
                    ? "FIRMWARE"
                    : "FILESYSTEM");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("AL_NET,OTA,COMPLETE");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            const unsigned int percent =
                total == 0 ? 0 : (progress * 100U) / total;
            static unsigned int lastPercent = 101;
            if (percent != lastPercent && percent % 10U == 0U) {
                Serial.printf("AL_NET,OTA,PROGRESS,%u%%\n", percent);
                lastPercent = percent;
            }
        });
        ArduinoOTA.onError([](ota_error_t error) {
            updating = false;
            if (updateFinishCallback != nullptr) updateFinishCallback();
            Serial.printf("AL_NET,OTA,ERROR,%u\n", error);
        });
        ArduinoOTA.begin();
        otaStarted = true;
        Serial.printf(
            "AL_NET,OTA,READY,%s.local,%s\n",
            deviceHostname,
            WiFi.localIP().toString().c_str());
    }
}

namespace ArrowLabNetwork
{
    void begin(const char *hostname)
    {
        if (started) return;
        started = true;
        deviceHostname =
            hostname != nullptr && hostname[0] != '\0'
                ? hostname
                : "arrowlab";

        savedCredentialsAvailable = loadSavedCredentials();
        if (!savedCredentialsAvailable) {
#if ARROWLAB_HAS_NETWORK_SECRETS
            copyText(
                activeSsid,
                sizeof(activeSsid),
                ARROWLAB_WIFI_SSID);
            copyText(
                activePassword,
                sizeof(activePassword),
                ARROWLAB_WIFI_PASSWORD);
#endif
        }

        WiFi.mode(WIFI_STA);
        WiFi.setHostname(deviceHostname);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);

        if (activeSsid[0] != '\0') {
            connectUsing(activeSsid, activePassword);
            Serial.printf(
                "AL_NET,WIFI,CONNECTING,%s,%s,%s\n",
                deviceHostname,
                activeSsid,
                savedCredentialsAvailable ? "NVS" : "FALLBACK");
        } else {
            Serial.println(
                "AL_NET,DISABLED,no saved or fallback credentials");
        }
    }

    void handle()
    {
        if (!started) return;

        if (credentialState == CredentialTestState::Testing) {
            if (
                WiFi.status() == WL_CONNECTED
                && WiFi.SSID() == pendingSsid
            ) {
                credentialState = CredentialTestState::Succeeded;
                Serial.printf(
                    "AL_NET,WIFI,TEST_OK,%s\n",
                    pendingSsid);
            } else if (
                static_cast<int32_t>(millis() - credentialDeadline) >= 0
            ) {
                credentialState = CredentialTestState::Failed;
                connectUsing(previousSsid, previousPassword);
                copyText(activeSsid, sizeof(activeSsid), previousSsid);
                copyText(
                    activePassword,
                    sizeof(activePassword),
                    previousPassword);
                Serial.printf(
                    "AL_NET,WIFI,TEST_FAILED,%s,REVERTING\n",
                    pendingSsid);
            }
        }

        if (WiFi.status() == WL_CONNECTED) {
            if (!otaStarted) startOta();
            ArduinoOTA.handle();
            return;
        }

        const uint32_t now = millis();
        if (now - lastConnectionLog >= CONNECTION_LOG_INTERVAL_MS) {
            lastConnectionLog = now;
            Serial.printf(
                "AL_NET,WIFI,WAITING,%s,STATUS=%d\n",
                deviceHostname,
                static_cast<int>(WiFi.status()));
        }
    }

    bool configured()
    {
        return activeSsid[0] != '\0'
            || credentialState == CredentialTestState::Testing
            || credentialState == CredentialTestState::Succeeded;
    }

    bool connected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    bool hasSavedCredentials()
    {
        return savedCredentialsAvailable;
    }

    bool savedPasswordForSsid(
        const char *ssid,
        char *password,
        size_t passwordSize)
    {
        return loadProfilePassword(ssid, password, passwordSize);
    }

    size_t savedProfileSsids(
        char ssids[][33],
        size_t maximumProfiles)
    {
        if (ssids == nullptr || maximumProfiles == 0) return 0;

        Preferences preferences;
        if (!preferences.begin(PREFERENCES_NAMESPACE, true)) return 0;

        size_t written = 0;
        const String primarySsid = preferences.getString(SSID_KEY, "");
        if (!primarySsid.isEmpty()) {
            copyText(ssids[written], sizeof(ssids[written]), primarySsid.c_str());
            ++written;
        }

        for (
            size_t index = 0;
            index < MAX_SAVED_PROFILES && written < maximumProfiles;
            ++index
        ) {
            char ssidKey[5];
            profileKey(ssidKey, sizeof(ssidKey), index, 's');
            const String savedSsid = preferences.getString(ssidKey, "");
            if (savedSsid.isEmpty()) continue;

            bool duplicate = false;
            for (size_t existing = 0; existing < written; ++existing) {
                if (savedSsid == ssids[existing]) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            copyText(
                ssids[written],
                sizeof(ssids[written]),
                savedSsid.c_str());
            ++written;
        }
        preferences.end();
        return written;
    }

    bool forgetSavedProfile(const char *ssid)
    {
        if (ssid == nullptr || ssid[0] == '\0') return false;

        Preferences preferences;
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) return false;

        bool removed = false;
        if (preferences.getString(SSID_KEY, "") == ssid) {
            preferences.remove(SSID_KEY);
            preferences.remove(PASSWORD_KEY);
            savedCredentialsAvailable = false;
            removed = true;
        }

        for (size_t index = 0; index < MAX_SAVED_PROFILES; ++index) {
            char ssidKey[5];
            char passwordKey[5];
            profileKey(ssidKey, sizeof(ssidKey), index, 's');
            profileKey(passwordKey, sizeof(passwordKey), index, 'p');
            if (preferences.getString(ssidKey, "") == ssid) {
                preferences.remove(ssidKey);
                preferences.remove(passwordKey);
                removed = true;
            }
        }
        preferences.end();

        if (strcmp(ssid, activeSsid) == 0) {
            activeProfileForgotten = true;
        }

        Serial.printf(
            "AL_NET,WIFI,FORGET,%s,%s\n",
            ssid,
            removed ? "REMOVED" : "NOT_FOUND");
        return removed;
    }

    Info info()
    {
        Info result;
        result.configured = configured();
        result.connected = connected();
        result.savedCredentials = savedCredentialsAvailable;
        result.credentialTest = credentialState;
        copyText(result.hostname, sizeof(result.hostname), deviceHostname);
        copyText(result.mac, sizeof(result.mac), WiFi.macAddress().c_str());

        if (result.connected) {
            copyText(result.ssid, sizeof(result.ssid), WiFi.SSID().c_str());
            copyText(
                result.ip,
                sizeof(result.ip),
                WiFi.localIP().toString().c_str());
            result.rssiDbm = static_cast<int16_t>(WiFi.RSSI());
        }
        return result;
    }

    bool testCredentials(const char *ssid, const char *password)
    {
        if (
            ssid == nullptr
            || ssid[0] == '\0'
            || credentialState == CredentialTestState::Testing
        ) {
            return false;
        }

        copyText(previousSsid, sizeof(previousSsid), activeSsid);
        copyText(
            previousPassword,
            sizeof(previousPassword),
            activePassword);
        copyText(pendingSsid, sizeof(pendingSsid), ssid);
        copyText(pendingPassword, sizeof(pendingPassword), password);
        credentialState = CredentialTestState::Testing;
        credentialDeadline = millis() + CREDENTIAL_TEST_TIMEOUT_MS;
        connectUsing(pendingSsid, pendingPassword);
        return true;
    }

    bool commitTestedCredentials()
    {
        if (credentialState != CredentialTestState::Succeeded) return false;
        if (!savePendingCredentials()) return false;

        copyText(activeSsid, sizeof(activeSsid), pendingSsid);
        copyText(activePassword, sizeof(activePassword), pendingPassword);
        savedCredentialsAvailable = true;
        activeProfileForgotten = false;
        credentialState = CredentialTestState::Idle;
        return true;
    }

    void revertCredentialTest()
    {
        if (credentialState == CredentialTestState::Idle) return;
        connectUsing(previousSsid, previousPassword);
        copyText(activeSsid, sizeof(activeSsid), previousSsid);
        copyText(activePassword, sizeof(activePassword), previousPassword);
        credentialState = CredentialTestState::Idle;
    }

    void forgetSavedCredentials()
    {
        Preferences preferences;
        if (preferences.begin(PREFERENCES_NAMESPACE, false)) {
            preferences.clear();
            preferences.end();
        }
        savedCredentialsAvailable = false;
    }

    bool startScan()
    {
        const int state = WiFi.scanComplete();
        if (state == WIFI_SCAN_RUNNING) return false;
        WiFi.scanDelete();
        return WiFi.scanNetworks(true, false) == WIFI_SCAN_RUNNING;
    }

    int scanComplete()
    {
        return WiFi.scanComplete();
    }

    size_t takeScanResults(ScanResult *results, size_t maximumResults)
    {
        const int count = WiFi.scanComplete();
        if (results == nullptr || maximumResults == 0 || count < 0) {
            return 0;
        }

        size_t written = 0;
        for (int index = 0; index < count && written < maximumResults; ++index) {
            const String ssid = WiFi.SSID(index);
            if (ssid.isEmpty()) continue;

            bool duplicate = false;
            for (size_t existing = 0; existing < written; ++existing) {
                if (ssid == results[existing].ssid) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            copyText(
                results[written].ssid,
                sizeof(results[written].ssid),
                ssid.c_str());
            results[written].rssiDbm =
                static_cast<int16_t>(WiFi.RSSI(index));
            results[written].secured =
                WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
            ++written;
        }
        WiFi.scanDelete();
        return written;
    }

    void setUpdateCallbacks(
        UpdateCallback startCallback,
        UpdateCallback finishCallback)
    {
        updateStartCallback = startCallback;
        updateFinishCallback = finishCallback;
    }

    bool updateInProgress()
    {
        return updating;
    }
}
