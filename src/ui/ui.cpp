#include "ui.h"

#include <cstdio>

namespace
{
    // Main colours
    constexpr uint32_t COLOUR_BACKGROUND = 0x101419;
    constexpr uint32_t COLOUR_PANEL      = 0x1A2027;
    constexpr uint32_t COLOUR_HEADER     = 0x141A20;
    constexpr uint32_t COLOUR_BORDER     = 0x303A44;
    constexpr uint32_t COLOUR_TEXT       = 0xF2F5F7;
    constexpr uint32_t COLOUR_MUTED      = 0x9AA6B2;
    constexpr uint32_t COLOUR_ACCENT     = 0x4EA3FF;

    struct ReadingPanelRefs
    {
        lv_obj_t *valueLabel = nullptr;
        lv_obj_t *unitLabel = nullptr;
        lv_obj_t *statusLabel = nullptr;
        lv_obj_t *tareButton = nullptr;
        lv_obj_t *calibrationButton = nullptr;
    };

    ReadingPanelRefs leftPanel;
    ReadingPanelRefs rightPanel;

    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *stateLabel  = nullptr;
    lv_obj_t *confirmationBox = nullptr;

    enum class ConfirmationAction
    {
        Tare,
        Calibration
    };

    ArrowLabUI::TareCallback tareCallback = nullptr;
    ArrowLabUI::CalibrationCallback calibrationCallback = nullptr;
    float calibrationReferenceGrams = 0.0f;
    ArrowLabUI::LoadSide pendingSide =
        ArrowLabUI::LoadSide::Left;
    ConfirmationAction pendingAction =
        ConfirmationAction::Tare;

    void stylePanel(lv_obj_t *panel)
    {
        lv_obj_set_style_bg_color(
            panel,
            lv_color_hex(COLOUR_PANEL),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(
            panel,
            LV_OPA_COVER,
            LV_PART_MAIN
        );

        lv_obj_set_style_border_color(
            panel,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN
        );

        lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

        // Prevent scrolling or scrollbars on fixed UI panels
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
    }

    lv_obj_t *createTextLabel(
        lv_obj_t *parent,
        const char *text,
        const lv_font_t *font,
        lv_color_t colour
    )
    {
        lv_obj_t *label = lv_label_create(parent);

        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, colour, LV_PART_MAIN);

        return label;
    }

    ReadingPanelRefs createReadingPanel(
        lv_obj_t *parent,
        const char *titleText,
        int xPosition,
        lv_event_cb_t tareEventCallback,
        lv_event_cb_t calibrationEventCallback
    )
    {
        ReadingPanelRefs refs;

        lv_obj_t *panel = lv_obj_create(parent);
        lv_obj_set_size(panel, 214, 148);
        lv_obj_set_pos(panel, xPosition, 54);
        stylePanel(panel);

        lv_obj_t *title = createTextLabel(
            panel,
            titleText,
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_MUTED)
        );

        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

