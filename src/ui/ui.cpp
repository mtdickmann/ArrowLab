#include "ui.h"

#include <cstdio>
#include <cstdlib>

namespace
{
    // Main colours
    constexpr uint32_t COLOUR_BACKGROUND = 0x101419;
    constexpr uint32_t COLOUR_PANEL = 0x1A2027;
    constexpr uint32_t COLOUR_HEADER = 0x141A20;
    constexpr uint32_t COLOUR_BORDER = 0x303A44;
    constexpr uint32_t COLOUR_TEXT = 0xF2F5F7;
    constexpr uint32_t COLOUR_MUTED = 0x9AA6B2;
    constexpr uint32_t COLOUR_ACCENT = 0x4EA3FF;
    constexpr uint32_t COLOUR_OK = 0x4CD964;
    constexpr uint32_t COLOUR_REQUIRED = 0xFFB020;

    struct ReadingPanelRefs
    {
        lv_obj_t *panel = nullptr;
        lv_obj_t *valueLabel = nullptr;
        lv_obj_t *unitLabel = nullptr;
        lv_obj_t *statusLabel = nullptr;
        lv_obj_t *settleBar = nullptr;
        lv_obj_t *tareButton = nullptr;
        lv_obj_t *calibrationButton = nullptr;
    };

    ReadingPanelRefs leftPanel;
    ReadingPanelRefs rightPanel;

    lv_obj_t *statusLabel = nullptr;
    lv_obj_t *stateLabel = nullptr;
    lv_obj_t *headerContextLabel = nullptr;
    lv_obj_t *confirmationBox = nullptr;
    lv_obj_t *helpBox = nullptr;
    lv_obj_t *currentPage = nullptr;
    lv_obj_t *homePage = nullptr;
    lv_obj_t *settingsPage = nullptr;
    lv_obj_t *calibrationPage = nullptr;
    lv_obj_t *diagnosticsMenuPage = nullptr;
    lv_obj_t *diagnosticSidePage = nullptr;
    lv_obj_t *diagnosticsPage = nullptr;
    lv_obj_t *homeCalibrationLabel = nullptr;
    lv_obj_t *homeHealthLabel = nullptr;
    lv_obj_t *faultBar = nullptr;
    lv_obj_t *faultLabel = nullptr;
    lv_obj_t *settingsCalibrationLabel = nullptr;
    lv_obj_t *settingsCalibrationButton = nullptr;
    lv_obj_t *diagnosticsButton = nullptr;
    lv_obj_t *diagnosticSideLabel = nullptr;
    lv_obj_t *diagnosticMassLabel = nullptr;
    lv_obj_t *diagnosticMassButton = nullptr;
    lv_obj_t *diagnosticStatusLabel = nullptr;
    lv_obj_t *diagnosticProgressBar = nullptr;
    lv_obj_t *diagnosticStartZeroButton = nullptr;
    lv_obj_t *diagnosticStartLoadButton = nullptr;
    lv_obj_t *diagnosticCancelButton = nullptr;
    lv_obj_t *diagnosticFinishButton = nullptr;
    lv_obj_t *diagnosticHostLabel = nullptr;
    lv_obj_t *massInputBox = nullptr;
    lv_obj_t *massInputTextArea = nullptr;
    lv_obj_t *diagnosticConfirmBox = nullptr;
    bool developerMode = false;
    bool diagnosticSideSelected = false;
    bool diagnosticRunActive = false;
    bool diagnosticAwaitingSave = false;
    bool diagnosticUseAutomaticInstruction = true;
    bool diagnosticHostConnected = false;
    bool leftCalibrationSetupActive = false;
    bool rightCalibrationSetupActive = false;
    bool leftCalibrationReady = false;
    bool rightCalibrationReady = false;
    uint32_t developerPressStart = 0;
    bool developerPressActive = false;
    constexpr uint32_t DEVELOPER_REVEAL_HOLD_MS = 2000;

    enum class ConfirmationAction
    {
        Tare,
        Calibration
    };

    ArrowLabUI::TareCallback tareCallback = nullptr;
    ArrowLabUI::CalibrationCallback calibrationCallback = nullptr;
    ArrowLabUI::DiagnosticStartCallback diagnosticStartCallback = nullptr;
    ArrowLabUI::DiagnosticCancelCallback diagnosticCancelCallback = nullptr;
    ArrowLabUI::DiagnosticFinishCallback diagnosticFinishCallback = nullptr;
    ArrowLabUI::LoadSide diagnosticSide = ArrowLabUI::LoadSide::Left;
    float diagnosticMassGrams = 0.0f;
    bool diagnosticPendingZeroRun = false;
    float calibrationReferenceGrams = 0.0f;
    enum class MassInputPurpose
    {
        DiagnosticLoad,
        Calibration
    };
    MassInputPurpose massInputPurpose = MassInputPurpose::DiagnosticLoad;
    ArrowLabUI::LoadSide massInputSide = ArrowLabUI::LoadSide::Left;
    ArrowLabUI::LoadSide pendingSide =
        ArrowLabUI::LoadSide::Left;
    ConfirmationAction pendingAction =
        ConfirmationAction::Tare;

    void stylePanel(lv_obj_t *panel)
    {
        lv_obj_set_style_bg_color(
            panel,
            lv_color_hex(COLOUR_PANEL),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(
            panel,
            LV_OPA_COVER,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            panel,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN);

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
        lv_color_t colour)
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
        lv_event_cb_t calibrationEventCallback)
    {
        ReadingPanelRefs refs;

        lv_obj_t *panel = lv_obj_create(parent);
        refs.panel = panel;
        lv_obj_set_size(panel, 214, 148);
        lv_obj_set_pos(panel, xPosition, 10);
        stylePanel(panel);

        lv_obj_t *title = createTextLabel(
            panel,
            titleText,
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_MUTED));

        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

