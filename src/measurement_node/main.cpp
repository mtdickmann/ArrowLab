#include <Arduino.h>
#include <HX711.h>

#include <cstring>

#include "Version.h"

namespace
{
    constexpr uint8_t LEFT_DT_PIN = 4;
    constexpr uint8_t LEFT_SCK_PIN = 5;
    constexpr uint8_t RIGHT_DT_PIN = 6;
    constexpr uint8_t RIGHT_SCK_PIN = 7;

    struct RawChannel
    {
        RawChannel(const char *channelName, uint8_t dataPin, uint8_t clockPin)
            : name(channelName), dtPin(dataPin), sckPin(clockPin)
        {
        }

        const char *name;
        uint8_t dtPin;
        uint8_t sckPin;
        HX711 hx711;
        long latestRaw = 0;
        long baselineRaw = 0;
        bool hasReading = false;
        bool baselineValid = false;
    };

    RawChannel leftChannel("LEFT", LEFT_DT_PIN, LEFT_SCK_PIN);
    RawChannel rightChannel("RIGHT", RIGHT_DT_PIN, RIGHT_SCK_PIN);

    char serialCommand[24];
    size_t serialCommandLength = 0;

    void printVersion()
    {
        Serial.printf(
            "%s v%u.%u.%u %s - Measurement Node raw diagnostic\n",
            Version::PROJECT_NAME,
            Version::MAJOR,
            Version::MINOR,
            Version::PATCH,
            Version::STATUS
        );
    }

    void printHelp()
    {
        Serial.println("Commands:");
        Serial.println("  ?   - show this help");
        Serial.println("  BL  - set LEFT baseline to latest raw reading");
        Serial.println("  BR  - set RIGHT baseline to latest raw reading");
        Serial.println("  BB  - set BOTH baselines to latest raw readings");
        Serial.println("  RL  - clear LEFT baseline; next sample becomes baseline");
        Serial.println("  RR  - clear RIGHT baseline; next sample becomes baseline");
        Serial.println("  RB  - clear BOTH baselines; next samples become baselines");
        Serial.println();
        Serial.println("CSV: AL_NODE,DATA,millis,side,raw,delta");
    }

    void beginChannel(RawChannel &channel)
    {
        channel.hx711.begin(channel.dtPin, channel.sckPin);

        // Prevent a disconnected DT lead floating low and impersonating an
        // HX711, matching the proven ArrowLab display-side behaviour.
        pinMode(channel.dtPin, INPUT_PULLUP);

        Serial.printf(
            "AL_NODE,CONFIG,%s,DT=%u,SCK=%u\n",
            channel.name,
            channel.dtPin,
            channel.sckPin
        );
    }

    void captureBaseline(RawChannel &channel)
    {
        if (!channel.hasReading) {
            Serial.printf("AL_NODE,EVENT,%s,NO_READING\n", channel.name);
            return;
        }

        channel.baselineRaw = channel.latestRaw;
        channel.baselineValid = true;
        Serial.printf(
            "AL_NODE,EVENT,%s,BASELINE,%ld\n",
            channel.name,
            channel.baselineRaw
        );
    }

    void resetBaseline(RawChannel &channel)
    {
        channel.baselineValid = false;
        Serial.printf("AL_NODE,EVENT,%s,BASELINE_CLEARED\n", channel.name);
    }

    void processCommand(const char *command)
    {
        if (strcmp(command, "?") == 0) {
            printHelp();
        } else if (strcmp(command, "BL") == 0) {
            captureBaseline(leftChannel);
        } else if (strcmp(command, "BR") == 0) {
            captureBaseline(rightChannel);
        } else if (strcmp(command, "BB") == 0) {
            captureBaseline(leftChannel);
            captureBaseline(rightChannel);
        } else if (strcmp(command, "RL") == 0) {
            resetBaseline(leftChannel);
        } else if (strcmp(command, "RR") == 0) {
            resetBaseline(rightChannel);
        } else if (strcmp(command, "RB") == 0) {
            resetBaseline(leftChannel);
            resetBaseline(rightChannel);
        } else if (command[0] != '\0') {
            Serial.printf("AL_NODE,EVENT,UNKNOWN_COMMAND,%s\n", command);
        }
    }

    void processSerialCommands()
    {
        while (Serial.available() > 0) {
            const char value = static_cast<char>(Serial.read());

            if (value == '\r') {
                continue;
            }

            if (value == '\n') {
                serialCommand[serialCommandLength] = '\0';
                processCommand(serialCommand);
                serialCommandLength = 0;
                continue;
            }

            if (serialCommandLength < sizeof(serialCommand) - 1) {
                serialCommand[serialCommandLength++] = value;
            } else {
                serialCommandLength = 0;
            }
        }
    }

    void sampleChannel(RawChannel &channel, uint32_t now)
    {
        if (!channel.hx711.is_ready()) {
            return;
        }

        channel.latestRaw = channel.hx711.read();
        channel.hasReading = true;

        if (!channel.baselineValid) {
            channel.baselineRaw = channel.latestRaw;
            channel.baselineValid = true;
            Serial.printf(
                "AL_NODE,EVENT,%s,AUTO_BASELINE,%ld\n",
                channel.name,
                channel.baselineRaw
            );
        }

        const long delta = channel.latestRaw - channel.baselineRaw;
        Serial.printf(
            "AL_NODE,DATA,%lu,%s,%ld,%ld\n",
            static_cast<unsigned long>(now),
            channel.name,
            channel.latestRaw,
            delta
        );
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    printVersion();
    Serial.println("Internal Affairs mode: RAW HX711 counts only.");
    beginChannel(leftChannel);
    beginChannel(rightChannel);
    printHelp();
}

void loop()
{
    const uint32_t now = millis();

    processSerialCommands();
    sampleChannel(leftChannel, now);
    sampleChannel(rightChannel, now);

    delay(1);
}
