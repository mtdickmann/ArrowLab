#include "network/NetworkService.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <cstdio>
#include <cstring>
#include <WiFi.h>

#if __has_include("NetworkSecrets.h")
#include "NetworkSecrets.h"
#define ARROWLAB_HAS_NETWORK_SECRETS 1
#else
#define ARROWLAB_HAS_NETWORK_SECRETS 0
#endif

namespace
{
    constexpr uint32_t CONNECTION_LOG_INTERVAL_MS = 10000;
    bool started = false;
    bool otaStarted = false;
    uint32_t lastConnectionLog = 0;
    const char *deviceHostname = "arrowlab";

    void startOta()
    {
#if ARROWLAB_HAS_NETWORK_SECRETS
        ArduinoOTA.setHostname(deviceHostname);

        if (ARROWLAB_OTA_PASSWORD[0] != '\0') {
            ArduinoOTA.setPassword(ARROWLAB_OTA_PASSWORD);
        }

        ArduinoOTA.onStart([]() {
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
            Serial.printf("AL_NET,OTA,ERROR,%u\n", error);
        });
        ArduinoOTA.begin();
        otaStarted = true;
        Serial.printf(
            "AL_NET,OTA,READY,%s.local,%s\n",
            deviceHostname,
            WiFi.localIP().toString().c_str());
#endif
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

#if ARROWLAB_HAS_NETWORK_SECRETS
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(deviceHostname);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);
        WiFi.begin(ARROWLAB_WIFI_SSID, ARROWLAB_WIFI_PASSWORD);
        Serial.printf(
            "AL_NET,WIFI,CONNECTING,%s,%s\n",
            deviceHostname,
            ARROWLAB_WIFI_SSID);
#else
        Serial.println(
            "AL_NET,DISABLED,create include/NetworkSecrets.h from the example");
#endif
    }

    void handle()
    {
#if ARROWLAB_HAS_NETWORK_SECRETS
        if (!started) return;

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
#endif
    }

    bool configured()
    {
        return ARROWLAB_HAS_NETWORK_SECRETS != 0;
    }

    bool connected()
    {
#if ARROWLAB_HAS_NETWORK_SECRETS
        return WiFi.status() == WL_CONNECTED;
#else
        return false;
#endif
    }

    Info info()
    {
        Info result;
        result.configured = configured();
        result.connected = connected();
        snprintf(
            result.hostname,
            sizeof(result.hostname),
            "%s",
            deviceHostname);

#if ARROWLAB_HAS_NETWORK_SECRETS
        snprintf(
            result.mac,
            sizeof(result.mac),
            "%s",
            WiFi.macAddress().c_str());

        if (result.connected) {
            snprintf(
                result.ssid,
                sizeof(result.ssid),
                "%s",
                WiFi.SSID().c_str());
            snprintf(
                result.ip,
                sizeof(result.ip),
                "%s",
                WiFi.localIP().toString().c_str());
            result.rssiDbm = static_cast<int16_t>(WiFi.RSSI());
        }
#endif
        return result;
    }
}
