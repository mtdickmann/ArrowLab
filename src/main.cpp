/*
 * ArrowLab firmware
 * Development firmware; version is defined only in Version.h.
 *
 * Top-level HMI and diagnostics coordinator.
 * The separate WROOM measurement node owns HX711 acquisition, weighing,
 * tare/calibration workflow and calibration persistence.
 */

#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "ArrowLabConfig.h"
#include "measurement/LoadCellChannel.h"
#include "measurement/LoadAggregation.h"
#include "measurement/MeasurementNodeClient.h"
#include "diagnostics/CreepDiagnostic.h"
#include "protocol/MeasurementProtocol.h"
#include "spine/SpineCalculation.h"
#include "ui/MassDisplay.h"
#include "ui/ui.h"
#include "Version.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace
{
    constexpr uint32_t SENSOR_UPDATE_INTERVAL_MS = 100;
    float calibrationReferenceGrams = 0.0f;
    ArrowLabConfig::MassUnit selectedMassUnit =
        ArrowLabConfig::PRIMARY_MASS_UNIT;
    volatile bool unitCycleRequested = false;
    float requestedMarkedSpine = 0.0f;
    // Raw-only mirrors retained for the hidden creep diagnostic. The Viewe
    // never initializes an HX711 or performs measurement calculations.
    LoadCellChannel leftSensor("Left remote", 0, 0);
    LoadCellChannel rightSensor("Right remote", 0, 0);
    MeasurementNodeClient measurementNode;
    CreepDiagnostic creepDiagnostic;

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

    ArrowLabProtocol::Side protocolSide(ArrowLabUI::LoadSide side)
    {
        return side == ArrowLabUI::LoadSide::Left
            ? ArrowLabProtocol::Side::Left
            : ArrowLabProtocol::Side::Right;
    }

    void requestTare(ArrowLabUI::LoadSide side)
    {
        measurementNode.requestTare(protocolSide(side));
    }

    void requestCalibration(
        ArrowLabUI::LoadSide side,
        float referenceGrams)
    {
        if (referenceGrams > 0.0f) {
            calibrationReferenceGrams = referenceGrams;
            ArrowLabUI::setCalibrationReferenceGrams(referenceGrams);
        }

        if (referenceGrams > 0.0f) {
            measurementNode.prepareCalibration(
                protocolSide(side),
                referenceGrams);
        } else {
            measurementNode.startCalibration(protocolSide(side));
        }
    }

    void requestMassUnitCycle()
    {
        unitCycleRequested = true;
    }

    void requestSpineStart(uint8_t positionCount, float markedSpine)
    {
        requestedMarkedSpine = markedSpine;
        measurementNode.startSpineTest(positionCount);
    }

    void requestSpineCancel()
    {
        measurementNode.cancelSpineTest();
    }

    void updateSpineDisplay(
        const ArrowLabProtocol::StatusPacket &status,
        uint32_t currentTime)
    {
        using SpineStage = ArrowLabProtocol::SpineStage;
        const SpineStage stage = static_cast<SpineStage>(status.spineStage);
        const uint8_t position = status.spineCurrentPosition;
        const uint8_t positionCount = status.spinePositionCount == 4 ? 4 : 1;
        const float arrowGrams = status.arrowMilliGrams / 1000.0f;
        const float appliedGrams = status.appliedMilliGrams / 1000.0f;
        const char *state = "READY";
        char detail[96] = "Empty both supports, then press START";
        char results[160] = "";
        bool active = false;
        bool complete = false;

        switch (stage) {
        case SpineStage::TaringLeft:
            state = "READ";
            snprintf(detail, sizeof(detail), "Automatic empty tare: LEFT");
            active = true;
            break;
        case SpineStage::TaringRight:
            state = "READ";
            snprintf(detail, sizeof(detail), "Automatic empty tare: RIGHT");
            active = true;
            break;
        case SpineStage::AwaitingArrow:
            state = "READ";
            snprintf(detail, sizeof(detail), "Place arrow on both supports");
            active = true;
            break;
        case SpineStage::StabilizingArrow:
            state = "READ";
            snprintf(detail, sizeof(detail), "Keep arrow still - capturing mass");
            active = true;
            break;
        case SpineStage::ReadyToPress:
            state = position > 0 ? "ROTATE + PUSH" : "PUSH";
            snprintf(
                detail,
                sizeof(detail),
                "Position %u/%u | arrow %.2f g | press to hard stop",
                position + 1,
                positionCount,
                arrowGrams);
            active = true;
            break;
        case SpineStage::Holding:
            state = "HOLD";
            snprintf(
                detail,
                sizeof(detail),
                "Position %u/%u | applied %.1f g | keep steady",
                position + 1,
                positionCount,
                appliedGrams);
            active = true;
            break;
        case SpineStage::AwaitingRelease:
            state = "CAPTURED";
            snprintf(
                detail,
                sizeof(detail),
                "Position %u captured - RELEASE fully",
                position + 1);
            active = true;
            break;
        case SpineStage::Complete: {
            state = "COMPLETE";
            complete = true;
            float values[4] = {};
            float minimum = 100000.0f;
            float maximum = 0.0f;
            for (uint8_t index = 0; index < positionCount; ++index) {
                values[index] = SpineCalculation::equivalentSpine(
                    status.positionMilliGrams[index] / 1000.0f);
                if (values[index] < minimum) minimum = values[index];
                if (values[index] > maximum) maximum = values[index];
            }
            if (positionCount == 1) {
                snprintf(results, sizeof(results), "MEASURED SPINE: %.0f", values[0]);
                if (requestedMarkedSpine > 0.0f) {
                    snprintf(
                        detail,
                        sizeof(detail),
                        "Marked %.0f | delta %+.0f (%+.1f%%) | arrow %.2f g",
                        requestedMarkedSpine,
                        SpineCalculation::markedDifference(values[0], requestedMarkedSpine),
                        SpineCalculation::markedDifferencePercent(values[0], requestedMarkedSpine),
                        arrowGrams);
                } else {
                    snprintf(detail, sizeof(detail), "Arrow %.2f g | single orientation", arrowGrams);
                }
            } else {
                const float range = maximum - minimum;
                snprintf(
                    results,
                    sizeof(results),
                    "P1 %.0f  P2 %.0f  P3 %.0f  P4 %.0f\nSTIFF %.0f  WEAK %.0f  RANGE %.0f / SAS %.3f in",
                    values[0], values[1], values[2], values[3],
                    minimum, maximum, range, range / 1000.0f);
                if (requestedMarkedSpine > 0.0f) {
                    snprintf(
                        detail,
                        sizeof(detail),
                        "Worst vs marked %.0f: %+.0f (%+.1f%%) | arrow %.2f g",
                        requestedMarkedSpine,
                        SpineCalculation::markedDifference(maximum, requestedMarkedSpine),
                        SpineCalculation::markedDifferencePercent(maximum, requestedMarkedSpine),
                        arrowGrams);
                } else {
                    snprintf(detail, sizeof(detail), "Best = stiffest; worst = weakest | arrow %.2f g", arrowGrams);
                }
            }
            break;
        }
        case SpineStage::Fault:
            state = "FAULT";
            snprintf(detail, sizeof(detail), "Restore both calibrated load channels");
            break;
        case SpineStage::Idle:
        default:
            break;
        }

        ArrowLabUI::setSpineDisplay(
            state,
            detail,
            results,
            status.spineHoldPercent,
            active,
            complete);
        (void)currentTime;
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
        const ArrowLabProtocol::ChannelStatus &channel,
        bool live
    )
    {
        if (!live) {
            snprintf(buffer, bufferSize, "---");
            return;
        }

        if (!(channel.flags & ArrowLabProtocol::TareComplete)) {
            snprintf(buffer, bufferSize, "TARE");
            return;
        }

        if (channel.flags & ArrowLabProtocol::Calibrated) {
            const float grams =
                static_cast<float>(channel.heldMilliGrams) / 1000.0f;
            MassDisplay::formatPrimary(
                buffer,
                bufferSize,
                grams,
                selectedMassUnit);
            return;
        }

        snprintf(
            buffer,
            bufferSize,
            "%ld",
            static_cast<long>(channel.heldRawCounts)
        );
    }

    void formatConversions(
        char *buffer,
        size_t bufferSize,
        const ArrowLabProtocol::ChannelStatus &channel,
        bool live)
    {
        if (
            !live
            || !(channel.flags & ArrowLabProtocol::TareComplete)
            || !(channel.flags & ArrowLabProtocol::Calibrated)
        ) {
            buffer[0] = '\0';
            return;
        }

        const float grams =
            static_cast<float>(channel.heldMilliGrams) / 1000.0f;
        MassDisplay::formatSecondary(
            buffer,
            bufferSize,
            grams,
            selectedMassUnit);
    }

    void updateDisplay(uint32_t currentTime)
    {
        using Stage = ArrowLabProtocol::CalibrationStage;
        if (unitCycleRequested) {
            unitCycleRequested = false;
            selectedMassUnit = MassDisplay::nextUnit(selectedMassUnit);
        }

        const bool nodeConnected = measurementNode.connected(currentTime);
        const ArrowLabProtocol::ChannelStatus &left =
            measurementNode.channel(ArrowLabProtocol::Side::Left);
        const ArrowLabProtocol::ChannelStatus &right =
            measurementNode.channel(ArrowLabProtocol::Side::Right);
        const bool leftLive = nodeConnected
            && (left.flags & ArrowLabProtocol::ChannelLive);
        const bool rightLive = nodeConnected
            && (right.flags & ArrowLabProtocol::ChannelLive);
        const Stage leftStage = static_cast<Stage>(left.stage);
        const Stage rightStage = static_cast<Stage>(right.stage);
        const bool leftCalibrated =
            left.flags & ArrowLabProtocol::Calibrated;
        const bool rightCalibrated =
            right.flags & ArrowLabProtocol::Calibrated;
        const bool leftUserTare =
            left.flags & ArrowLabProtocol::UserTareConfirmed;
        const bool rightUserTare =
            right.flags & ArrowLabProtocol::UserTareConfirmed;

        char leftText[24];
        char rightText[24];
        char leftConversions[48];
        char rightConversions[48];

        formatReading(
            leftText,
            sizeof(leftText),
            left,
            leftLive
        );

        formatReading(
            rightText,
            sizeof(rightText),
            right,
            rightLive
        );

        formatConversions(
            leftConversions,
            sizeof(leftConversions),
            left,
            leftLive
        );
        formatConversions(
            rightConversions,
            sizeof(rightConversions),
            right,
            rightLive
        );

        const auto setupActive = [](Stage stage) {
            return stage == Stage::AwaitingLoad
                || stage == Stage::ReadyToCalibrate
                || stage == Stage::Settling
                || stage == Stage::Sampling;
        };
        const auto calibrationBusy = [](Stage stage) {
            return stage == Stage::Settling
                || stage == Stage::Sampling;
        };
        const bool tareInProgress =
            leftStage == Stage::Taring || rightStage == Stage::Taring;

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
        ArrowLabUI::setLoadConversions(
            ArrowLabUI::LoadSide::Left,
            leftConversions);
        ArrowLabUI::setLoadConversions(
            ArrowLabUI::LoadSide::Right,
            rightConversions);
        ArrowLabUI::setSensorHealth(
            nodeConnected,
            leftLive,
            rightLive);
        ArrowLabUI::setCalibrationValidity(
            leftCalibrated,
            rightCalibrated
        );

        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Left,
            leftCalibrated
                ? MassDisplay::unitText(selectedMassUnit)
                : "RAW"
        );
        ArrowLabUI::setLoadUnit(
            ArrowLabUI::LoadSide::Right,
            rightCalibrated
                ? MassDisplay::unitText(selectedMassUnit)
                : "RAW"
        );

        const bool leftReady =
            leftLive && leftCalibrated && leftUserTare;
        const bool rightReady =
            rightLive && rightCalibrated && rightUserTare;
        const LoadAggregation::Result weighResult =
            LoadAggregation::combine(
                {
                    leftReady,
                    left.heldRawCounts,
                    left.heldMilliGrams
                },
                {
                    rightReady,
                    right.heldRawCounts,
                    right.heldMilliGrams
                });

        const float weighGrams =
            static_cast<float>(weighResult.totalMilliGrams) / 1000.0f;
        const char *weighInstruction =
            "Place load on LEFT, RIGHT, or both cassettes";

        if (weighResult.source == LoadAggregation::Source::NotReady) {
            weighInstruction = "Tare and calibrate a live cassette first";
        }

        char weighPrimary[24];
        char weighSecondary[48];
        if (weighResult.source == LoadAggregation::Source::NotReady) {
            snprintf(weighPrimary, sizeof(weighPrimary), "---");
            weighSecondary[0] = '\0';
        } else {
            MassDisplay::formatPrimary(
                weighPrimary,
                sizeof(weighPrimary),
                weighGrams,
                selectedMassUnit);
            MassDisplay::formatSecondary(
                weighSecondary,
                sizeof(weighSecondary),
                weighGrams,
                selectedMassUnit);
        }

        ArrowLabUI::setWeighDisplay(
            LoadAggregation::sourceText(weighResult.source),
            weighPrimary,
            MassDisplay::unitText(selectedMassUnit),
            weighSecondary,
            weighInstruction);

        updateSpineDisplay(measurementNode.status(), currentTime);

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Left,
            left.flags & ArrowLabProtocol::TareComplete,
            leftStage == Stage::Taring,
            leftUserTare,
            leftStage == Stage::ReadyToCalibrate,
            calibrationBusy(leftStage),
            leftCalibrated,
            setupActive(leftStage),
            left.settleRemainingSeconds,
            left.settlePercent
        );

        ArrowLabUI::setLoadStatus(
            ArrowLabUI::LoadSide::Right,
            right.flags & ArrowLabProtocol::TareComplete,
            rightStage == Stage::Taring,
            rightUserTare,
            rightStage == Stage::ReadyToCalibrate,
            calibrationBusy(rightStage),
            rightCalibrated,
            setupActive(rightStage),
            right.settleRemainingSeconds,
            right.settlePercent
        );

        const bool calibrationInProgress =
            calibrationBusy(leftStage) || calibrationBusy(rightStage);

        if (!nodeConnected) {
            ArrowLabUI::setStatus(
                "Check node power and UART wiring"
            );
            ArrowLabUI::setState(
                "NODE OFFLINE",
                lv_color_hex(0xFF4D4D)
            );
        } else if (tareInProgress) {
            ArrowLabUI::setStatus(
                "Taring load cell - keep setup stable"
            );

            ArrowLabUI::setState(
                "TARING",
                lv_color_hex(0xFFB020)
            );
        } else if (calibrationInProgress) {
            ArrowLabUI::setStatus(
                calibrationBusy(leftStage)
                    ? "Calibrating LEFT - keep weight stable"
                    : "Calibrating RIGHT - keep weight stable"
            );

            ArrowLabUI::setState(
                "CALIBRATING",
                lv_color_hex(0xFFB020)
            );
        } else if (leftLive && rightLive) {
            if (
                leftStage == Stage::AwaitingLoad
            ) {
                char nextAction[72];
                snprintf(
                    nextAction,
                    sizeof(nextAction),
                    "NEXT: Place %.1f g calibration weight on LEFT",
                    static_cast<float>(left.referenceMilliGrams) / 1000.0f);
                ArrowLabUI::setStatus(nextAction);
            } else if (
                rightStage == Stage::AwaitingLoad
            ) {
                char nextAction[72];
                snprintf(
                    nextAction,
                    sizeof(nextAction),
                    "NEXT: Place %.1f g calibration weight on RIGHT",
                    static_cast<float>(right.referenceMilliGrams) / 1000.0f);
                ArrowLabUI::setStatus(nextAction);
            } else if (
                leftStage == Stage::ReadyToCalibrate
            ) {
                ArrowLabUI::setStatus(
                    "NEXT: Press LEFT CAL to start 30 s stabilization");
            } else if (
                rightStage == Stage::ReadyToCalibrate
            ) {
                ArrowLabUI::setStatus(
                    "NEXT: Press RIGHT CAL to start 30 s stabilization");
            } else if (
                leftCalibrated && rightCalibrated
            ) {
                ArrowLabUI::setStatus(
                    "Both load channels calibrated"
                );
            } else if (
                leftUserTare && rightUserTare
            ) {
                if (leftCalibrated) {
                    ArrowLabUI::setStatus(
                        "Calibrate RIGHT with reference weight"
                    );
                } else if (rightCalibrated) {
                    ArrowLabUI::setStatus(
                        "Calibrate LEFT with reference weight"
                    );
                } else {
                    ArrowLabUI::setStatus(
                        "Both loads ready for calibration"
                    );
                }
            } else if (leftUserTare) {
                ArrowLabUI::setStatus(
                    "Tare RIGHT before calibration"
                );
            } else if (rightUserTare) {
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
                "Left live - WROOM reports right HX711 offline"
            );

            ArrowLabUI::setState(
                "LEFT LIVE",
                lv_color_hex(0xFFB020)
            );
        } else if (rightLive) {
            ArrowLabUI::setStatus(
                "Right live - WROOM reports left HX711 offline"
            );

            ArrowLabUI::setState(
                "RIGHT LIVE",
                lv_color_hex(0xFFB020)
            );
        } else {
            ArrowLabUI::setStatus(
                "Measurement node online - waiting for HX711 data"
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

    // Production measurement link: one-wire half-duplex on VIEWE GPIO17
    // to WROOM GPIO8. GPIO11/GPIO12 remain free for the restored onboard SD
    // interface and are reserved only as the documented UART fallback.
    measurementNode.begin();

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
    ArrowLabUI::setUnitCycleCallback(requestMassUnitCycle);
    ArrowLabUI::setSpineCallbacks(requestSpineStart, requestSpineCancel);
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
        "Connecting to WROOM measurement node"
    );

    ArrowLabUI::setState(
        "INITIALIZING",
        lv_color_hex(0xFFB020)
    );

    lvgl_port_unlock();

    Serial.println(
        "Viewe HMI ready - awaiting WROOM measurement data"
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

    measurementNode.poll(now);
    const bool packetFresh = measurementNode.freshPacket();
    const bool nodeConnected = measurementNode.connected(now);
    const ArrowLabProtocol::ChannelStatus &left =
        measurementNode.channel(ArrowLabProtocol::Side::Left);
    const ArrowLabProtocol::ChannelStatus &right =
        measurementNode.channel(ArrowLabProtocol::Side::Right);
    const bool leftFresh = packetFresh
        && nodeConnected
        && (left.flags & ArrowLabProtocol::ChannelLive);
    const bool rightFresh = packetFresh
        && nodeConnected
        && (right.flags & ArrowLabProtocol::ChannelLive);

    if (leftFresh) {
        leftSensor.acceptRemoteRaw(left.rawCount, now);
    }
    if (rightFresh) {
        rightSensor.acceptRemoteRaw(right.rawCount, now);
    }

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
