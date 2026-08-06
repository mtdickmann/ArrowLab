#pragma once

#include <lvgl.h>

namespace ArrowLabUI
{
    using TareCallback = void (*)();

    /**
     * Creates the complete ArrowLab home screen.
     * Call once after LVGL has been initialized.
     */
    void create();

    /**
     * Registers the application callback used by the TARE button.
     * The UI only requests a tare; HX711 state remains application-owned.
     */
    void setTareCallback(TareCallback callback);

    /**
     * Update the displayed raw reading for the left sensor.
     */
    void setLeftReading(const char *text);

    /**
     * Update the displayed raw reading for the right sensor.
     */
    void setRightReading(const char *text);

    /**
     * Update the status message at the bottom of the screen.
     */
    void setStatus(const char *text);

    /**
     * Update the state indicator shown at bottom-right.
     *
     * Example:
     * setState("READY", lv_color_hex(0x4CD964));
     */
    void setState(const char *text, lv_color_t colour);
}