        refs.statusLabel = createTextLabel(
            panel,
            "TARING  CAL --",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));

        lv_obj_align(
            refs.statusLabel,
            LV_ALIGN_TOP_MID,
            0,
            30);

        lv_obj_t *divider = lv_obj_create(panel);
        lv_obj_set_size(divider, 170, 2);
        lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 51);

        lv_obj_set_style_bg_color(
            divider,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);

        lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(divider, 1, LV_PART_MAIN);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);

        refs.settleBar = lv_bar_create(panel);
        lv_obj_set_size(refs.settleBar, 170, 5);
        lv_obj_align(refs.settleBar, LV_ALIGN_TOP_MID, 0, 55);
        lv_bar_set_range(refs.settleBar, 0, 100);
        lv_bar_set_value(refs.settleBar, 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(
            refs.settleBar,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            refs.settleBar,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_INDICATOR);
        lv_obj_add_flag(refs.settleBar, LV_OBJ_FLAG_HIDDEN);

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
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            readingRow,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            readingRow,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            readingRow,
            LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(
            readingRow,
            LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            readingRow,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        refs.valueLabel = createTextLabel(
            readingRow,
            "0",
            &lv_font_montserrat_30,
            lv_color_hex(COLOUR_TEXT));

        refs.unitLabel = createTextLabel(
            readingRow,
            "RAW",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));

        refs.tareButton = lv_btn_create(panel);
        lv_obj_set_size(refs.tareButton, 68, 28);
        lv_obj_align(
            refs.tareButton,
            LV_ALIGN_BOTTOM_LEFT,
            10,
            -6);
        lv_obj_set_style_radius(
            refs.tareButton,
            7,
            LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            refs.tareButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            refs.tareButton,
            2,
            LV_PART_MAIN);
        lv_obj_add_event_cb(
            refs.tareButton,
            tareEventCallback,
            LV_EVENT_CLICKED,
            nullptr);

        lv_obj_t *tareLabel = createTextLabel(
            refs.tareButton,
            "TARE",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(tareLabel);

        refs.calibrationButton = lv_btn_create(panel);
        lv_obj_set_size(refs.calibrationButton, 68, 28);
        lv_obj_align(
            refs.calibrationButton,
            LV_ALIGN_BOTTOM_LEFT,
            84,
            -6);
        lv_obj_set_style_radius(
            refs.calibrationButton,
            7,
            LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            refs.calibrationButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            refs.calibrationButton,
            2,
            LV_PART_MAIN);
        lv_obj_add_event_cb(
            refs.calibrationButton,
            calibrationEventCallback,
            LV_EVENT_CLICKED,
            nullptr);

        lv_obj_t *calibrationLabel = createTextLabel(
            refs.calibrationButton,
            "CAL",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(calibrationLabel);

        return refs;
    }

    void confirmationEvent(lv_event_t *event)
    {
        lv_obj_t *messageBox =
            lv_event_get_current_target(event);

        const uint16_t selectedButton =
            lv_msgbox_get_active_btn(messageBox);

        if (selectedButton == 1)
        {
            if (
                pendingAction == ConfirmationAction::Tare && tareCallback != nullptr)
            {
                tareCallback(pendingSide);
            }
            else if (
                pendingAction == ConfirmationAction::Calibration && calibrationCallback != nullptr)
            {
                calibrationCallback(pendingSide, 0.0f);
            }
        }

        confirmationBox = nullptr;
        lv_msgbox_close(messageBox);
    }

    void showTareConfirmation(ArrowLabUI::LoadSide side)
    {
        if (confirmationBox != nullptr)
        {
            return;
        }

        pendingSide = side;
        pendingAction = ConfirmationAction::Tare;

        static const char *buttons[] = {
            "CANCEL",
            "TARE",
            ""};

        const bool isLeft =
            side == ArrowLabUI::LoadSide::Left;

        confirmationBox = lv_msgbox_create(
            nullptr,
            isLeft ? "TARE LEFT" : "TARE RIGHT",
            isLeft
                ? "Place calibration platform only on LEFT load.\nEnsure the setup is stable before confirming."
                : "Place calibration platform only on RIGHT load.\nEnsure the setup is stable before confirming.",
            buttons,
            false);

        lv_obj_set_width(confirmationBox, 390);

        lv_obj_add_event_cb(
            confirmationBox,
            confirmationEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);

        lv_obj_center(confirmationBox);
    }

    void showCalibrationConfirmation(
        ArrowLabUI::LoadSide side)
    {
        if (confirmationBox != nullptr)
        {
            return;
        }

        pendingSide = side;
        pendingAction = ConfirmationAction::Calibration;

        static const char *buttons[] = {
            "CANCEL",
            "CALIBRATE",
            ""};

        const bool isLeft =
            side == ArrowLabUI::LoadSide::Left;

        char message[160];
        snprintf(
            message,
            sizeof(message),
            "Keep the %.1f g reference weight stable on %s.\n"
            "CALIBRATE starts the 30-second stabilization, then "
            "records and stores the new factor.",
            calibrationReferenceGrams,
            isLeft ? "LEFT" : "RIGHT");

        confirmationBox = lv_msgbox_create(
            nullptr,
            isLeft ? "CALIBRATE LEFT" : "CALIBRATE RIGHT",
            message,
            buttons,
            false);

        lv_obj_set_width(confirmationBox, 410);
        lv_obj_add_event_cb(
            confirmationBox,
            confirmationEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);
        lv_obj_center(confirmationBox);
    }

    void tareLeftButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        {
            showTareConfirmation(
                ArrowLabUI::LoadSide::Left);
        }
    }

    void tareRightButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        {
            showTareConfirmation(
                ArrowLabUI::LoadSide::Right);
        }
    }

    void showCalibrationMassInput(ArrowLabUI::LoadSide side);

    void handleCalibrationButton(ArrowLabUI::LoadSide side)
    {
        const bool setupActive = side == ArrowLabUI::LoadSide::Left
            ? leftCalibrationSetupActive
            : rightCalibrationSetupActive;
        const bool ready = side == ArrowLabUI::LoadSide::Left
            ? leftCalibrationReady
            : rightCalibrationReady;

        if (ready) {
            showCalibrationConfirmation(side);
        } else if (!setupActive) {
            showCalibrationMassInput(side);
        }
    }

    void calibrationLeftButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        {
            handleCalibrationButton(ArrowLabUI::LoadSide::Left);
        }
    }

    void calibrationRightButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED)
        {
            handleCalibrationButton(ArrowLabUI::LoadSide::Right);
        }
    }


    void showPage(lv_obj_t *page)
    {
        lv_obj_add_flag(homePage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(settingsPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(calibrationPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(diagnosticsMenuPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(diagnosticSidePage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(diagnosticsPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN);
        currentPage = page;

        if (headerContextLabel != nullptr) {
            const char *title = "";
            if (page == homePage) {
                title = "HOME";
            } else if (page == settingsPage) {
                title = "SETTINGS";
            } else if (page == calibrationPage) {
                title = "CALIBRATION";
            } else if (page == diagnosticsMenuPage) {
                title = "DIAGNOSTICS";
            } else if (page == diagnosticSidePage) {
                title = "CREEP TEST";
            } else if (page == diagnosticsPage) {
                title = "CREEP DIAGNOSTIC";
            }
            lv_label_set_text(headerContextLabel, title);
        }
    }

    void helpCloseEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        helpBox = nullptr;
        lv_msgbox_close(lv_event_get_current_target(event));
    }

    void helpButtonEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) != LV_EVENT_CLICKED
            || helpBox != nullptr
        ) {
            return;
        }

        const char *title = "ARROWLAB HELP";
        const char *message =
            "Follow the highlighted next action. Red faults must be "
            "corrected before measurement.";

        if (currentPage == calibrationPage) {
            title = "CALIBRATION HELP";
            message =
                "For either side: fit the empty platform and TARE. "
                "Press CAL, enter the actual reference mass, then place "
                "that weight. When prompted, press CAL again to start "
                "the 30-second stabilization and calibration.";
        } else if (currentPage == diagnosticSidePage) {
            title = "CREEP TEST HELP";
            message =
                "Choose one raw channel. Left and Right runs are "
                "independent and never change operational calibration.";
        } else if (currentPage == diagnosticsPage) {
            title = "CREEP DIAGNOSTIC HELP";
            message =
                "Start the PC logger first. Follow the NEXT/WAIT line. "
                "ZERO BASE records unloaded raw evidence. SET MASS and "
                "LOAD TEST record loaded raw evidence. "
                "Do not disturb an active run.";
        }

        static const char *buttons[] = {"OK", ""};
        helpBox = lv_msgbox_create(
            nullptr,
            title,
            message,
            buttons,
            false);
        lv_obj_set_width(helpBox, 420);
        lv_obj_add_event_cb(
            helpBox,
            helpCloseEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);
        lv_obj_center(helpBox);
    }

    void homeButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(homePage);
        }
    }

    void settingsButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(settingsPage);
        }
    }

    void calibrationPageButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(calibrationPage);
        }
    }

    void closeInformationBoxEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        lv_msgbox_close(lv_event_get_current_target(event));
    }

    void diagnosticsButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(diagnosticsMenuPage);
        }
    }

    void diagnosticBackEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) == LV_EVENT_CLICKED
            && !diagnosticRunActive
            && !diagnosticAwaitingSave
        ) {
            showPage(diagnosticSidePage);
        }
    }

    void diagnosticsMenuBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(settingsPage);
        }
    }

    void diagnosticToolEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(diagnosticSidePage);
        }
    }

    void diagnosticSideBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(diagnosticsMenuPage);
        }
    }

    void refreshDiagnosticSideLabel()
    {
        if (diagnosticSideLabel == nullptr) {
            return;
        }

        if (!diagnosticSideSelected) {
            lv_label_set_text(
                diagnosticSideLabel,
                "SIDE: --");
            return;
        }

        const char *text =
            diagnosticSide == ArrowLabUI::LoadSide::Left
                ? "LEFT RAW LOGGER"
                : "RIGHT RAW LOGGER";
        lv_label_set_text(
            diagnosticSideLabel,
            text);
    }

    void refreshDiagnosticControls()
    {
        const bool canConfigure =
            !diagnosticRunActive
            && !diagnosticAwaitingSave
            && diagnosticSideSelected;

        const bool canConfigureLoad = canConfigure;

        const bool canLoad =
            canConfigureLoad
            && diagnosticMassGrams > 0.0f;

        if (diagnosticStartZeroButton != nullptr) {
            if (canConfigure) {
                lv_obj_clear_state(
                    diagnosticStartZeroButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(
                    diagnosticStartZeroButton,
                    LV_STATE_DISABLED);
            }
        }

        if (diagnosticMassButton != nullptr) {
            if (canConfigureLoad) {
                lv_obj_clear_state(
                    diagnosticMassButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(
                    diagnosticMassButton,
                    LV_STATE_DISABLED);
            }
        }

        if (diagnosticStartLoadButton != nullptr) {
            if (canLoad) {
                lv_obj_clear_state(
                    diagnosticStartLoadButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(
                    diagnosticStartLoadButton,
                    LV_STATE_DISABLED);
            }
        }

        if (diagnosticCancelButton != nullptr) {
            if (diagnosticRunActive && !diagnosticAwaitingSave) {
                lv_obj_clear_state(
                    diagnosticCancelButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(
                    diagnosticCancelButton,
                    LV_STATE_DISABLED);
            }
        }

        if (diagnosticFinishButton != nullptr) {
            if (
                !diagnosticRunActive
                && !diagnosticAwaitingSave
                && diagnosticSideSelected
            ) {
                lv_obj_clear_state(
                    diagnosticFinishButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(
                    diagnosticFinishButton,
                    LV_STATE_DISABLED);
            }
        }
    }

    void refreshDiagnosticInstruction()
    {
        if (
            diagnosticStatusLabel == nullptr
            || !diagnosticUseAutomaticInstruction
        ) {
            return;
        }

        const char *instruction = "NEXT: Select a load cell";

        if (diagnosticSideSelected) {
            if (diagnosticMassGrams <= 0.0f) {
                instruction =
                    "NEXT: ZERO BASE or SET MASS for a load test";
            } else {
                instruction =
                    "NEXT: Keep test mass off platform; press LOAD TEST";
            }
        }

        lv_label_set_text(diagnosticStatusLabel, instruction);
    }

    void diagnosticSideEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
            return;
        }

        diagnosticSide =
            static_cast<ArrowLabUI::LoadSide>(
                reinterpret_cast<uintptr_t>(
                    lv_event_get_user_data(event)));
        diagnosticSideSelected = true;
        diagnosticMassGrams = 0.0f;
        if (diagnosticMassLabel != nullptr) {
            lv_label_set_text(diagnosticMassLabel, "MASS: -- g");
        }
        diagnosticUseAutomaticInstruction = true;
        refreshDiagnosticSideLabel();
        refreshDiagnosticControls();
        refreshDiagnosticInstruction();
        showPage(diagnosticsPage);
    }

    void closeMassInput()
    {
        if (massInputBox != nullptr) {
            lv_obj_del_async(massInputBox);
            massInputBox = nullptr;
            massInputTextArea = nullptr;
        }
    }

    void massKeyboardEvent(lv_event_t *event)
    {
        const lv_event_code_t code =
            lv_event_get_code(event);

        if (code == LV_EVENT_CANCEL) {
            closeMassInput();
            return;
        }

        if (code != LV_EVENT_READY) {
            return;
        }

        const char *text =
            lv_textarea_get_text(massInputTextArea);
        const float value = std::strtof(text, nullptr);

        if (value <= 0.0f || value > 1850.0f) {
            return;
        }

        if (massInputPurpose == MassInputPurpose::Calibration) {
            calibrationReferenceGrams = value;
            if (calibrationCallback != nullptr) {
                calibrationCallback(massInputSide, value);
            }
        } else {
            diagnosticMassGrams = value;

            char label[40];
            snprintf(
                label,
                sizeof(label),
                "MASS: %.3f g",
                diagnosticMassGrams);
            lv_label_set_text(diagnosticMassLabel, label);
            refreshDiagnosticControls();
            diagnosticUseAutomaticInstruction = true;
            refreshDiagnosticInstruction();
        }

        closeMassInput();
    }

    void createMassInput(
        MassInputPurpose purpose,
        ArrowLabUI::LoadSide side)
    {
        if (massInputBox != nullptr) {
            return;
        }

        massInputPurpose = purpose;
        massInputSide = side;

        lv_obj_t *screen = lv_scr_act();

        massInputBox = lv_obj_create(screen);
        lv_obj_set_size(massInputBox, 456, 250);
        lv_obj_center(massInputBox);
        lv_obj_set_style_bg_color(
            massInputBox,
            lv_color_hex(COLOUR_PANEL),
            LV_PART_MAIN);
        lv_obj_set_style_border_color(
            massInputBox,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(massInputBox, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_all(massInputBox, 8, LV_PART_MAIN);
        lv_obj_clear_flag(massInputBox, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *heading = createTextLabel(
            massInputBox,
            purpose == MassInputPurpose::Calibration
                ? "ENTER CALIBRATION MASS (g)"
                : "ENTER ACTUAL TEST MASS (g)",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 0);

        massInputTextArea = lv_textarea_create(massInputBox);
        lv_obj_set_size(massInputTextArea, 180, 38);
        lv_obj_align(massInputTextArea, LV_ALIGN_TOP_MID, 0, 25);
        lv_textarea_set_one_line(massInputTextArea, true);
        lv_textarea_set_accepted_chars(massInputTextArea, "0123456789.");
        lv_textarea_set_max_length(massInputTextArea, 8);

        if (
            purpose == MassInputPurpose::Calibration
            && calibrationReferenceGrams > 0.0f
        ) {
            char currentMass[16];
            snprintf(
                currentMass,
                sizeof(currentMass),
                "%.3f",
                calibrationReferenceGrams);
            lv_textarea_set_text(massInputTextArea, currentMass);
        }

        lv_obj_t *keyboard = lv_keyboard_create(massInputBox);
        lv_obj_set_size(keyboard, 430, 160);
        lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
        lv_keyboard_set_textarea(keyboard, massInputTextArea);
        lv_obj_add_event_cb(
            keyboard,
            massKeyboardEvent,
            LV_EVENT_ALL,
            nullptr);

        lv_obj_move_foreground(massInputBox);
    }

    void showCalibrationMassInput(ArrowLabUI::LoadSide side)
    {
        createMassInput(MassInputPurpose::Calibration, side);
    }

    void diagnosticMassEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) != LV_EVENT_CLICKED
            || massInputBox != nullptr
            || diagnosticRunActive
            || diagnosticAwaitingSave
            || !selectedDiagnosticZeroComplete()
            || !selectedDiagnosticCalibrated()
        ) {
            return;
        }

        createMassInput(
            MassInputPurpose::DiagnosticLoad,
            diagnosticSide);
    }

    void diagnosticConfirmEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
            return;
        }

        lv_obj_t *box =
            lv_event_get_current_target(event);
        const uint16_t selectedButton =
            lv_msgbox_get_active_btn(box);

        if (
            selectedButton == 1
            && diagnosticStartCallback != nullptr
        ) {
            diagnosticStartCallback(
                diagnosticSide,
                diagnosticMassGrams,
                diagnosticPendingZeroRun);
        }

        diagnosticConfirmBox = nullptr;
        lv_msgbox_close(box);
    }

    void showDiagnosticStartConfirmation(bool zeroRun)
    {
        if (diagnosticConfirmBox != nullptr) {
            return;
        }

        if (!diagnosticSideSelected) {
            static const char *buttons[] = {"OK", ""};
            diagnosticConfirmBox = lv_msgbox_create(
                nullptr,
                "SELECT LOAD CELL",
                "Select LEFT or RIGHT before starting a diagnostic run.",
                buttons,
                false);
            lv_obj_set_width(diagnosticConfirmBox, 390);
            lv_obj_add_event_cb(
                diagnosticConfirmBox,
                diagnosticConfirmEvent,
                LV_EVENT_VALUE_CHANGED,
                nullptr);
            lv_obj_center(diagnosticConfirmBox);
            return;
        }

        if (!zeroRun && diagnosticMassGrams <= 0.0f) {
            static const char *buttons[] = {"OK", ""};
            diagnosticConfirmBox = lv_msgbox_create(
                nullptr,
                "MASS REQUIRED",
                "Enter the actual applied mass before starting a load run.",
                buttons,
                false);
            lv_obj_set_width(diagnosticConfirmBox, 400);
            lv_obj_add_event_cb(
                diagnosticConfirmBox,
                diagnosticConfirmEvent,
                LV_EVENT_VALUE_CHANGED,
                nullptr);
            lv_obj_center(diagnosticConfirmBox);
            return;
        }

        diagnosticPendingZeroRun = zeroRun;

        static const char *buttons[] = {
            "CANCEL",
            "START",
            ""
        };

        char message[220];

        if (zeroRun) {
            snprintf(
                message,
                sizeof(message),
                "Normal fixed arrow rest only.\n"
                "Remove calibration platform and all added weight.\n"
                "START captures a private raw reference, then logging begins.");
        } else {
            snprintf(
                message,
                sizeof(message),
                "Fit the calibration platform on %s.\n"
                "Keep the %.3f g test weight OFF.\n"
                "START captures a private raw reference. Then place the weight; "
                "logging starts automatically when load is detected.",
                diagnosticSide == ArrowLabUI::LoadSide::Left
                    ? "LEFT"
                    : "RIGHT",
                diagnosticMassGrams);
        }

        diagnosticConfirmBox = lv_msgbox_create(
            nullptr,
            zeroRun ? "START ZERO BASELINE" : "START LOAD TEST",
            message,
            buttons,
            false);
        lv_obj_set_width(diagnosticConfirmBox, 430);
        lv_obj_add_event_cb(
            diagnosticConfirmBox,
            diagnosticConfirmEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);
        lv_obj_center(diagnosticConfirmBox);
    }

    void diagnosticStartZeroEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showDiagnosticStartConfirmation(true);
        }
    }

    void diagnosticStartLoadEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showDiagnosticStartConfirmation(false);
        }
    }

    void diagnosticCancelEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) == LV_EVENT_CLICKED
            && diagnosticCancelCallback != nullptr
        ) {
            diagnosticCancelCallback();
        }
    }

    void diagnosticFinishEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) == LV_EVENT_CLICKED
            && diagnosticFinishCallback != nullptr
        ) {
            diagnosticFinishCallback();
        }
    }

    void developerRevealEvent(lv_event_t *event)
    {
        const lv_event_code_t code =
            lv_event_get_code(event);

        if (code == LV_EVENT_PRESSED) {
            developerPressStart = lv_tick_get();
            developerPressActive = true;
            return;
        }

        if (code == LV_EVENT_PRESS_LOST) {
            developerPressActive = false;
            return;
        }

        if (
            code != LV_EVENT_RELEASED
            || !developerPressActive
        ) {
            return;
        }

        developerPressActive = false;

        const uint32_t heldMs =
            lv_tick_elaps(developerPressStart);

        if (heldMs < DEVELOPER_REVEAL_HOLD_MS) {
            return;
        }

        if (!developerMode) {
            developerMode = true;
            lv_obj_clear_flag(
                diagnosticsButton,
                LV_OBJ_FLAG_HIDDEN);
        }

        static const char *buttons[] = {"OK", ""};
        lv_obj_t *box = lv_msgbox_create(
            nullptr,
            "DEVELOPER MODE",
            "Developer mode enabled.\nDiagnostics is now available in Settings.",
            buttons,
            false);
        lv_obj_set_width(box, 390);
        lv_obj_add_event_cb(
            box,
            closeInformationBoxEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);
        lv_obj_center(box);
    }

    lv_obj_t *createMenuButton(
        lv_obj_t *parent,
        const char *text,
        int y,
        lv_event_cb_t callback)
    {
        lv_obj_t *button = lv_btn_create(parent);
        lv_obj_set_size(button, 420, 54);
        lv_obj_set_pos(button, 30, y);
        lv_obj_set_style_radius(button, 9, LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            button,
            lv_color_hex(COLOUR_PANEL),
            LV_PART_MAIN);
        lv_obj_set_style_border_color(
            button,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        if (callback != nullptr) {
            lv_obj_add_event_cb(
                button,
                callback,
                LV_EVENT_CLICKED,
                nullptr);
        }

        lv_obj_t *label = createTextLabel(
            button,
            text,
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 4, 0);
        return button;
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
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        // Persistent header. Version information belongs on the future
        // About screen; the top-right position is reserved for Help.
        lv_obj_t *header = lv_obj_create(screen);
        lv_obj_set_size(header, 480, 44);
        lv_obj_set_pos(header, 0, 0);
        lv_obj_set_style_bg_color(
            header,
            lv_color_hex(COLOUR_HEADER),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
        lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

        /*
         * The title has a deliberately generous invisible touch target.
         * Developer mode is revealed ONLY by LVGL's long-press event;
         * a normal tap has no action.
         */
        lv_obj_t *titleTouchTarget = lv_obj_create(header);
        lv_obj_set_size(titleTouchTarget, 170, 44);
        lv_obj_align(titleTouchTarget, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_opa(
            titleTouchTarget,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            titleTouchTarget,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            titleTouchTarget,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            titleTouchTarget,
            LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(
            titleTouchTarget,
            LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(
            titleTouchTarget,
            developerRevealEvent,
            LV_EVENT_ALL,
            nullptr);

        lv_obj_t *title = createTextLabel(
            titleTouchTarget,
            "ArrowLab",
            &lv_font_montserrat_20,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 18, 0);

        headerContextLabel = createTextLabel(
            header,
            "HOME",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(headerContextLabel, 244, 24);
        lv_obj_set_pos(headerContextLabel, 174, 11);
        lv_obj_set_style_text_align(
            headerContextLabel,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);

        lv_obj_t *helpButton = lv_btn_create(header);
        lv_obj_set_size(helpButton, 34, 30);
        lv_obj_align(helpButton, LV_ALIGN_RIGHT_MID, -14, 0);
        lv_obj_set_style_radius(helpButton, 15, LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            helpButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);
        lv_obj_add_event_cb(
            helpButton,
            helpButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *helpLabel = createTextLabel(
            helpButton,
            "?",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(helpLabel);

        homePage = lv_obj_create(screen);
        lv_obj_set_size(homePage, 480, 228);
        lv_obj_set_pos(homePage, 0, 44);
        lv_obj_set_style_bg_opa(homePage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(homePage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(homePage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(homePage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *spineButton = createMenuButton(
            homePage,
            "SPINE TEST",
            36,
            nullptr);
        lv_obj_add_state(spineButton, LV_STATE_DISABLED);

        createMenuButton(
            homePage,
            "SETTINGS",
            98,
            settingsButtonEvent);

        homeCalibrationLabel = createTextLabel(
            homePage,
            "CALIBRATION REQUIRED",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_set_pos(homeCalibrationLabel, 30, 166);

        homeHealthLabel = createTextLabel(
            homePage,
            "CHECKING LOAD CELLS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_set_pos(homeHealthLabel, 30, 190);

        settingsPage = lv_obj_create(screen);
        lv_obj_set_size(settingsPage, 480, 228);
        lv_obj_set_pos(settingsPage, 0, 44);
        lv_obj_set_style_bg_opa(settingsPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(settingsPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(settingsPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(settingsPage, LV_OBJ_FLAG_SCROLLABLE);

        createMenuButton(
            settingsPage,
            "<  HOME",
            8,
            homeButtonEvent);

        settingsCalibrationButton = createMenuButton(
            settingsPage,
            "CALIBRATION",
            70,
            calibrationPageButtonEvent);
        settingsCalibrationLabel = createTextLabel(
            settingsCalibrationButton,
            "REQUIRED",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_align(settingsCalibrationLabel, LV_ALIGN_RIGHT_MID, -6, 0);

        diagnosticsButton = createMenuButton(
            settingsPage,
            "DIAGNOSTICS  [DEV]",
            132,
            diagnosticsButtonEvent);
        lv_obj_add_flag(diagnosticsButton, LV_OBJ_FLAG_HIDDEN);

        diagnosticsMenuPage = lv_obj_create(screen);
        lv_obj_set_size(diagnosticsMenuPage, 480, 228);
        lv_obj_set_pos(diagnosticsMenuPage, 0, 44);
        lv_obj_set_style_bg_opa(
            diagnosticsMenuPage,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            diagnosticsMenuPage,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            diagnosticsMenuPage,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            diagnosticsMenuPage,
            LV_OBJ_FLAG_SCROLLABLE);

        createMenuButton(
            diagnosticsMenuPage,
            "<  SETTINGS",
            8,
            diagnosticsMenuBackEvent);
        createMenuButton(
            diagnosticsMenuPage,
            "LOAD-CELL CREEP TEST",
            70,
            diagnosticToolEvent);

        lv_obj_t *diagnosticsHint = createTextLabel(
            diagnosticsMenuPage,
            "Developer diagnostic tools",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(diagnosticsHint, 30, 150);

        diagnosticSidePage = lv_obj_create(screen);
        lv_obj_set_size(diagnosticSidePage, 480, 228);
        lv_obj_set_pos(diagnosticSidePage, 0, 44);
        lv_obj_set_style_bg_opa(
            diagnosticSidePage,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            diagnosticSidePage,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            diagnosticSidePage,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            diagnosticSidePage,
            LV_OBJ_FLAG_SCROLLABLE);

        createMenuButton(
            diagnosticSidePage,
            "<  DIAGNOSTICS",
            8,
            diagnosticSideBackEvent);

        lv_obj_t *sideHeading = createTextLabel(
            diagnosticSidePage,
            "SELECT LOAD CELL",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_pos(sideHeading, 30, 74);

        lv_obj_t *sideLeftButton = lv_btn_create(diagnosticSidePage);
        lv_obj_set_size(sideLeftButton, 200, 58);
        lv_obj_set_pos(sideLeftButton, 30, 106);
        lv_obj_add_event_cb(
            sideLeftButton,
            diagnosticSideEvent,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(LoadSide::Left)));
        lv_obj_t *diagnosticLeftLabel = createTextLabel(
            sideLeftButton,
            "LEFT\nRAW CREEP TEST",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(diagnosticLeftLabel);

        lv_obj_t *sideRightButton = lv_btn_create(diagnosticSidePage);
        lv_obj_set_size(sideRightButton, 200, 58);
        lv_obj_set_pos(sideRightButton, 250, 106);
        lv_obj_add_event_cb(
            sideRightButton,
            diagnosticSideEvent,
            LV_EVENT_CLICKED,
            reinterpret_cast<void *>(
                static_cast<uintptr_t>(LoadSide::Right)));
        lv_obj_t *diagnosticRightLabel = createTextLabel(
            sideRightButton,
            "RIGHT\nRAW CREEP TEST",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(diagnosticRightLabel);

        diagnosticsPage = lv_obj_create(screen);
        lv_obj_set_size(diagnosticsPage, 480, 228);
        lv_obj_set_pos(diagnosticsPage, 0, 44);
        lv_obj_set_style_bg_opa(
            diagnosticsPage,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            diagnosticsPage,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            diagnosticsPage,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            diagnosticsPage,
            LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *diagBack = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagBack, 86, 32);
        lv_obj_set_pos(diagBack, 14, 8);
        lv_obj_add_event_cb(
            diagBack,
            diagnosticBackEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *diagBackLabel = createTextLabel(
            diagBack,
            "< BACK",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(diagBackLabel);

        diagnosticSideLabel = createTextLabel(
            diagnosticsPage,
            "SIDE: --",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(diagnosticSideLabel, 350, 24);
        lv_obj_set_pos(diagnosticSideLabel, 110, 13);
        lv_label_set_long_mode(
            diagnosticSideLabel,
            LV_LABEL_LONG_DOT);

        diagnosticMassLabel = createTextLabel(
            diagnosticsPage,
            "MASS: -- g",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(diagnosticMassLabel, 14, 55);

        diagnosticHostLabel = createTextLabel(
            diagnosticsPage,
            "PC LOGGER OFFLINE",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_set_pos(diagnosticHostLabel, 160, 55);

        diagnosticMassButton = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagnosticMassButton, 108, 34);
        lv_obj_set_pos(diagnosticMassButton, 356, 50);
        lv_obj_add_event_cb(
            diagnosticMassButton,
            diagnosticMassEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *massButtonLabel = createTextLabel(
            diagnosticMassButton,
            "SET MASS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(massButtonLabel);

        diagnosticStartZeroButton = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagnosticStartZeroButton, 214, 38);
        lv_obj_set_pos(diagnosticStartZeroButton, 14, 88);
        lv_obj_add_event_cb(
            diagnosticStartZeroButton,
            diagnosticStartZeroEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *zeroLabel = createTextLabel(
            diagnosticStartZeroButton,
            "ZERO BASE",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(zeroLabel);

        diagnosticStartLoadButton = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagnosticStartLoadButton, 214, 38);
        lv_obj_set_pos(diagnosticStartLoadButton, 234, 88);
        lv_obj_add_event_cb(
            diagnosticStartLoadButton,
            diagnosticStartLoadEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *loadLabel = createTextLabel(
            diagnosticStartLoadButton,
            "LOAD TEST",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(loadLabel);

        diagnosticCancelButton = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagnosticCancelButton, 104, 38);
        lv_obj_set_size(diagnosticCancelButton, 104, 34);
        lv_obj_set_pos(diagnosticCancelButton, 124, 132);
        lv_obj_add_event_cb(
            diagnosticCancelButton,
            diagnosticCancelEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *cancelLabel = createTextLabel(
            diagnosticCancelButton,
            "CANCEL",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(cancelLabel);
        lv_obj_add_state(
            diagnosticCancelButton,
            LV_STATE_DISABLED);

        diagnosticFinishButton = lv_btn_create(diagnosticsPage);
        lv_obj_set_size(diagnosticFinishButton, 104, 34);
        lv_obj_set_pos(diagnosticFinishButton, 234, 132);
        lv_obj_add_event_cb(
            diagnosticFinishButton,
            diagnosticFinishEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *finishLabel = createTextLabel(
            diagnosticFinishButton,
            "DONE",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(finishLabel);
        lv_obj_add_state(
            diagnosticFinishButton,
            LV_STATE_DISABLED);

        diagnosticStatusLabel = createTextLabel(
            diagnosticsPage,
            "Raw logger: no operational TARE or CAL is used",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_pos(diagnosticStatusLabel, 14, 168);
        lv_obj_set_width(diagnosticStatusLabel, 438);
        lv_label_set_long_mode(
            diagnosticStatusLabel,
            LV_LABEL_LONG_WRAP);

        diagnosticProgressBar = lv_bar_create(diagnosticsPage);
        lv_obj_set_size(diagnosticProgressBar, 438, 12);
        lv_obj_set_pos(diagnosticProgressBar, 14, 199);
        lv_bar_set_range(diagnosticProgressBar, 0, 100);
        lv_bar_set_value(
            diagnosticProgressBar,
            0,
            LV_ANIM_OFF);
        lv_obj_set_style_bg_color(
            diagnosticProgressBar,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            diagnosticProgressBar,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_INDICATOR);

        lv_obj_t *diagHint = createTextLabel(
            diagnosticsPage,
            "Progress shown above - details under Help",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(diagHint, 14, 213);

        calibrationPage = lv_obj_create(screen);
        lv_obj_set_size(calibrationPage, 480, 228);
        lv_obj_set_pos(calibrationPage, 0, 44);
        lv_obj_set_style_bg_opa(calibrationPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(calibrationPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(calibrationPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(calibrationPage, LV_OBJ_FLAG_SCROLLABLE);

        leftPanel = createReadingPanel(
            calibrationPage,
            "LEFT LOAD",
            18,
            tareLeftButtonEvent,
            calibrationLeftButtonEvent);

        rightPanel = createReadingPanel(
            calibrationPage,
            "RIGHT LOAD",
            248,
            tareRightButtonEvent,
            calibrationRightButtonEvent);

        lv_obj_t *statusBar = lv_obj_create(calibrationPage);
        lv_obj_set_size(statusBar, 480, 58);
        lv_obj_align(statusBar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(
            statusBar,
            lv_color_hex(COLOUR_HEADER),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(statusBar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(statusBar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(statusBar, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(statusBar, 0, LV_PART_MAIN);
        lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *backButton = lv_btn_create(statusBar);
        lv_obj_set_size(backButton, 76, 28);
        lv_obj_align(backButton, LV_ALIGN_LEFT_MID, 12, 0);
        lv_obj_set_style_bg_color(
            backButton,
            lv_color_hex(COLOUR_ACCENT),
            LV_PART_MAIN);
        lv_obj_add_event_cb(
            backButton,
            settingsButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *backLabel = createTextLabel(
            backButton,
            "< BACK",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(backLabel);

        statusLabel = createTextLabel(
            statusBar,
            "Display initialized",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_align(statusLabel, LV_ALIGN_LEFT_MID, 98, -9);

        stateLabel = createTextLabel(
            statusBar,
            "READY",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_OK));
        lv_obj_align(stateLabel, LV_ALIGN_LEFT_MID, 98, 13);

        // Instrument-level fault overlay. This belongs to the root
        // screen so it remains visible regardless of the active page.
        faultBar = lv_obj_create(screen);
        lv_obj_set_size(faultBar, 480, 28);
        lv_obj_align(faultBar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(
            faultBar,
            lv_color_hex(COLOUR_HEADER),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(
            faultBar,
            LV_OPA_COVER,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            faultBar,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_radius(
            faultBar,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            faultBar,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            faultBar,
            LV_OBJ_FLAG_SCROLLABLE);

        faultLabel = createTextLabel(
            faultBar,
            "LOAD CELL FAULT",
            &lv_font_montserrat_16,
            lv_color_hex(0xFF4D4D));
        lv_obj_center(faultLabel);
        lv_obj_add_flag(faultBar, LV_OBJ_FLAG_HIDDEN);

        showPage(homePage);
    }

    void setTareCallback(TareCallback callback)
    {
        tareCallback = callback;
    }

    void setCalibrationCallback(
        CalibrationCallback callback)
    {
        calibrationCallback = callback;
    }

    void setCalibrationReferenceGrams(float grams)
    {
        calibrationReferenceGrams = grams;
    }

    void setDiagnosticCallbacks(
        DiagnosticStartCallback startCallback,
        DiagnosticCancelCallback cancelCallback,
        DiagnosticFinishCallback finishCallback)
    {
        diagnosticStartCallback = startCallback;
        diagnosticCancelCallback = cancelCallback;
        diagnosticFinishCallback = finishCallback;
    }

    void setDiagnosticStatus(
        const char *text,
        uint8_t progressPercent,
        bool active,
        bool awaitingSave)
    {
        diagnosticRunActive = active;
        diagnosticAwaitingSave = awaitingSave;
        diagnosticUseAutomaticInstruction =
            text == nullptr || text[0] == '\0';

        if (diagnosticStatusLabel != nullptr) {
            if (diagnosticUseAutomaticInstruction) {
                refreshDiagnosticInstruction();
            } else {
                lv_label_set_text(
                    diagnosticStatusLabel,
                    text);
            }
        }

        if (diagnosticProgressBar != nullptr) {
            lv_bar_set_value(
                diagnosticProgressBar,
                progressPercent,
                LV_ANIM_OFF);
        }

        refreshDiagnosticSideLabel();
        refreshDiagnosticControls();
    }

    void setDiagnosticHostConnected(bool hostConnected)
    {
        diagnosticHostConnected = hostConnected;

        if (diagnosticHostLabel != nullptr) {
            lv_label_set_text(
                diagnosticHostLabel,
                hostConnected
                    ? "PC LOGGER CONNECTED"
                    : "PC LOGGER OFFLINE");
            lv_obj_set_style_text_color(
                diagnosticHostLabel,
                lv_color_hex(
                    hostConnected ? COLOUR_OK : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }

        refreshDiagnosticSideLabel();
        refreshDiagnosticControls();
        refreshDiagnosticInstruction();
    }

    void setCalibrationValidity(
        bool leftCalibrated,
        bool rightCalibrated)
    {
        const bool bothCalibrated =
            leftCalibrated && rightCalibrated;

        if (homeCalibrationLabel != nullptr)
        {
            lv_label_set_text(
                homeCalibrationLabel,
                bothCalibrated
                    ? "CALIBRATION OK"
                    : "CALIBRATION REQUIRED");
            lv_obj_set_style_text_color(
                homeCalibrationLabel,
                lv_color_hex(
                    bothCalibrated
                        ? COLOUR_OK
                        : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }

        if (settingsCalibrationLabel != nullptr)
        {
            lv_label_set_text(
                settingsCalibrationLabel,
                bothCalibrated ? "OK" : "REQUIRED");
            lv_obj_set_style_text_color(
                settingsCalibrationLabel,
                lv_color_hex(
                    bothCalibrated
                        ? COLOUR_OK
                        : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }

        if (settingsCalibrationButton != nullptr)
        {
            lv_obj_set_style_border_color(
                settingsCalibrationButton,
                lv_color_hex(
                    bothCalibrated
                        ? COLOUR_OK
                        : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }
    }

    void setLeftReading(const char *text)
    {
        if (leftPanel.valueLabel != nullptr)
        {
            lv_label_set_text(
                leftPanel.valueLabel,
                text);
        }
    }

    void setRightReading(const char *text)
    {
        if (rightPanel.valueLabel != nullptr)
        {
            lv_label_set_text(
                rightPanel.valueLabel,
                text);
        }
    }

    void setSensorHealth(bool leftLive, bool rightLive)
    {
        if (
            homeHealthLabel == nullptr
            || faultBar == nullptr
            || faultLabel == nullptr
        ) {
            return;
        }

        if (leftLive && rightLive) {
            lv_label_set_text(
                homeHealthLabel,
                "LOAD CELLS ONLINE");
            lv_obj_set_style_text_color(
                homeHealthLabel,
                lv_color_hex(COLOUR_OK),
                LV_PART_MAIN);
            lv_obj_add_flag(
                faultBar,
                LV_OBJ_FLAG_HIDDEN);
            return;
        }

        // The global fault strip carries the failure message on every
        // page; do not duplicate it in the Home-only health line.
        lv_label_set_text(homeHealthLabel, "");

        const char *faultText =
            !leftLive && !rightLive
                ? "FAULT: LEFT + RIGHT LOAD CELLS"
                : (!leftLive
                    ? "FAULT: LEFT LOAD CELL"
                    : "FAULT: RIGHT LOAD CELL");

        lv_label_set_text(faultLabel, faultText);
        lv_obj_clear_flag(
            faultBar,
            LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(faultBar);
    }

    void setLoadUnit(
        LoadSide side,
        const char *text)
    {
        lv_obj_t *label =
            side == LoadSide::Left
                ? leftPanel.unitLabel
                : rightPanel.unitLabel;

        if (label != nullptr)
        {
            lv_label_set_text(label, text);
        }
    }

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
        uint8_t settlePercent)
    {
        lv_obj_t *label =
            side == LoadSide::Left
                ? leftPanel.statusLabel
                : rightPanel.statusLabel;

        if (label == nullptr)
        {
            return;
        }

        char text[32];

        const char *tareText =
            tareInProgress
                ? "TARING"
                : (userTareConfirmed ? "TARE OK" : "TARE REQ");

        char calibrationText[16];

        if (calibrationInProgress && settleRemainingSeconds > 0) {
            snprintf(
                calibrationText,
                sizeof(calibrationText),
                "CAL %lus",
                static_cast<unsigned long>(
                    settleRemainingSeconds));
        } else if (calibrationInProgress) {
            snprintf(calibrationText, sizeof(calibrationText), "CAL...");
        } else if (calibrationReady) {
            snprintf(
                calibrationText,
                sizeof(calibrationText),
                "CAL READY");
        } else if (calibrated) {
            snprintf(
                calibrationText,
                sizeof(calibrationText),
                "CAL OK");
        } else {
            snprintf(
                calibrationText,
                sizeof(calibrationText),
                "CAL --");
        }

        snprintf(
            text,
            sizeof(text),
            "%s  %s",
            tareText,
            calibrationText);

        lv_label_set_text(label, text);

        ReadingPanelRefs &panel =
            side == LoadSide::Left
                ? leftPanel
                : rightPanel;

        if (panel.settleBar != nullptr)
        {
            if (calibrationInProgress && settleRemainingSeconds > 0) {
                lv_bar_set_value(
                    panel.settleBar,
                    settlePercent,
                    LV_ANIM_OFF);
                lv_obj_clear_flag(
                    panel.settleBar,
                    LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(
                    panel.settleBar,
                    LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (panel.tareButton != nullptr)
        {
            lv_obj_set_style_bg_color(
                panel.tareButton,
                lv_color_hex(
                    tareComplete && userTareConfirmed
                        ? COLOUR_OK
                        : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }

        if (panel.calibrationButton != nullptr)
        {
            lv_obj_set_style_bg_color(
                panel.calibrationButton,
                lv_color_hex(
                    calibrated && !calibrationSetupActive
                        ? COLOUR_OK
                        : COLOUR_REQUIRED),
                LV_PART_MAIN);
        }

        if (panel.tareButton != nullptr)
        {
            if (calibrationInProgress)
            {
                lv_obj_add_state(
                    panel.tareButton,
                    LV_STATE_DISABLED);
            }
            else
            {
                lv_obj_clear_state(
                    panel.tareButton,
                    LV_STATE_DISABLED);
            }
        }

        if (panel.calibrationButton != nullptr)
        {
            if (
                !tareComplete
                || !userTareConfirmed
                || calibrationInProgress
                || (calibrationSetupActive && !calibrationReady))
            {
                lv_obj_add_state(
                    panel.calibrationButton,
                    LV_STATE_DISABLED);
            }
            else
            {
                lv_obj_clear_state(
                    panel.calibrationButton,
                    LV_STATE_DISABLED);
            }
        }

        if (side == LoadSide::Left) {
            leftCalibrationSetupActive = calibrationSetupActive;
            leftCalibrationReady = calibrationReady;
        } else {
            rightCalibrationSetupActive = calibrationSetupActive;
            rightCalibrationReady = calibrationReady;
        }
    }

    void setStatus(const char *text)
    {
        if (statusLabel != nullptr)
        {
            lv_label_set_text(statusLabel, text);
        }
    }

    void setState(const char *text, lv_color_t colour)
    {
        if (stateLabel != nullptr)
        {
            lv_label_set_text(stateLabel, text);
            lv_obj_set_style_text_color(
                stateLabel,
                colour,
                LV_PART_MAIN);
        }
    }
}