        refs.statusLabel = createTextLabel(
            panel,
            "TARING  CAL --",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED)
        );

        lv_obj_align(
            refs.statusLabel,
            LV_ALIGN_TOP_MID,
            0,
            30
        );

        lv_obj_t *divider = lv_obj_create(panel);
        lv_obj_set_size(divider, 170, 2);
        lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 51);

        lv_obj_set_style_bg_color(
            divider,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(divider, 1, LV_PART_MAIN);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);

        /*
         * Keep the reading and its unit in one flex row so a changing
         * number width cannot overwrite the unit label.
         */
        lv_obj_t *readingRow = lv_obj_create(panel);
        lv_obj_set_size(readingRow, 190, 42);
        lv_obj_align(readingRow, LV_ALIGN_CENTER, 0, 8);
        lv_obj_set_style_bg_opa(
            readingRow,
            LV_OPA_TRANSP,
            LV_PART_MAIN
        );
        lv_obj_set_style_border_width(
            readingRow,
            0,
            LV_PART_MAIN
        );
        lv_obj_set_style_pad_all(
            readingRow,
            0,
            LV_PART_MAIN
        );
        lv_obj_clear_flag(
            readingRow,
            LV_OBJ_FLAG_SCROLLABLE
        );
        lv_obj_set_flex_flow(
            readingRow,
            LV_FLEX_FLOW_ROW
        );
        lv_obj_set_flex_align(
            readingRow,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER
        );

        refs.valueLabel = createTextLabel(
            readingRow,
            "0",
            &lv_font_montserrat_30,
            lv_color_hex(COLOUR_TEXT)
        );

        refs.unitLabel = createTextLabel(
            readingRow,
            "RAW",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED)
        );

        refs.tareButton = lv_btn_create(panel);
        lv_obj_set_size(refs.tareButton, 68, 28);
        lv_obj_align(
            refs.tareButton,
            LV_ALIGN_BOTTOM_LEFT,
            10,
            -6
        );
        lv_obj_set_style_radius(
            refs.tareButton,
            7,
            LV_PART_MAIN
        );
        lv_obj_set_style_bg_color(
            refs.tareButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN
        );
        lv_obj_set_style_pad_all(
            refs.tareButton,
            2,
            LV_PART_MAIN
        );
        lv_obj_add_event_cb(
            refs.tareButton,
            tareEventCallback,
            LV_EVENT_CLICKED,
            nullptr
        );

        lv_obj_t *tareLabel = createTextLabel(
            refs.tareButton,
            "TARE",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT)
        );
        lv_obj_center(tareLabel);

        refs.calibrationButton = lv_btn_create(panel);
        lv_obj_set_size(refs.calibrationButton, 68, 28);
        lv_obj_align(
            refs.calibrationButton,
            LV_ALIGN_BOTTOM_LEFT,
            84,
            -6
        );
        lv_obj_set_style_radius(
            refs.calibrationButton,
            7,
            LV_PART_MAIN
        );
        lv_obj_set_style_bg_color(
            refs.calibrationButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN
        );
        lv_obj_set_style_pad_all(
            refs.calibrationButton,
            2,
            LV_PART_MAIN
        );
        lv_obj_add_event_cb(
            refs.calibrationButton,
            calibrationEventCallback,
            LV_EVENT_CLICKED,
            nullptr
        );

        lv_obj_t *calibrationLabel = createTextLabel(
            refs.calibrationButton,
            "CAL",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT)
        );
        lv_obj_center(calibrationLabel);

        return refs;
    }

    void confirmationEvent(lv_event_t *event)
    {
        lv_obj_t *messageBox =
            lv_event_get_current_target(event);

        const uint16_t selectedButton =
            lv_msgbox_get_active_btn(messageBox);

        if (selectedButton == 1) {
            if (
                pendingAction == ConfirmationAction::Tare
                && tareCallback != nullptr
            ) {
                tareCallback(pendingSide);
            } else if (
                pendingAction
                    == ConfirmationAction::Calibration
                && calibrationCallback != nullptr
            ) {
                calibrationCallback(pendingSide);
            }
        }

        confirmationBox = nullptr;
        lv_msgbox_close(messageBox);
    }

    void showTareConfirmation(ArrowLabUI::LoadSide side)
    {
        if (confirmationBox != nullptr) {
            return;
        }

        pendingSide = side;
        pendingAction = ConfirmationAction::Tare;

        static const char *buttons[] = {
            "CANCEL",
            "TARE",
            ""
        };

        const bool isLeft =
            side == ArrowLabUI::LoadSide::Left;

        confirmationBox = lv_msgbox_create(
            nullptr,
            isLeft ? "TARE LEFT" : "TARE RIGHT",
            isLeft
                ? "Place calibration platform on LEFT load.\nEnsure the setup is stable before confirming."
                : "Place calibration platform on RIGHT load.\nEnsure the setup is stable before confirming.",
            buttons,
            false
        );

        lv_obj_set_width(confirmationBox, 390);

        lv_obj_add_event_cb(
            confirmationBox,
            confirmationEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr
        );

        lv_obj_center(confirmationBox);
    }

    void showCalibrationConfirmation(
        ArrowLabUI::LoadSide side
    )
    {
        if (confirmationBox != nullptr) {
            return;
        }

        pendingSide = side;
        pendingAction = ConfirmationAction::Calibration;

        static const char *buttons[] = {
            "CANCEL",
            "CALIBRATE",
            ""
        };

        const bool isLeft =
            side == ArrowLabUI::LoadSide::Left;

        char message[160];
        snprintf(
            message,
            sizeof(message),
            "Keep the calibration platform on %s.\n"
            "Place the %.1f g reference weight on it "
            "and allow it to settle.",
            isLeft ? "LEFT" : "RIGHT",
            calibrationReferenceGrams
        );

        confirmationBox = lv_msgbox_create(
            nullptr,
            isLeft ? "CALIBRATE LEFT" : "CALIBRATE RIGHT",
            message,
            buttons,
            false
        );

        lv_obj_set_width(confirmationBox, 410);
        lv_obj_add_event_cb(
            confirmationBox,
            confirmationEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr
        );
        lv_obj_center(confirmationBox);
    }

    void tareLeftButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showTareConfirmation(
                ArrowLabUI::LoadSide::Left
            );
        }
    }

    void tareRightButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showTareConfirmation(
                ArrowLabUI::LoadSide::Right
            );
        }
    }

    void calibrationLeftButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showCalibrationConfirmation(
                ArrowLabUI::LoadSide::Left
            );
        }
    }

    void calibrationRightButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showCalibrationConfirmation(
                ArrowLabUI::LoadSide::Right
            );
        }
    }

}

