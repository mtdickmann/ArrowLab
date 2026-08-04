/*
 * ArrowLab firmware
 * Development version 0.1
 *
 * Dual HX711 raw-count test
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

    struct SensorChannel
    {
        HX711 hx711;
        const char *name;
        uint8_t dtPin;
        uint8_t sckPin;

        long rawValue = 0;
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

        Serial.printf(
            "%s raw: %ld\n",
            sensor.name,
            sensor.rawValue
        );

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

    void updateDisplay(uint32_t currentTime)
    {
        const bool leftLive =
            sensorIsLive(leftSensor, currentTime);

        const bool rightLive =
            sensorIsLive(rightSensor, currentTime);

        char leftText[24];
        char rightText[24];

        if (leftLive) {
            snprintf(
                leftText,
                sizeof(leftText),
                "%ld",
                leftSensor.rawValue
            );
        } else {
            snprintf(
                leftText,
                sizeof(leftText),
                "---"
            );
        }

        if (rightLive) {
            snprintf(
                rightText,
                sizeof(rightText),
                "%ld",
                rightSensor.rawValue
            );
        } else {
            snprintf(
                rightText,
                sizeof(rightText),
                "---"
            );
        }

        lvgl_port_lock(-1);

        ArrowLabUI::setLeftReading(leftText);
        ArrowLabUI::setRightReading(rightText);

        if (leftLive && rightLive) {
            ArrowLabUI::setStatus(
                "Receiving both load-cell channels"
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

    Serial.println("Dual HX711 initialization complete");
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

    /*
     * Each channel is tested independently.
     * One missing sensor cannot prevent the other from working.
     */
    readSensor(leftSensor, now);
    readSensor(rightSensor, now);

    updateDisplay(now);

    delay(5);
}