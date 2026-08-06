/*
 * ArrowLab firmware
 * Development version 0.1
 *
 * Dual HX711 zero-adjusted raw-count test
 */

#include <Arduino.h>
#include <HX711.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "ui/ui.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace
{
    constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 100;
    constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;
    constexpr uint8_t TARE_SAMPLE_COUNT = 20;

    struct SensorChannel
    {
        HX711 hx711;
        const char *name;
        uint8_t dtPin;
        uint8_t sckPin;

        long rawValue = 0;
        long tareOffset = 0;
        long zeroedValue = 0;

        int64_t tareAccumulator = 0;
        uint8_t tareSamples = 0;
        bool tareComplete = false;

        bool hasReading = false;
        uint32_t lastReadingTime = 0;

        SensorChannel(
            const char *sensorName,
            uint8_t dataPin,
            uint8_t clockPin
        )
            : name(sensorName),
              dtPin(dataPin),
              sckPin(clockPin)
        {
        }
    };

    /*
     * Final GPIO assignment:
     *
     * Left HX711:
     *   DT  = GPIO13
     *   SCK = GPIO10
     *
     * Right HX711:
     *   DT  = GPIO11
     *   SCK = GPIO12
     */
    SensorChannel leftSensor("Left", 13, 10);
    SensorChannel rightSensor("Right", 11, 12);

    Board *displayBoard = nullptr;

    uint32_t lastSensorUpdate = 0;
    volatile bool tareRequested = false;

    void beginSensor(SensorChannel &sensor)
    {
        Serial.printf(
            "Initializing %s HX711: DT=%u, SCK=%u\n",
            sensor.name,
            sensor.dtPin,
            sensor.sckPin
        );

        sensor.hx711.begin(
            sensor.dtPin,
            sensor.sckPin
        );

        /*
         * Prevent a disconnected DT input from floating low and
         * falsely appearing as a ready HX711.
         */
        pinMode(sensor.dtPin, INPUT_PULLUP);
    }

    void resetTare(SensorChannel &sensor)
    {
        sensor.tareOffset = 0;
        sensor.zeroedValue = 0;
        sensor.tareAccumulator = 0;
        sensor.tareSamples = 0;
        sensor.tareComplete = false;
    }

    void requestTare()
    {
        tareRequested = true;
    }

    void startRequestedTare()
    {
        resetTare(leftSensor);
        resetTare(rightSensor);
        tareRequested = false;

        Serial.println(
            "Manual tare requested - zeroing both channels"
        );
    }

    void updateTare(SensorChannel &sensor)
    {
        if (sensor.tareComplete) {
            sensor.zeroedValue =
                sensor.rawValue - sensor.tareOffset;
            return;
        }

        sensor.tareAccumulator += sensor.rawValue;
        sensor.tareSamples++;

        if (sensor.tareSamples < TARE_SAMPLE_COUNT) {
            return;
        }

        sensor.tareOffset = static_cast<long>(
            sensor.tareAccumulator / TARE_SAMPLE_COUNT
        );

        sensor.zeroedValue = 0;
        sensor.tareComplete = true;

        Serial.printf(
            "%s tare complete: offset=%ld (%u samples)\n",
            sensor.name,
            sensor.tareOffset,
            TARE_SAMPLE_COUNT
        );
    }

    bool readSensor(
        SensorChannel &sensor,
        uint32_t currentTime
    )
    {
        /*
         * A real HX711 briefly becomes not-ready between conversions.
         * Therefore, not-ready is not immediately treated as an error.
         */
        if (!sensor.hx711.is_ready()) {
            return false;
        }

        sensor.rawValue = sensor.hx711.read();
        sensor.hasReading = true;
        sensor.lastReadingTime = currentTime;

        updateTare(sensor);

        if (sensor.tareComplete) {
            Serial.printf(
                "%s raw: %ld  zeroed: %ld\n",
                sensor.name,
                sensor.rawValue,
                sensor.zeroedValue
            );
        } else {
            Serial.printf(
                "%s raw: %ld  taring: %u/%u\n",
                sensor.name,
                sensor.rawValue,
                sensor.tareSamples,
                TARE_SAMPLE_COUNT
            );
        }

        return true;
    }

    bool sensorIsLive(
        const SensorChannel &sensor,
        uint32_t currentTime
    )
    {
        if (!sensor.hasReading) {
            return false;
        }

        return (
            currentTime - sensor.lastReadingTime
            <= SENSOR_TIMEOUT_MS
        );
    }

    void formatReading(
        char *buffer,
        size_t bufferSize,
        const SensorChannel &sensor,
        bool live
    )
    {
        if (!live) {
            snprintf(buffer, bufferSize, "---");
            return;
        }

        if (!sensor.tareComplete) {
            snprintf(buffer, bufferSize, "TARE");
            return;
        }

        snprintf(
            buffer,
            bufferSize,
            "%ld",
            sensor.zeroedValue
        );
    }

    void updateDisplay(uint32_t currentTime)
    {
        const bool leftLive =
            sensorIsLive(leftSensor, currentTime);

        const bool rightLive =
            sensorIsLive(rightSensor, currentTime);

        char leftText[24];
        char rightText[24];

        formatReading(
            leftText,
            sizeof(leftText),
            leftSensor,
            leftLive
        );

        formatReading(
            rightText,
            sizeof(rightText),
            rightSensor,
            rightLive
        );

        const bool tareInProgress =
            (leftLive && !leftSensor.tareComplete)
            || (rightLive && !rightSensor.tareComplete);

        lvgl_port_lock(-1);

        ArrowLabUI::setLeftReading(leftText);
        ArrowLabUI::setRightReading(rightText);

        if (tareInProgress) {
            ArrowLabUI::setStatus(
                "Zeroing load cells - keep unloaded"
            );

            ArrowLabUI::setState(
                "TARING",
                lv_color_hex(0xFFB020)
            );
        } else if (leftLive && rightLive) {
            ArrowLabUI::setStatus(
                "Both load-cell channels zeroed"
            );

            ArrowLabUI::setState(
                "DUAL LIVE",
                lv_color_hex(0x4CD964)
            );
        } else if (leftLive) {
            ArrowLabUI::setStatus(
                "Left live - waiting for right HX711"
            );

            ArrowLabUI::setState(
                "LEFT LIVE",
                lv_color_hex(0xFFB020)
            );
        } else if (rightLive) {
            ArrowLabUI::setStatus(
                "Right live - waiting for left HX711"
            );

            ArrowLabUI::setState(
                "RIGHT LIVE",
                lv_color_hex(0xFFB020)
            );
        } else {
            ArrowLabUI::setStatus(
                "Waiting for HX711 data"
            );

            ArrowLabUI::setState(
                "NO DATA",
                lv_color_hex(0xFF4D4D)
            );
        }

        lvgl_port_unlock();
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("ArrowLab v0.1 starting");

    displayBoard = new Board();
    displayBoard->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = displayBoard->getLCD();
    lcd->configFrameBufferNumber(
        LVGL_PORT_DISP_BUFFER_NUM
    );

#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcdBus = lcd->getBus();

    if (
        lcdBus->getBasicAttributes().type
        == ESP_PANEL_BUS_TYPE_RGB
    ) {
        static_cast<BusRGB *>(lcdBus)
            ->configRGB_BounceBufferSize(
                lcd->getFrameWidth() * 10
            );
    }
#endif
#endif

    assert(displayBoard->begin());

    Serial.println("Initializing LVGL");

    lvgl_port_init(
        displayBoard->getLCD(),
        displayBoard->getTouch()
    );

    lvgl_port_lock(-1);

    ArrowLabUI::create();
    ArrowLabUI::setTareCallback(requestTare);
    ArrowLabUI::setLeftReading("---");
    ArrowLabUI::setRightReading("---");
    ArrowLabUI::setStatus(
        "Starting dual HX711 system"
    );

    ArrowLabUI::setState(
        "INITIALIZING",
        lv_color_hex(0xFFB020)
    );

    lvgl_port_unlock();

    beginSensor(leftSensor);
    beginSensor(rightSensor);

    Serial.println(
        "Dual HX711 initialization complete - starting independent tare"
    );
}

void loop()
{
    const uint32_t now = millis();

    if (
        now - lastSensorUpdate
        < SENSOR_UPDATE_INTERVAL_MS
    ) {
        delay(5);
        return;
    }

    lastSensorUpdate = now;

    if (tareRequested) {
        startRequestedTare();
    }

    /*
     * Each channel is tested independently.
     * One missing sensor cannot prevent the other from working.
     */
    readSensor(leftSensor, now);
    readSensor(rightSensor, now);

    updateDisplay(now);

    delay(5);
}
