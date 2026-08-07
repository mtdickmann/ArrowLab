#pragma once

#include <lvgl.h>

namespace ArrowLabUI
{
    enum class LoadSide
    {
        Left,
        Right
    };

    struct DiagnosticChannelState
    {
        bool baselineCaptured = false;
        bool tareComplete = false;
        bool tareInProgress = false;
        bool userTareConfirmed = false;
        bool calibrated = false;
        bool calibrationSetupActive = false;
        bool calibrationReady = false;
        bool calibrationInProgress = false;
        bool calibrationLoadDetected = false;
        uint32_t settleRemainingSeconds = 0;
        uint8_t settlePercent = 0;
    };

    using TareCallback = void (*)(LoadSide side);
    using CalibrationCallback = void (*)(LoadSide side, float referenceGrams);
    using DiagnosticStartCallback = void (*)(
        LoadSide side,
        float testMassGrams,
        bool zeroBaseline
    );
    using DiagnosticCancelCallback = void (*)();
    using DiagnosticFinishCallback = void (*)();
    using DiagnosticResetCallback = void (*)(LoadSide side);

    /**
     * Creates the complete ArrowLab home screen.
     * Call once after LVGL has been initialized.
     */
    void create();

    /**
     * Registers the application callback used after a user confirms
     * a LEFT or RIGHT tare operation.
     */
    void setTareCallback(TareCallback callback);
    void setCalibrationCallback(CalibrationCallback callback);
    void setCalibrationReferenceGrams(float grams);
    void setDiagnosticCallbacks(
        DiagnosticStartCallback startCallback,
        DiagnosticCancelCallback cancelCallback,
        DiagnosticFinishCallback finishCallback,
        DiagnosticResetCallback resetCallback
    );
    void setDiagnosticStatus(
        const char *text,
        uint8_t progressPercent,
        bool active,
        bool awaitingSave
    );
    void setDiagnosticChannelState(
        const DiagnosticChannelState &left,
        const DiagnosticChannelState &right,
        bool hostConnected
    );
    void setCalibrationValidity(bool leftCalibrated, bool rightCalibrated);
    void setSensorHealth(bool leftLive, bool rightLive);

    /**
     * Update the displayed raw reading for the left sensor.
     */
    void setLeftReading(const char *text);

    /**
     * Update the displayed raw reading for the right sensor.
     */
    void setRightReading(const char *text);
    void setLoadUnit(LoadSide side, const char *text);

    /**
     * Update the tare/calibration status shown for one load channel.
     */
    void setLoadStatus(
        LoadSide side,
        bool tareComplete,
        bool tareInProgress,
        bool userTareConfirmed,
        bool calibrationReady,
        bool calibrationInProgress,
        bool calibrated,
        bool calibrationSetupActive,
        uint32_t settleRemainingSeconds,
        uint8_t settlePercent
    );

    /**
     * Update the status message at the bottom of the screen.
     */
    void setStatus(const char *text);

    /**
     * Update the state indicator shown at bottom-right.
     */
    void setState(const char *text, lv_color_t colour);
}
