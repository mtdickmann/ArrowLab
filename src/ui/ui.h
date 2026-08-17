#pragma once

#include <lvgl.h>

namespace ArrowLabUI
{
    enum class LoadSide
    {
        Left,
        Right
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
    using UnitCycleCallback = void (*)();
    using SpineStartCallback = void (*)(uint8_t positionCount, float markedSpine);
    using SpineControlCallback = void (*)();
    using SpineMarkedCallback = void (*)(float markedSpine);
    using WifiScanCallback = void (*)();
    using WifiConnectCallback = void (*)(
        const char *ssid,
        const char *password);
    using WifiForgetCallback = bool (*)(const char *ssid);

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
    void setUnitCycleCallback(UnitCycleCallback callback);
    void setSpineCallbacks(
        SpineStartCallback startCallback,
        SpineControlCallback cancelCallback,
        SpineControlCallback confirmClearCallback,
        SpineControlCallback confirmZeroCallback,
        SpineControlCallback restartCallback,
        SpineMarkedCallback markedCallback);
    void setCalibrationReferenceGrams(float grams);
    void setWifiCallbacks(
        WifiScanCallback scanCallback,
        WifiConnectCallback connectCallback,
        WifiForgetCallback forgetCallback);
    void setWifiScanResults(
        const char ssids[][33],
        const int16_t *rssiDbm,
        const bool *secured,
        size_t count);
    void setWifiScanBusy();
    void setWifiSetupResult(
        bool busy,
        bool success,
        const char *message);
    void setWifiSavedCredentials(bool saved);

    void setDiagnosticCallbacks(
        DiagnosticStartCallback startCallback,
        DiagnosticCancelCallback cancelCallback,
        DiagnosticFinishCallback finishCallback
    );
    void setDiagnosticStatus(
        const char *text,
        uint8_t progressPercent,
        bool active,
        bool awaitingSave
    );
    void setDiagnosticHostConnected(bool hostConnected);
    void setCalibrationValidity(bool leftCalibrated, bool rightCalibrated);
    void setSensorHealth(
        bool measurementNodeOnline,
        bool leftLive,
        bool rightLive);

    void setFirmwareUpdateActive(bool active);
    void setFirmwareUpdateFailed();

    void setNetworkStatus(
        bool vieweConnected,
        const char *vieweHostname,
        const char *vieweSsid,
        const char *vieweIp,
        int16_t vieweRssiDbm,
        const char *vieweMac,
        bool wroomOnline,
        bool wroomConnected,
        const char *wroomHostname,
        const char *wroomSsid,
        const char *wroomIp,
        int16_t wroomRssiDbm,
        const char *wroomMac);

    /**
     * Update the displayed raw reading for the left sensor.
     */
    void setLeftReading(const char *text);

    /**
     * Update the displayed raw reading for the right sensor.
     */
    void setRightReading(const char *text);
    void setLoadUnit(LoadSide side, const char *text);
    void setLoadConversions(LoadSide side, const char *text);
    void setWeighDisplay(
        const char *source,
        const char *primary,
        const char *unit,
        const char *secondary,
        const char *instruction);
    void setSpineDisplay(
        const char *state,
        const char *detail,
        const char *results,
        const char *primaryAction,
        uint8_t progressPercent,
        bool active,
        bool complete,
        bool clearConfirmationRequired,
        bool zeroConfirmationRequired);

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
