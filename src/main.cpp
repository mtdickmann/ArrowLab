/*
 * ArrowLab firmware
 * Development firmware; version is defined only in Version.h.
 *
 * Dual HX711 zero-adjusted raw-count test
 */

#include <Arduino.h>
#include <cstdio>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "measurement/LoadCellChannel.h"
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
    constexpr uint32_t CALIBRATION_SETTLE_TIME_MS = 30000;

    /*
     * Reference-load detection is intentionally far above observed
     * tare noise, but below the expected signal from a 500 g minimum
     * calibration mass so channel sensitivity can vary safely.
     */
    constexpr long CALIBRATION_LOAD_THRESHOLD_COUNTS = 250000;

    // Default for the current development reference weight.
    // This becomes user-editable from the calibration UI later.
    float calibrationReferenceGrams = 999.8f;
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
    CreepDiagnostic creepDiagnostic;
    InstrumentStorage instrumentStorage;

    Board *displayBoard = nullptr;

    uint32_t lastSensorUpdate = 0;
    volatile bool leftTareRequested = false;
    volatile bool rightTareRequested = false;
    volatile bool leftCalibrationRequested = false;
    volatile bool rightCalibrationRequested = false;
    volatile bool diagnosticStartRequested = false;
    volatile bool diagnosticCancelRequested = false;
    volatile bool diagnosticFinishRequested = false;
    ArrowLabUI::LoadSide diagnosticRequestedSide =
        ArrowLabUI::LoadSide::Left;
    float diagnosticRequestedMassGrams = 0.0f;
    bool diagnosticRequestedZeroBaseline = false;
    volatile bool diagnosticResetChannelRequested = false;
    ArrowLabUI::LoadSide diagnosticMaintenanceSide =
        ArrowLabUI::LoadSide::Left;
    bool leftCalibrationStored = false;
    bool rightCalibrationStored = false;
    bool leftBaselineStored = false;
    bool rightBaselineStored = false;

    char serialLine[128];
    size_t serialLineLength = 0;

    StoredLoadSide storedSide(ArrowLabUI::LoadSide side)
    {
        return side == ArrowLabUI::LoadSide::Left
            ? StoredLoadSide::Left
            : StoredLoadSide::Right;
    }

    void requestTare(ArrowLabUI::LoadSide side)
    {
        if (side == ArrowLabUI::LoadSide::Left) {
            leftTareRequested = true;
        } else {
            rightTareRequested = true;
        }
    }

    void requestCalibration(ArrowLabUI::LoadSide side)
    {
        if (side == ArrowLabUI::LoadSide::Left) {
            leftCalibrationRequested = true;
        } else {
            rightCalibrationRequested = true;
        }
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

    void requestDiagnosticResetChannel(ArrowLabUI::LoadSide side)
    {
        diagnosticMaintenanceSide = side;
        diagnosticResetChannelRequested = true;
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

    void startRequestedCalibrations(uint32_t currentTime)
    {
        if (leftCalibrationRequested) {
            leftCalibrationRequested = false;

            if (
                !rightSensor.calibrationInProgress()
                && leftSensor.startCalibration(
                    calibrationReferenceGrams,
                    currentTime,
                    CALIBRATION_SETTLE_TIME_MS
                )
            ) {
                leftCalibrationStored = false;
                Serial.println(
                    "LEFT calibration confirmed"
                );
            }
        }

        if (rightCalibrationRequested) {
            rightCalibrationRequested = false;

            if (
                !leftSensor.calibrationInProgress()
                && rightSensor.startCalibration(
                    calibrationReferenceGrams,
                    currentTime,
                    CALIBRATION_SETTLE_TIME_MS
                )
            ) {
                rightCalibrationStored = false;
                Serial.println(
                    "RIGHT calibration confirmed"
                );
            }
        }
    }

    void processDiagnosticRequests(uint32_t currentTime)
    {
        if (diagnosticResetChannelRequested) {
            diagnosticResetChannelRequested = false;
            const DiagnosticSide side =
                diagnosticMaintenanceSide == ArrowLabUI::LoadSide::Left
                    ? DiagnosticSide::Left
                    : DiagnosticSide::Right;
            creepDiagnostic.setBaselineCaptured(side, false);
            instrumentStorage.resetChannel(
                storedSide(diagnosticMaintenanceSide)
            );
            if (side == DiagnosticSide::Left) {
                leftSensor.clearCalibration();
                leftCalibrationStored = false;
                leftBaselineStored = false;
            } else {
                rightSensor.clearCalibration();
                rightCalibrationStored = false;
                rightBaselineStored = false;
            }
            Serial.printf(
                "AL_DIAG,EVENT,%s,CHANNEL_STATE_RESET\n",
                side == DiagnosticSide::Left ? "LEFT" : "RIGHT"
            );
        }

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
                leftSensor,
                rightSensor,
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

        if (sensor.calibrated()) {
            snprintf(
                buffer,
                bufferSize,
                "%.1f",
                sensor.grams()
            );
            return;
        }

        snprintf(
            buffer,
            bufferSize,
            "%ld",
            sensor.filteredZeroedValue()
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
            diagnosticState == CreepDiagnostic::State::Taring
            || diagnosticState == CreepDiagnostic::State::AwaitingLoad
            || diagnosticState == CreepDiagnostic::State::Running;

        switch (diagnosticState) {
        case CreepDiagnostic::State::WaitingForHost:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "NEXT: Start PC logger and wait for CONNECTED");
            break;

        case CreepDiagnostic::State::Taring:
            snprintf(
                diagnosticStatus,
                sizeof(diagnosticStatus),
                "WAIT: Taring %s - keep setup stable",
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
        ArrowLabUI::DiagnosticChannelState leftDiagnosticState;
        leftDiagnosticState.baselineCaptured =
            creepDiagnostic.baselineCaptured(DiagnosticSide::Left);
        leftDiagnosticState.tareComplete = leftSensor.tareComplete();
        leftDiagnosticState.userTareConfirmed =
            leftSensor.userTareConfirmed();
        leftDiagnosticState.calibrated = leftSensor.calibrated();
        leftDiagnosticState.calibrationReady =
            leftSensor.calibrationReady(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);
        leftDiagnosticState.calibrationInProgress =
            leftSensor.calibrationInProgress();
        leftDiagnosticState.calibrationLoadDetected =
            leftSensor.calibrationLoadDetected();
        leftDiagnosticState.settleRemainingSeconds =
            leftSensor.calibrationSettleRemainingSeconds(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);
        leftDiagnosticState.settlePercent =
            leftSensor.calibrationSettlePercent(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);

        ArrowLabUI::DiagnosticChannelState rightDiagnosticState;
        rightDiagnosticState.baselineCaptured =
            creepDiagnostic.baselineCaptured(DiagnosticSide::Right);
        rightDiagnosticState.tareComplete = rightSensor.tareComplete();
        rightDiagnosticState.userTareConfirmed =
            rightSensor.userTareConfirmed();
        rightDiagnosticState.calibrated = rightSensor.calibrated();
        rightDiagnosticState.calibrationReady =
            rightSensor.calibrationReady(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);
        rightDiagnosticState.calibrationInProgress =
            rightSensor.calibrationInProgress();
        rightDiagnosticState.calibrationLoadDetected =
            rightSensor.calibrationLoadDetected();
        rightDiagnosticState.settleRemainingSeconds =
            rightSensor.calibrationSettleRemainingSeconds(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);
        rightDiagnosticState.settlePercent =
            rightSensor.calibrationSettlePercent(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS);

        ArrowLabUI::setDiagnosticChannelState(
            leftDiagnosticState,
            rightDiagnosticState,
            creepDiagnostic.hostConnected(currentTime));

        ArrowLabUI::setLeftReading(leftText);
        ArrowLabUI::setRightReading(rightText);
        ArrowLabUI::setSensorHealth(leftLive, rightLive);
        ArrowLabUI::setCalibrationValidity(
            leftSensor.calibrated(),
            rightSensor.calibrated()
        );

        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Left,
            leftSensor.calibrated() ? "g" : "RAW"
        );
        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Right,
            rightSensor.calibrated() ? "g" : "RAW"
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Left,
            leftSensor.tareComplete(),
            leftSensor.userTareConfirmed(),
            leftSensor.calibrationReady(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            ),
            leftSensor.calibrationInProgress(),
            leftSensor.calibrated(),
            leftSensor.calibrationLoadDetected(),
            leftSensor.calibrationSettleRemainingSeconds(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            ),
            leftSensor.calibrationSettlePercent(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            )
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Right,
            rightSensor.tareComplete(),
            rightSensor.userTareConfirmed(),
            rightSensor.calibrationReady(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            ),
            rightSensor.calibrationInProgress(),
            rightSensor.calibrated(),
            rightSensor.calibrationLoadDetected(),
            rightSensor.calibrationSettleRemainingSeconds(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            ),
            rightSensor.calibrationSettlePercent(
                currentTime,
                CALIBRATION_SETTLE_TIME_MS
            )
        );

        const bool calibrationInProgress =
            leftSensor.calibrationInProgress()
            || rightSensor.calibrationInProgress();

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
                leftSensor.calibrationInProgress()
                    ? "Calibrating LEFT - keep weight stable"
                    : "Calibrating RIGHT - keep weight stable"
            );

            ArrowLabUI::setState(
                "CALIBRATING",
                lv_color_hex(0xFFB020)
            );
        } else if (leftLive && rightLive) {
            if (
                leftSensor.calibrated()
                && rightSensor.calibrated()
            ) {
                ArrowLabUI::setStatus(
                    "Both load channels calibrated"
                );
            } else if (
                leftSensor.userTareConfirmed()
                && rightSensor.userTareConfirmed()
            ) {
                if (leftSensor.calibrated()) {
                    ArrowLabUI::setStatus(
                        "Calibrate RIGHT with reference weight"
                    );
                } else if (rightSensor.calibrated()) {
                    ArrowLabUI::setStatus(
                        "Calibrate LEFT with reference weight"
                    );
                } else {
                    ArrowLabUI::setStatus(
                        "Both loads ready for calibration"
                    );
                }
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
        requestDiagnosticFinish,
        requestDiagnosticResetChannel
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

    StoredCalibration leftCalibration;
    if (instrumentStorage.loadCalibration(
            StoredLoadSide::Left,
            leftCalibration)) {
        leftSensor.restoreCalibration(
            leftCalibration.factor,
            leftCalibration.referenceGrams);
        leftCalibrationStored = true;
        Serial.println("Restored LEFT calibration from NVS");
    }

    StoredCalibration rightCalibration;
    if (instrumentStorage.loadCalibration(
            StoredLoadSide::Right,
            rightCalibration)) {
        rightSensor.restoreCalibration(
            rightCalibration.factor,
            rightCalibration.referenceGrams);
        rightCalibrationStored = true;
        Serial.println("Restored RIGHT calibration from NVS");
    }

    leftBaselineStored = instrumentStorage.baselineCaptured(
        StoredLoadSide::Left);
    rightBaselineStored = instrumentStorage.baselineCaptured(
        StoredLoadSide::Right);
    creepDiagnostic.setBaselineCaptured(
        DiagnosticSide::Left,
        leftBaselineStored);
    creepDiagnostic.setBaselineCaptured(
        DiagnosticSide::Right,
        rightBaselineStored);

    Serial.println(
        "Dual HX711 initialization complete - starting independent tare"
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
        leftTareRequested
        || rightTareRequested
    ) {
        startRequestedTares();
    }

    if (
        leftCalibrationRequested
        || rightCalibrationRequested
    ) {
        startRequestedCalibrations(now);
    }

    if (
        diagnosticStartRequested
        || diagnosticCancelRequested
        || diagnosticFinishRequested
        || diagnosticResetChannelRequested
    ) {
        processDiagnosticRequests(now);
    }

    /*
     * Each channel is tested independently.
     * One missing sensor cannot prevent the other from working.
     */
    const bool leftFresh = leftSensor.read(now);

    if (leftFresh) {
        leftSensor.updateCalibrationLoadDetection(
            CALIBRATION_LOAD_THRESHOLD_COUNTS
        );
    }

    const bool rightFresh = rightSensor.read(now);

    if (rightFresh) {
        rightSensor.updateCalibrationLoadDetection(
            CALIBRATION_LOAD_THRESHOLD_COUNTS
        );
    }

    creepDiagnostic.update(
        now,
        leftFresh,
        rightFresh,
        leftSensor,
        rightSensor
    );

    if (
        leftSensor.calibrated()
        && !leftSensor.calibrationInProgress()
        && !leftCalibrationStored
    ) {
        leftCalibrationStored = instrumentStorage.saveCalibration(
            StoredLoadSide::Left,
            leftSensor.calibrationFactor(),
            leftSensor.calibrationReferenceGrams());
        if (leftCalibrationStored) {
            Serial.println("Stored LEFT calibration in NVS");
        }
    }

    if (
        rightSensor.calibrated()
        && !rightSensor.calibrationInProgress()
        && !rightCalibrationStored
    ) {
        rightCalibrationStored = instrumentStorage.saveCalibration(
            StoredLoadSide::Right,
            rightSensor.calibrationFactor(),
            rightSensor.calibrationReferenceGrams());
        if (rightCalibrationStored) {
            Serial.println("Stored RIGHT calibration in NVS");
        }
    }

    const bool leftBaselineNow = creepDiagnostic.baselineCaptured(
        DiagnosticSide::Left);
    if (leftBaselineNow && !leftBaselineStored) {
        leftBaselineStored = instrumentStorage.setBaselineCaptured(
            StoredLoadSide::Left,
            true);
    }

    const bool rightBaselineNow = creepDiagnostic.baselineCaptured(
        DiagnosticSide::Right);
    if (rightBaselineNow && !rightBaselineStored) {
        rightBaselineStored = instrumentStorage.setBaselineCaptured(
            StoredLoadSide::Right,
            true);
    }

    updateDisplay(now);

    delay(5);
}
