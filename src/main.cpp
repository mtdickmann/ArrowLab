/*
 * ArrowLab firmware
 * Development version 0.1
 *
 * Dual HX711 zero-adjusted raw-count test
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "measurement/LoadCellChannel.h"
#include "ui/ui.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace
{
    constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 100;
    constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;
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
    LoadCellChannel leftSensor("Left", 13, 10);
    LoadCellChannel rightSensor("Right", 11, 12);

    Board *displayBoard = nullptr;

    uint32_t lastSensorUpdate = 0;
    volatile bool leftTareRequested = false;
    volatile bool rightTareRequested = false;

    void requestTare(ArrowLabUI::LoadSide side)
    {
        if (side == ArrowLabUI::LoadSide::Left) {
            leftTareRequested = true;
        } else {
            rightTareRequested = true;
        }
    }

    void startRequestedTares()
    {
        if (leftTareRequested) {
            leftSensor.startUserTare();
            leftTareRequested = false;

            Serial.println(
                "Manual LEFT tare confirmed"
            );
        }

        if (rightTareRequested) {
            rightSensor.startUserTare();
            rightTareRequested = false;

            Serial.println(
                "Manual RIGHT tare confirmed"
            );
        }
    }

    void formatReading(
        char *buffer,
        size_t bufferSize,
        const LoadCellChannel &sensor,
        bool live
    )
    {
        if (!live) {
            snprintf(buffer, bufferSize, "---");
            return;
        }

        if (!sensor.tareComplete()) {
            snprintf(buffer, bufferSize, "TARE");
            return;
        }

        snprintf(
            buffer,
            bufferSize,
            "%ld",
            sensor.zeroedValue()
        );
    }

    void updateDisplay(uint32_t currentTime)
    {
        const bool leftLive =
            leftSensor.isLive(currentTime, SENSOR_TIMEOUT_MS);

        const bool rightLive =
            rightSensor.isLive(currentTime, SENSOR_TIMEOUT_MS);

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
            (leftLive && !leftSensor.tareComplete())
            || (rightLive && !rightSensor.tareComplete());

        lvgl_port_lock(-1);

        ArrowLabUI::setLeftReading(leftText);
        ArrowLabUI::setRightReading(rightText);

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Left,
            leftSensor.tareComplete(),
            leftSensor.userTareConfirmed(),
            leftSensor.calibrated()
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Right,
            rightSensor.tareComplete(),
            rightSensor.userTareConfirmed(),
            rightSensor.calibrated()
        );

        if (tareInProgress) {
            ArrowLabUI::setStatus(
                "Taring load cell - keep setup stable"
            );

            ArrowLabUI::setState(
                "TARING",
                lv_color_hex(0xFFB020)
            );
        } else if (leftLive && rightLive) {
            if (
                leftSensor.userTareConfirmed()
                && rightSensor.userTareConfirmed()
            ) {
                ArrowLabUI::setStatus(
                    "Both loads ready for calibration"
                );
            } else if (leftSensor.userTareConfirmed()) {
                ArrowLabUI::setStatus(
                    "Tare RIGHT before calibration"
                );
            } else if (rightSensor.userTareConfirmed()) {
                ArrowLabUI::setStatus(
                    "Tare LEFT before calibration"
                );
            } else {
                ArrowLabUI::setStatus(
                    "Tare each load before calibration"
                );
            }

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

    leftSensor.begin();
    rightSensor.begin();

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

    if (
        leftTareRequested
        || rightTareRequested
    ) {
        startRequestedTares();
    }

    /*
     * Each channel is tested independently.
     * One missing sensor cannot prevent the other from working.
     */
    leftSensor.read(now);
    rightSensor.read(now);

    updateDisplay(now);

    delay(5);
}
