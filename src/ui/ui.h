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
    using CalibrationCallback = void (*)(LoadSide side);

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
    void setCalibrationValidity(bool leftCalibrated, bool rightCalibrated);

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
        bool userTareConfirmed,
        bool calibrationReady,
        bool calibrationInProgress,
        bool calibrated
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
