/*
 * ArrowLab firmware
 * Development firmware; version is defined only in Version.h.
 *
 * Top-level hardware, diagnostics and UI coordinator.
 * Tare/calibration workflow lives in CalibrationController.
 */

#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "measurement/LoadCellChannel.h"
#include "measurement/MeasurementChannel.h"
#include "calibration/CalibrationController.h"
#include "diagnostics/CreepDiagnostic.h"
#include "storage/InstrumentStorage.h"
#include "ui/ui.h"
#include "Version.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace
{
    constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 100;
    constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;
    float calibrationReferenceGrams = 0.0f;
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
    MeasurementChannel leftMeasurement;
    MeasurementChannel rightMeasurement;
    CreepDiagnostic creepDiagnostic;
    InstrumentStorage instrumentStorage;
    CalibrationController calibrationController(
        leftMeasurement,
        rightMeasurement,
        instrumentStorage);

    Board *displayBoard = nullptr;

    uint32_t lastSensorUpdate = 0;
    volatile bool diagnosticStartRequested = false;
    volatile bool diagnosticCancelRequested = false;
    volatile bool diagnosticFinishRequested = false;
    ArrowLabUI::LoadSide diagnosticRequestedSide =
        ArrowLabUI::LoadSide::Left;
    float diagnosticRequestedMassGrams = 0.0f;
    bool diagnosticRequestedZeroBaseline = false;
    char serialLine[128];
    size_t serialLineLength = 0;

    CalibrationSide calibrationSide(ArrowLabUI::LoadSide side)
    {
        return side == ArrowLabUI::LoadSide::Left
            ? CalibrationSide::Left
            : CalibrationSide::Right;
    }

    void requestTare(ArrowLabUI::LoadSide side)
    {
        calibrationController.requestTare(calibrationSide(side));
    }

    void requestCalibration(
        ArrowLabUI::LoadSide side,
        float referenceGrams)
    {
        if (referenceGrams > 0.0f) {
            calibrationReferenceGrams = referenceGrams;
            ArrowLabUI::setCalibrationReferenceGrams(referenceGrams);
        }

        calibrationController.requestCalibration(
            calibrationSide(side),
            referenceGrams);
    }

    void requestDiagnosticStart(
        ArrowLabUI::LoadSide side,
        float testMassGrams,
        bool zeroBaseline)
    {
        diagnosticRequestedSide = side;
        diagnosticRequestedMassGrams = testMassGrams;
        diagnosticRequestedZeroBaseline = zeroBaseline;
        diagnosticStartRequested = true;
    }

    void requestDiagnosticCancel()
    {
        diagnosticCancelRequested = true;
    }

    void requestDiagnosticFinish()
    {
        diagnosticFinishRequested = true;
    }

    void processSerialInput(uint32_t currentTime)
    {
        while (Serial.available() > 0) {
            const char value = static_cast<char>(Serial.read());

            if (value == '\r') {
                continue;
            }

            if (value == '\n') {
                if (serialLineLength > 0) {
                    serialLine[serialLineLength] = '\0';
                    creepDiagnostic.handleHostCommand(
                        serialLine,
                        currentTime
                    );
                    serialLineLength = 0;
                }
                continue;
            }

            if (serialLineLength < sizeof(serialLine) - 1) {
                serialLine[serialLineLength++] = value;
            } else {
                serialLineLength = 0;
            }
        }
    }

    void processDiagnosticRequests(uint32_t currentTime)
    {
        if (diagnosticCancelRequested) {
            diagnosticCancelRequested = false;
            diagnosticStartRequested = false;
            creepDiagnostic.cancel();
        }

        if (diagnosticFinishRequested) {
            diagnosticFinishRequested = false;

            if (!creepDiagnostic.finishSession()) {
                Serial.println(
                    "AL_DIAG,EVENT,FINISH_REJECTED"
                );
            }
        }

        if (!diagnosticStartRequested) {
            return;
        }

        diagnosticStartRequested = false;

        const DiagnosticSide side =
            diagnosticRequestedSide
                    == ArrowLabUI::LoadSide::Left
                ? DiagnosticSide::Left
                : DiagnosticSide::Right;

        if (!creepDiagnostic.start(
                side,
                diagnosticRequestedMassGrams,
                diagnosticRequestedZeroBaseline,
                currentTime
            )) {
            Serial.println(
                "AL_DIAG,EVENT,START_REJECTED"
            );
        }
    }

    void formatReading(
        char *buffer,
        size_t bufferSize,
        const MeasurementChannel &measurement,
        bool live
    )
    {
        if (!live) {
            snprintf(buffer, bufferSize, "---");
            return;
        }

        if (!measurement.tareComplete()) {
            snprintf(buffer, bufferSize, "TARE");
            return;
        }

        if (measurement.calibrated()) {
            snprintf(
                buffer,
                bufferSize,
                "%.1f",
                measurement.heldGrams()
            );
            return;
        }

        snprintf(
            buffer,
            bufferSize,
            "%ld",
            measurement.heldRawCounts()
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
            leftMeasurement,
            leftLive
        );

        formatReading(
            rightText,
            sizeof(rightText),
            rightMeasurement,
            rightLive
        );

        const CalibrationController::ChannelStatus leftCalibration =
            calibrationController.status(CalibrationSide::Left, currentTime);
        const CalibrationController::ChannelStatus rightCalibration =
            calibrationController.status(CalibrationSide::Right, currentTime);

        const auto setupActive = [](CalibrationController::Stage stage) {
            return stage == CalibrationController::Stage::AwaitingLoad
                || stage == CalibrationController::Stage::ReadyToCalibrate
                || stage == CalibrationController::Stage::Settling
                || stage == CalibrationController::Stage::Sampling;
        };
        const auto calibrationBusy = [](CalibrationController::Stage stage) {
            return stage == CalibrationController::Stage::Settling
                || stage == CalibrationController::Stage::Sampling;
        };
        const bool tareInProgress =
            leftCalibration.stage == CalibrationController::Stage::Taring
            || rightCalibration.stage == CalibrationController::Stage::Taring;

        lvgl_port_lock(-1);

        char diagnosticStatus[96];
        const CreepDiagnostic::State diagnosticState =
            creepDiagnostic.state();
        const char *diagnosticSideText =
            creepDiagnostic.side() == DiagnosticSide::Left
                ? "LEFT"
                : "RIGHT";
        const bool diagnosticActive =
            diagnosticState == CreepDiagnostic::State::WaitingForHost
            ||
            diagnosticState == CreepDiagnostic::State::CapturingReference
            || diagnosticState == CreepDiagnostic::State::AwaitingLoad
            || diagnosticState == CreepDiagnostic::State::Running;

        switch (diagnosticState) {
        case CreepDiagnostic::State::WaitingForHost:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "NEXT: Start PC logger and wait for CONNECTED");
            break;

        case CreepDiagnostic::State::CapturingReference:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "WAIT: Capturing %s raw reference",
                diagnosticSideText);
            break;

        case CreepDiagnostic::State::AwaitingLoad:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "NEXT: Place %.3f g centrally on %s",
                creepDiagnostic.testMassGrams(),
                diagnosticSideText);
            break;

        case CreepDiagnostic::State::Running: {
            const uint32_t elapsedSeconds =
                creepDiagnostic.elapsedMs(currentTime) / 1000;
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "DO NOT TOUCH: %s %.3f g - %02lu:%02lu / 30:00",
                diagnosticSideText,
                creepDiagnostic.testMassGrams(),
                static_cast<unsigned long>(
                    elapsedSeconds / 60),
                static_cast<unsigned long>(
                    elapsedSeconds % 60));
            break;
        }

        case CreepDiagnostic::State::AwaitingSave:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "WAIT: Saving and verifying the complete run");
            break;

        case CreepDiagnostic::State::Complete:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "NEXT: Remove test weight; continue or press DONE");
            break;

        case CreepDiagnostic::State::Idle:
        default:
            diagnosticStatus[0] = '\0';
            break;
        }

        ArrowLabUI::setDiagnosticStatus(
            diagnosticStatus,
            creepDiagnostic.progressPercent(currentTime),
            diagnosticActive,
            creepDiagnostic.awaitingSave()
        );
        ArrowLabUI::setDiagnosticHostConnected(
            creepDiagnostic.hostConnected(currentTime));

        ArrowLabUI::setLeftReading(leftText);
        ArrowLabUI::setRightReading(rightText);
        ArrowLabUI::setSensorHealth(leftLive, rightLive);
        ArrowLabUI::setCalibrationValidity(
            leftMeasurement.calibrated(),
            rightMeasurement.calibrated()
        );

        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Left,
            leftMeasurement.calibrated() ? "g" : "RAW"
        );
        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Right,
            rightMeasurement.calibrated() ? "g" : "RAW"
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Left,
            leftMeasurement.tareComplete(),
            leftCalibration.stage == CalibrationController::Stage::Taring,
            leftMeasurement.userTareConfirmed(),
            leftCalibration.stage
                == CalibrationController::Stage::ReadyToCalibrate,
            calibrationBusy(leftCalibration.stage),
            leftMeasurement.calibrated(),
            setupActive(leftCalibration.stage),
            leftCalibration.settleRemainingSeconds,
            leftCalibration.settlePercent
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Right,
            rightMeasurement.tareComplete(),
            rightCalibration.stage == CalibrationController::Stage::Taring,
            rightMeasurement.userTareConfirmed(),
            rightCalibration.stage
                == CalibrationController::Stage::ReadyToCalibrate,
            calibrationBusy(rightCalibration.stage),
            rightMeasurement.calibrated(),
            setupActive(rightCalibration.stage),
            rightCalibration.settleRemainingSeconds,
            rightCalibration.settlePercent
        );

        const bool calibrationInProgress =
            calibrationBusy(leftCalibration.stage)
            || calibrationBusy(rightCalibration.stage);

        if (tareInProgress) {
            ArrowLabUI::setStatus(
                "Taring load cell - keep setup stable"
            );

            ArrowLabUI::setState(
                "TARING",
                lv_color_hex(0xFFB020)
            );
        } else if (calibrationInProgress) {
            ArrowLabUI::setStatus(
                calibrationBusy(leftCalibration.stage)
                    ? "Calibrating LEFT - keep weight stable"
                    : "Calibrating RIGHT - keep weight stable"
            );

            ArrowLabUI::setState(
                "CALIBRATING",
                lv_color_hex(0xFFB020)
            );
        } else if (leftLive && rightLive) {
            if (
                leftCalibration.stage
                    == CalibrationController::Stage::AwaitingLoad
            ) {
                char nextAction[72];
                snprintf(
                    nextAction,
                    sizeof(nextAction),
                    "NEXT: Place %.1f g calibration weight on LEFT",
                    leftCalibration.referenceGrams);
                ArrowLabUI::setStatus(nextAction);
            } else if (
                rightCalibration.stage
                    == CalibrationController::Stage::AwaitingLoad
            ) {
                char nextAction[72];
                snprintf(
                    nextAction,
                    sizeof(nextAction),
                    "NEXT: Place %.1f g calibration weight on RIGHT",
                    rightCalibration.referenceGrams);
                ArrowLabUI::setStatus(nextAction);
            } else if (
                leftCalibration.stage
                    == CalibrationController::Stage::ReadyToCalibrate
            ) {
                ArrowLabUI::setStatus(
                    "NEXT: Press LEFT CAL to start 30 s stabilization");
            } else if (
                rightCalibration.stage
                    == CalibrationController::Stage::ReadyToCalibrate
            ) {
                ArrowLabUI::setStatus(
                    "NEXT: Press RIGHT CAL to start 30 s stabilization");
            } else if (
                leftMeasurement.calibrated()
                && rightMeasurement.calibrated()
            ) {
                ArrowLabUI::setStatus(
                    "Both load channels calibrated"
                );
            } else if (
                leftMeasurement.userTareConfirmed()
                && rightMeasurement.userTareConfirmed()
            ) {
                if (leftMeasurement.calibrated()) {
                    ArrowLabUI::setStatus(
                        "Calibrate RIGHT with reference weight"
                    );
                } else if (rightMeasurement.calibrated()) {
                    ArrowLabUI::setStatus(
                        "Calibrate LEFT with reference weight"
                    );
                } else {
                    ArrowLabUI::setStatus(
                        "Both loads ready for calibration"
                    );
                }
            } else if (leftMeasurement.userTareConfirmed()) {
                ArrowLabUI::setStatus(
                    "Tare RIGHT before calibration"
                );
            } else if (rightMeasurement.userTareConfirmed()) {
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
    Serial.printf(
        "%s v%u.%u.%u %s starting\n",
        Version::PROJECT_NAME,
        Version::MAJOR,
        Version::MINOR,
        Version::PATCH,
        Version::STATUS
    );

    if (!instrumentStorage.begin()) {
        Serial.println("WARNING: persistent instrument storage unavailable");
    }

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
    ArrowLabUI::setCalibrationCallback(
        requestCalibration
    );
    ArrowLabUI::setDiagnosticCallbacks(
        requestDiagnosticStart,
        requestDiagnosticCancel,
        requestDiagnosticFinish
    );
    ArrowLabUI::setCalibrationReferenceGrams(
        calibrationReferenceGrams
    );
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
    calibrationController.begin();

    Serial.println(
        "Dual HX711 initialization complete - awaiting deliberate tare"
    );
}

void loop()
{
    const uint32_t now = millis();

    // Serial commands must be serviced even between HX711 UI refreshes so
    // heartbeat/ACK traffic cannot be starved by the 100 ms sensor cadence.
    processSerialInput(now);

    if (
        now - lastSensorUpdate
        < SENSOR_UPDATE_INTERVAL_MS
    ) {
        delay(5);
        return;
    }

    lastSensorUpdate = now;

    if (
        diagnosticStartRequested
        || diagnosticCancelRequested
        || diagnosticFinishRequested
    ) {
        processDiagnosticRequests(now);
    }

    /*
     * Each channel is tested independently.
     * One missing sensor cannot prevent the other from working.
     */
    const bool leftFresh = leftSensor.read(now);

    if (leftFresh) {
        leftMeasurement.onRawSample(leftSensor.rawValue(), now);
        calibrationController.onFreshReading(CalibrationSide::Left);
    }

    const bool rightFresh = rightSensor.read(now);

    if (rightFresh) {
        rightMeasurement.onRawSample(rightSensor.rawValue(), now);
        calibrationController.onFreshReading(CalibrationSide::Right);
    }

    calibrationController.update(now);

    creepDiagnostic.update(
        now,
        leftFresh,
        rightFresh,
        leftSensor,
        rightSensor
    );

    updateDisplay(now);

    delay(5);
}