namespace ArrowLabUI
{
    void create()
    {
        lv_obj_t *screen = lv_scr_act();

        lv_obj_set_style_bg_color(
            screen,
            lv_color_hex(COLOUR_BACKGROUND),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        // Header
        lv_obj_t *header = lv_obj_create(screen);
        lv_obj_set_size(header, 480, 44);
        lv_obj_set_pos(header, 0, 0);

        lv_obj_set_style_bg_color(
            header,
            lv_color_hex(COLOUR_HEADER),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
        lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *title = createTextLabel(
            header,
            "ArrowLab",
            &lv_font_montserrat_20,
            lv_color_hex(COLOUR_TEXT)
        );

        lv_obj_align(title, LV_ALIGN_LEFT_MID, 18, 0);

        lv_obj_t *version = createTextLabel(
            header,
            "v0.1 DEV",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED)
        );

        lv_obj_align(version, LV_ALIGN_RIGHT_MID, -18, 0);

        // Reading panels
        leftPanel = createReadingPanel(
            screen,
            "LEFT LOAD",
            18,
            tareLeftButtonEvent,
            calibrationLeftButtonEvent
        );

        rightPanel = createReadingPanel(
            screen,
            "RIGHT LOAD",
            248,
            tareRightButtonEvent,
            calibrationRightButtonEvent
        );

        // Bottom status bar
        lv_obj_t *statusBar = lv_obj_create(screen);
        lv_obj_set_size(statusBar, 480, 58);
        lv_obj_align(statusBar, LV_ALIGN_BOTTOM_MID, 0, 0);

        lv_obj_set_style_bg_color(
            statusBar,
            lv_color_hex(COLOUR_HEADER),
            LV_PART_MAIN
        );

        lv_obj_set_style_bg_opa(statusBar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(statusBar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(statusBar, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(statusBar, 0, LV_PART_MAIN);
        lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *statusHeading = createTextLabel(
            statusBar,
            "STATUS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED)
        );

        lv_obj_align(statusHeading, LV_ALIGN_LEFT_MID, 18, -11);

        statusLabel = createTextLabel(
            statusBar,
            "Display initialized",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT)
        );

        lv_obj_align(statusLabel, LV_ALIGN_LEFT_MID, 18, 12);

        stateLabel = createTextLabel(
            statusBar,
            "READY",
            &lv_font_montserrat_16,
            lv_color_hex(0x4CD964)
        );

        lv_obj_align(stateLabel, LV_ALIGN_RIGHT_MID, -18, 0);
    }

    void setTareCallback(TareCallback callback)
    {
        tareCallback = callback;
    }

    void setCalibrationCallback(
        CalibrationCallback callback
    )
    {
        calibrationCallback = callback;
    }

    void setCalibrationReferenceGrams(float grams)
    {
        calibrationReferenceGrams = grams;
    }

    void setLeftReading(const char *text)
    {
        if (leftPanel.valueLabel != nullptr) {
            lv_label_set_text(
                leftPanel.valueLabel,
                text
            );
        }
    }

    void setRightReading(const char *text)
    {
        if (rightPanel.valueLabel != nullptr) {
            lv_label_set_text(
                rightPanel.valueLabel,
                text
            );
        }
    }

    void setLoadUnit(
        LoadSide side,
        const char *text
    )
    {
        lv_obj_t *label =
            side == LoadSide::Left
                ? leftPanel.unitLabel
                : rightPanel.unitLabel;

        if (label != nullptr) {
            lv_label_set_text(label, text);
        }
    }

    void setLoadStatus(
        LoadSide side,
        bool tareComplete,
        bool userTareConfirmed,
        bool calibrationInProgress,
        bool calibrated
    )
    {
        lv_obj_t *label =
            side == LoadSide::Left
                ? leftPanel.statusLabel
                : rightPanel.statusLabel;

        if (label == nullptr) {
            return;
        }

        char text[32];

        const char *tareText =
            !tareComplete
                ? "TARING"
                : (userTareConfirmed ? "TARE OK" : "TARE REQ");

        const char *calibrationText =
            calibrationInProgress
                ? "CAL..."
                : (calibrated ? "CAL OK" : "CAL --");

        snprintf(
            text,
            sizeof(text),
            "%s  %s",
            tareText,
            calibrationText
        );

        lv_label_set_text(label, text);

        ReadingPanelRefs &panel =
            side == LoadSide::Left
                ? leftPanel
                : rightPanel;

        if (panel.tareButton != nullptr) {
            if (calibrationInProgress) {
                lv_obj_add_state(
                    panel.tareButton,
                    LV_STATE_DISABLED
                );
            } else {
                lv_obj_clear_state(
                    panel.tareButton,
                    LV_STATE_DISABLED
                );
            }
        }

        if (panel.calibrationButton != nullptr) {
            if (
                !tareComplete
                || !userTareConfirmed
                || calibrationInProgress
            ) {
                lv_obj_add_state(
                    panel.calibrationButton,
                    LV_STATE_DISABLED
                );
            } else {
                lv_obj_clear_state(
                    panel.calibrationButton,
                    LV_STATE_DISABLED
                );
            }
        }
    }

    void setStatus(const char *text)
    {
        if (statusLabel != nullptr) {
            lv_label_set_text(statusLabel, text);
        }
    }

    void setState(const char *text, lv_color_t colour)
    {
        if (stateLabel != nullptr) {
            lv_label_set_text(stateLabel, text);
            lv_obj_set_style_text_color(
                stateLabel,
                colour,
                LV_PART_MAIN
            );
        }
    }
}
