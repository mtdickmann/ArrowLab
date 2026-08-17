#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ArrowLabConfig.h"
#include "network/NetworkService.h"

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
        lv_obj_t *conversionLabel = nullptr;
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
    lv_obj_t *weighPage = nullptr;
    lv_obj_t *spinePage = nullptr;
    lv_obj_t *settingsPage = nullptr;
    lv_obj_t *wifiPage = nullptr;
    lv_obj_t *wifiSavedPage = nullptr;
    lv_obj_t *wifiProfilePage = nullptr;
    lv_obj_t *wifiNetworksPage = nullptr;
    lv_obj_t *wifiPasswordPage = nullptr;
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
    lv_obj_t *vieweNetworkLabel = nullptr;
    lv_obj_t *wroomNetworkLabel = nullptr;
    lv_obj_t *headerWifiButton = nullptr;
    lv_obj_t *headerWifiArcs[3] = {};
    lv_obj_t *headerWifiDot = nullptr;
    lv_obj_t *firmwareUpdateOverlay = nullptr;
    lv_obj_t *firmwareUpdateMessage = nullptr;
    lv_obj_t *firmwareUpdateDismissButton = nullptr;
    lv_obj_t *wifiConfigureButtonLabel = nullptr;
    constexpr size_t MAX_WIFI_SCAN_RESULTS = 12;
    constexpr size_t MAX_WIFI_SAVED_PROFILES = 8;
    lv_obj_t *wifiSavedStatusLabel = nullptr;
    lv_obj_t *wifiSavedList = nullptr;
    lv_obj_t *wifiSavedRows[MAX_WIFI_SAVED_PROFILES] = {};
    lv_obj_t *wifiSavedLabels[MAX_WIFI_SAVED_PROFILES] = {};
    lv_obj_t *wifiSavedForgetButtons[MAX_WIFI_SAVED_PROFILES] = {};
    lv_obj_t *wifiForgetBox = nullptr;
    lv_obj_t *wifiProfileSsidLabel = nullptr;
    lv_obj_t *wifiProfileStateLabel = nullptr;
    lv_obj_t *wifiProfilePasswordLabel = nullptr;
    lv_obj_t *wifiProfileRevealIcon = nullptr;
    lv_obj_t *wifiProfileConnectButton = nullptr;
    lv_obj_t *wifiProfileConnectLabel = nullptr;
    char wifiProfileSsid[33] = {};
    char wifiSavedSsids[MAX_WIFI_SAVED_PROFILES][33] = {};
    char wifiProfilePassword[65] = {};
    size_t wifiSavedCount = 0;
    size_t wifiForgetIndex = 0;
    bool wifiProfilePasswordRevealed = false;
    bool wifiProfileConnectionActive = false;
    lv_obj_t *wifiScanStatusLabel = nullptr;
    lv_obj_t *wifiNetworkList = nullptr;
    lv_obj_t *wifiNetworkButtons[MAX_WIFI_SCAN_RESULTS] = {};
    lv_obj_t *wifiNetworkButtonLabels[MAX_WIFI_SCAN_RESULTS] = {};
    lv_obj_t *wifiPasswordSsidLabel = nullptr;
    lv_obj_t *wifiPasswordStatusLabel = nullptr;
    lv_obj_t *wifiPasswordTextArea = nullptr;
    lv_obj_t *wifiPasswordRevealButton = nullptr;
    lv_obj_t *wifiPasswordRevealIcon = nullptr;
    lv_obj_t *wifiKeyboard = nullptr;
    bool wifiPasswordRevealed = false;
    char wifiScannedSsids[MAX_WIFI_SCAN_RESULTS][33] = {};
    size_t wifiScannedCount = 0;
    size_t wifiSelectedIndex = 0;
    lv_obj_t *weighSourceLabel = nullptr;
    lv_obj_t *weighValueLabel = nullptr;
    lv_obj_t *weighUnitLabel = nullptr;
    lv_obj_t *weighConversionLabel = nullptr;
    lv_obj_t *weighInstructionLabel = nullptr;
    lv_obj_t *weighTareLeftButton = nullptr;
    lv_obj_t *weighTareRightButton = nullptr;
    lv_obj_t *spineModeLabel = nullptr;
    lv_obj_t *spineStateLabel = nullptr;
    lv_obj_t *spineDetailLabel = nullptr;
    lv_obj_t *spineResultsLabel = nullptr;
    lv_obj_t *spineProgressBar = nullptr;
    lv_obj_t *spineStartButton = nullptr;
    lv_obj_t *spineStartButtonLabel = nullptr;
    lv_obj_t *spineMarkedButton = nullptr;
    lv_obj_t *spineMarkedButtonLabel = nullptr;
    lv_obj_t *spineCancelButton = nullptr;
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
    lv_obj_t *massInputKeyboard = nullptr;
    lv_obj_t *diagnosticConfirmBox = nullptr;
    bool developerMode =
        ArrowLabConfig::DEVELOPER_MODE_DEFAULT_ENABLED;
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
    ArrowLabUI::UnitCycleCallback unitCycleCallback = nullptr;
    ArrowLabUI::SpineStartCallback spineStartCallback = nullptr;
    ArrowLabUI::SpineControlCallback spineCancelCallback = nullptr;
    ArrowLabUI::SpineControlCallback spineConfirmClearCallback = nullptr;
    ArrowLabUI::SpineControlCallback spineConfirmZeroCallback = nullptr;
    ArrowLabUI::SpineControlCallback spineRestartCallback = nullptr;
    ArrowLabUI::SpineMarkedCallback spineMarkedCallback = nullptr;
    ArrowLabUI::WifiScanCallback wifiScanCallback = nullptr;
    ArrowLabUI::WifiConnectCallback wifiConnectCallback = nullptr;
    ArrowLabUI::WifiForgetCallback wifiForgetCallback = nullptr;
    uint8_t selectedSpinePositionCount = 1;
    float markedSpine = 0.0f;
    bool spineRunActive = false;
    bool spineClearConfirmationRequired = false;
    bool spineZeroConfirmationRequired = false;
    ArrowLabUI::LoadSide diagnosticSide = ArrowLabUI::LoadSide::Left;
    float diagnosticMassGrams = 0.0f;
    bool diagnosticPendingZeroRun = false;
    float calibrationReferenceGrams = 0.0f;
    enum class MassInputPurpose
    {
        DiagnosticLoad,
        Calibration,
        MarkedSpine
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

    void unitCycleEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) == LV_EVENT_CLICKED
            && unitCycleCallback != nullptr
        ) {
            unitCycleCallback();
        }
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
        lv_obj_set_size(readingRow, 190, 34);
        lv_obj_align(readingRow, LV_ALIGN_TOP_MID, 0, 58);
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
        lv_obj_add_flag(readingRow, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(
            readingRow,
            unitCycleEvent,
            LV_EVENT_CLICKED,
            nullptr);
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

        refs.conversionLabel = createTextLabel(
            panel,
            "",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_width(refs.conversionLabel, 190);
        lv_obj_set_style_text_align(
            refs.conversionLabel,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);
        lv_obj_align(
            refs.conversionLabel,
            LV_ALIGN_TOP_MID,
            0,
            93);

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

        const char *message = nullptr;
        if (currentPage == weighPage) {
            message = isLeft
                ? "Remove all added load from LEFT.\nEnsure the cassette is stable before confirming."
                : "Remove all added load from RIGHT.\nEnsure the cassette is stable before confirming.";
        } else {
            message = isLeft
                ? "Place calibration platform only on LEFT load.\nEnsure the setup is stable before confirming."
                : "Place calibration platform only on RIGHT load.\nEnsure the setup is stable before confirming.";
        }

        confirmationBox = lv_msgbox_create(
            nullptr,
            isLeft ? "TARE LEFT" : "TARE RIGHT",
            message,
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
    void createMassInput(MassInputPurpose purpose, ArrowLabUI::LoadSide side);

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
        lv_obj_add_flag(weighPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(spinePage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(settingsPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifiPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifiSavedPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifiProfilePage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifiNetworksPage, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(wifiPasswordPage, LV_OBJ_FLAG_HIDDEN);
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
            } else if (page == weighPage) {
                title = "WEIGH";
            } else if (page == spinePage) {
                title = selectedSpinePositionCount == 4
                    ? "SAS TEST"
                    : "SPINE TEST";
            } else if (page == settingsPage) {
                title = "SETTINGS";
            } else if (page == wifiPage) {
                title = "WI-FI & NETWORK";
            } else if (page == wifiSavedPage) {
                title = "SAVED NETWORKS";
            } else if (page == wifiProfilePage) {
                title = "NETWORK PROFILE";
            } else if (page == wifiNetworksPage) {
                title = "SELECT NETWORK";
            } else if (page == wifiPasswordPage) {
                title = "WI-FI PASSWORD";
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
        } else if (currentPage == weighPage) {
            title = "WEIGH HELP";
            message =
                "Tare empty cassettes deliberately. Place a load on LEFT, "
                "RIGHT, or both; ArrowLab selects the active cassette and "
                "adds both when required. Tap the large reading to cycle "
                "g, gr and oz.";
        } else if (currentPage == spinePage) {
            title = selectedSpinePositionCount == 4
                ? "SAS TEST HELP"
                : "SPINE TEST HELP";
            message = selectedSpinePositionCount == 4
                ? "Start empty and place the arrow when prompted. At SET "
                  "ZERO, bring the plunger to the arrow top, establish the "
                  "12.7 mm travel datum and confirm ZERO SET. Then press to "
                  "the hard stop at four 90 degree positions. Partial presses "
                  "reset automatically; RESTART retries the current position."
                : "Start empty and place the arrow when prompted. At SET "
                  "ZERO, bring the plunger to the arrow top, establish the "
                  "12.7 mm travel datum and confirm ZERO SET. Then press to "
                  "the hard stop. Partial presses reset automatically; "
                  "RESTART retries and CANCEL stops the test.";
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

    void updateHeaderWifiStrength(bool connected, int16_t rssiDbm)
    {
        uint8_t activeArcs = 0;
        uint32_t activeColour = COLOUR_REQUIRED;

        if (connected) {
            activeArcs = rssiDbm >= -55
                ? 3
                : rssiDbm >= -67
                    ? 2
                    : rssiDbm >= -75
                        ? 1
                        : 0;
            activeColour = activeArcs >= 2
                ? COLOUR_OK
                : COLOUR_REQUIRED;
        }

        for (uint8_t index = 0; index < 3; ++index) {
            if (headerWifiArcs[index] == nullptr) continue;
            lv_obj_set_style_arc_color(
                headerWifiArcs[index],
                lv_color_hex(
                    connected && index < activeArcs
                        ? activeColour
                        : COLOUR_BORDER),
                LV_PART_MAIN);
        }

        if (headerWifiDot != nullptr) {
            lv_obj_set_style_bg_color(
                headerWifiDot,
                lv_color_hex(
                    connected
                        ? activeColour
                        : 0xFF4D4D),
                LV_PART_MAIN);
        }
    }

    void wifiButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(wifiPage);
        }
    }

    void wifiBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(settingsPage);
        }
    }

    void refreshSavedWifiProfiles()
    {
        wifiSavedCount = ArrowLabNetwork::savedProfileSsids(
            wifiSavedSsids,
            MAX_WIFI_SAVED_PROFILES);
        const ArrowLabNetwork::Info network = ArrowLabNetwork::info();

        for (
            size_t index = 0;
            index < MAX_WIFI_SAVED_PROFILES;
            ++index
        ) {
            if (index >= wifiSavedCount) {
                lv_obj_add_flag(
                    wifiSavedRows[index],
                    LV_OBJ_FLAG_HIDDEN);
                continue;
            }

            char text[48];
            snprintf(
                text,
                sizeof(text),
                "%s%s",
                wifiSavedSsids[index],
                network.connected
                    && strcmp(network.ssid, wifiSavedSsids[index]) == 0
                        ? "  [ACTIVE]"
                        : "");
            lv_label_set_text(wifiSavedLabels[index], text);
            lv_obj_clear_flag(
                wifiSavedRows[index],
                LV_OBJ_FLAG_HIDDEN);
        }

        if (wifiSavedStatusLabel != nullptr) {
            char text[40];
            snprintf(
                text,
                sizeof(text),
                wifiSavedCount == 0
                    ? "NO SAVED NETWORKS"
                    : "%u SAVED NETWORK%s",
                static_cast<unsigned>(wifiSavedCount),
                wifiSavedCount == 1 ? "" : "S");
            lv_label_set_text(wifiSavedStatusLabel, text);
        }
        if (wifiSavedList != nullptr) {
            lv_obj_scroll_to_y(wifiSavedList, 0, LV_ANIM_OFF);
        }
    }

    void wifiSavedBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(wifiPage);
        }
    }

    void wifiSavedOpenEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            refreshSavedWifiProfiles();
            showPage(wifiSavedPage);
        }
    }

    void refreshWifiProfilePassword()
    {
        if (wifiProfilePasswordLabel == nullptr) return;

        if (wifiProfilePassword[0] == '\0') {
            lv_label_set_text(
                wifiProfilePasswordLabel,
                "(no password)");
            return;
        }

        if (wifiProfilePasswordRevealed) {
            lv_label_set_text(
                wifiProfilePasswordLabel,
                wifiProfilePassword);
            return;
        }

        char masked[65];
        const size_t length = strlen(wifiProfilePassword);
        const size_t visibleLength =
            length < sizeof(masked) - 1
                ? length
                : sizeof(masked) - 1;
        memset(masked, '*', visibleLength);
        masked[visibleLength] = '\0';
        lv_label_set_text(wifiProfilePasswordLabel, masked);
    }

    void wifiProfileRevealEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

        wifiProfilePasswordRevealed =
            !wifiProfilePasswordRevealed;
        refreshWifiProfilePassword();
        if (wifiProfileRevealIcon != nullptr) {
            lv_obj_set_style_text_color(
                wifiProfileRevealIcon,
                lv_color_hex(
                    wifiProfilePasswordRevealed
                        ? COLOUR_ACCENT
                        : COLOUR_MUTED),
                LV_PART_MAIN);
        }
    }

    void wifiProfileBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(wifiSavedPage);
        }
    }

    void wifiProfileConnectEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) != LV_EVENT_CLICKED
            || wifiConnectCallback == nullptr
            || wifiProfileSsid[0] == '\0'
        ) {
            return;
        }

        wifiProfileConnectionActive = true;
        lv_obj_add_state(
            wifiProfileConnectButton,
            LV_STATE_DISABLED);
        lv_label_set_text(
            wifiProfileConnectLabel,
            "CONNECTING");
        wifiConnectCallback(
            wifiProfileSsid,
            wifiProfilePassword);
    }

    void wifiSavedProfileEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

        const size_t index = reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));
        if (index >= wifiSavedCount) return;

        snprintf(
            wifiProfileSsid,
            sizeof(wifiProfileSsid),
            "%s",
            wifiSavedSsids[index]);
        lv_label_set_text(
            wifiProfileSsidLabel,
            wifiProfileSsid);

        const ArrowLabNetwork::Info network =
            ArrowLabNetwork::info();
        const bool active =
            network.connected
            && strcmp(network.ssid, wifiSavedSsids[index]) == 0;
        lv_label_set_text(
            wifiProfileStateLabel,
            active ? "STATUS: ACTIVE" : "STATUS: SAVED");
        lv_obj_set_style_text_color(
            wifiProfileStateLabel,
            lv_color_hex(active ? COLOUR_OK : COLOUR_MUTED),
            LV_PART_MAIN);
        wifiProfileConnectionActive = false;
        lv_label_set_text(
            wifiProfileConnectLabel,
            active ? "CONNECTED" : "CONNECT");
        if (active) {
            lv_obj_add_state(
                wifiProfileConnectButton,
                LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(
                wifiProfileConnectButton,
                LV_STATE_DISABLED);
        }

        wifiProfilePassword[0] = '\0';
        ArrowLabNetwork::savedPasswordForSsid(
            wifiSavedSsids[index],
            wifiProfilePassword,
            sizeof(wifiProfilePassword));
        wifiProfilePasswordRevealed = false;
        refreshWifiProfilePassword();
        lv_obj_set_style_text_color(
            wifiProfileRevealIcon,
            lv_color_hex(COLOUR_MUTED),
            LV_PART_MAIN);
        showPage(wifiProfilePage);
    }

    void wifiForgetConfirmEvent(lv_event_t *event)
    {
        lv_obj_t *messageBox = lv_event_get_current_target(event);
        const uint16_t selectedButton =
            lv_msgbox_get_active_btn(messageBox);

        if (
            selectedButton == 1
            && wifiForgetCallback != nullptr
            && wifiForgetIndex < wifiSavedCount
        ) {
            const bool removed =
                wifiForgetCallback(wifiSavedSsids[wifiForgetIndex]);
            if (wifiSavedStatusLabel != nullptr && !removed) {
                lv_label_set_text(
                    wifiSavedStatusLabel,
                    "FORGET FAILED - WROOM MUST BE ONLINE");
                lv_obj_set_style_text_color(
                    wifiSavedStatusLabel,
                    lv_color_hex(0xFF4D4D),
                    LV_PART_MAIN);
            }
            if (removed) refreshSavedWifiProfiles();
        }

        wifiForgetBox = nullptr;
        lv_msgbox_close(messageBox);
    }

    void wifiSavedForgetEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) != LV_EVENT_CLICKED
            || wifiForgetBox != nullptr
        ) {
            return;
        }

        wifiForgetIndex = reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));
        if (wifiForgetIndex >= wifiSavedCount) return;

        const ArrowLabNetwork::Info network = ArrowLabNetwork::info();
        const bool active =
            network.connected
            && strcmp(
                network.ssid,
                wifiSavedSsids[wifiForgetIndex]) == 0;

        char message[180];
        snprintf(
            message,
            sizeof(message),
            active
                ? "Forget network %s?\nArrowLab stays connected for now, "
                  "but this network will not be available after restart."
                : "Forget network %s?",
            wifiSavedSsids[wifiForgetIndex]);

        static const char *buttons[] = {
            "CANCEL",
            "FORGET",
            ""};
        wifiForgetBox = lv_msgbox_create(
            nullptr,
            active ? "FORGET ACTIVE NETWORK?" : "FORGET NETWORK?",
            message,
            buttons,
            false);
        lv_obj_set_width(wifiForgetBox, 420);
        lv_obj_add_event_cb(
            wifiForgetBox,
            wifiForgetConfirmEvent,
            LV_EVENT_VALUE_CHANGED,
            nullptr);
        lv_obj_center(wifiForgetBox);
    }

    void wifiStartScan()
    {
        showPage(wifiNetworksPage);
        if (wifiNetworkList != nullptr) {
            lv_obj_scroll_to_y(
                wifiNetworkList,
                0,
                LV_ANIM_OFF);
        }
        if (wifiScanStatusLabel != nullptr) {
            lv_label_set_text(wifiScanStatusLabel, "SCANNING...");
        }
        for (auto *button : wifiNetworkButtons) {
            if (button != nullptr) {
                lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (wifiScanCallback != nullptr) wifiScanCallback();
    }

    void wifiConfigureEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) wifiStartScan();
    }

    void wifiRefreshEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) wifiStartScan();
    }

    void wifiNetworksBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) showPage(wifiPage);
    }

    void wifiPasswordBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(wifiNetworksPage);
        }
    }

    void wifiPasswordRevealEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) != LV_EVENT_CLICKED
            || wifiPasswordTextArea == nullptr
        ) {
            return;
        }

        wifiPasswordRevealed = !wifiPasswordRevealed;
        lv_textarea_set_password_mode(
            wifiPasswordTextArea,
            !wifiPasswordRevealed);
        if (wifiPasswordRevealIcon != nullptr) {
            lv_obj_set_style_text_color(
                wifiPasswordRevealIcon,
                lv_color_hex(
                    wifiPasswordRevealed ? COLOUR_ACCENT : COLOUR_MUTED),
                LV_PART_MAIN);
        }
    }

    void wifiNetworkSelectEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        wifiSelectedIndex = reinterpret_cast<uintptr_t>(
            lv_event_get_user_data(event));
        if (wifiSelectedIndex >= wifiScannedCount) return;

        if (wifiPasswordSsidLabel != nullptr) {
            char text[64];
            snprintf(
                text,
                sizeof(text),
                "CONNECT TO: %s",
                wifiScannedSsids[wifiSelectedIndex]);
            lv_label_set_text(wifiPasswordSsidLabel, text);
        }
        char rememberedPassword[65] = {};
        const bool remembered =
            ArrowLabNetwork::savedPasswordForSsid(
                wifiScannedSsids[wifiSelectedIndex],
                rememberedPassword,
                sizeof(rememberedPassword));
        if (wifiPasswordStatusLabel != nullptr) {
            lv_label_set_text(
                wifiPasswordStatusLabel,
                remembered
                    ? "Saved password loaded - tap checkmark to connect"
                    : "Enter password, then tap the keyboard checkmark");
        }
        if (wifiPasswordTextArea != nullptr) {
            lv_textarea_set_text(
                wifiPasswordTextArea,
                remembered ? rememberedPassword : "");
            wifiPasswordRevealed = false;
            lv_textarea_set_password_mode(wifiPasswordTextArea, true);
        }
        if (wifiPasswordRevealIcon != nullptr) {
            lv_obj_set_style_text_color(
                wifiPasswordRevealIcon,
                lv_color_hex(COLOUR_MUTED),
                LV_PART_MAIN);
        }
        if (wifiKeyboard != nullptr) {
            lv_obj_clear_state(wifiKeyboard, LV_STATE_DISABLED);
        }
        showPage(wifiPasswordPage);
    }

    void wifiPasswordFieldEvent(lv_event_t *event)
    {
        const lv_event_code_t code = lv_event_get_code(event);
        if (
            (code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED)
            && wifiKeyboard != nullptr
        ) {
            lv_obj_clear_flag(wifiKeyboard, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void wifiKeyboardEvent(lv_event_t *event)
    {
        const lv_event_code_t code = lv_event_get_code(event);
        if (code == LV_EVENT_CANCEL) {
            lv_obj_add_flag(wifiKeyboard, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        if (
            code != LV_EVENT_READY
            || wifiConnectCallback == nullptr
            || wifiSelectedIndex >= wifiScannedCount
        ) {
            return;
        }

        wifiConnectCallback(
            wifiScannedSsids[wifiSelectedIndex],
            lv_textarea_get_text(wifiPasswordTextArea));
        lv_label_set_text(
            wifiPasswordStatusLabel,
            "Connecting ArrowLab...");
        lv_obj_add_state(wifiKeyboard, LV_STATE_DISABLED);
    }

    void weighButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
            showPage(weighPage);
        }
    }

    void openSpinePage(uint8_t positions)
    {
        selectedSpinePositionCount = positions;
        spineRunActive = false;
        if (spineModeLabel != nullptr) {
            lv_label_set_text(
                spineModeLabel,
                positions == 4 ? "4 POSITIONS / 90 DEG" : "SINGLE POSITION");
        }
        if (spineStateLabel != nullptr) lv_label_set_text(spineStateLabel, "READY");
        if (spineDetailLabel != nullptr) {
            lv_label_set_text(
                spineDetailLabel,
                "Raise plunger clear; empty both supports; press START");
        }
        if (spineResultsLabel != nullptr) lv_label_set_text(spineResultsLabel, "");
        if (spineProgressBar != nullptr) lv_bar_set_value(spineProgressBar, 0, LV_ANIM_OFF);
        showPage(spinePage);
    }

    void spineButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) openSpinePage(1);
    }

    void sasButtonEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) == LV_EVENT_CLICKED) openSpinePage(4);
    }

    void spineStartEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        if (!spineRunActive && spineStartCallback != nullptr) {
            spineStartCallback(selectedSpinePositionCount, markedSpine);
        } else if (
            spineRunActive
            && spineClearConfirmationRequired
            && spineConfirmClearCallback != nullptr
        ) {
            spineConfirmClearCallback();
        } else if (
            spineRunActive
            && spineZeroConfirmationRequired
            && spineConfirmZeroCallback != nullptr
        ) {
            spineConfirmZeroCallback();
        } else if (spineRunActive && spineRestartCallback != nullptr) {
            spineRestartCallback();
        }
    }

    void spineCancelEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        if (spineCancelCallback != nullptr) spineCancelCallback();
    }

    void spineBackEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        if (spineRunActive && spineCancelCallback != nullptr) {
            spineCancelCallback();
        }
        spineRunActive = false;
        spineClearConfirmationRequired = false;
        spineZeroConfirmationRequired = false;
        showPage(homePage);
    }

    void spineMarkedEvent(lv_event_t *event)
    {
        if (lv_event_get_code(event) != LV_EVENT_CLICKED || spineRunActive) return;
        if (markedSpine > 0.0f) {
            markedSpine = 0.0f;
            lv_label_set_text(spineMarkedButtonLabel, "SET MARKED");
            if (spineMarkedCallback != nullptr) spineMarkedCallback(0.0f);
            return;
        }
        createMassInput(MassInputPurpose::MarkedSpine, ArrowLabUI::LoadSide::Left);
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
            massInputKeyboard = nullptr;
        }
    }

    void massInputFieldEvent(lv_event_t *event)
    {
        const lv_event_code_t code = lv_event_get_code(event);
        if (
            (code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED)
            && massInputKeyboard != nullptr
        ) {
            lv_obj_clear_flag(massInputKeyboard, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void massKeyboardEvent(lv_event_t *event)
    {
        const lv_event_code_t code =
            lv_event_get_code(event);

        if (code == LV_EVENT_CANCEL) {
            if (massInputKeyboard != nullptr) {
                lv_obj_add_flag(
                    massInputKeyboard,
                    LV_OBJ_FLAG_HIDDEN);
            }
            return;
        }

        if (code != LV_EVENT_READY) {
            return;
        }

        const char *text =
            lv_textarea_get_text(massInputTextArea);
        const float value = std::strtof(text, nullptr);

        const float maximum = massInputPurpose == MassInputPurpose::MarkedSpine
            ? 3000.0f : 1850.0f;
        if (value <= 0.0f || value > maximum) {
            return;
        }

        if (massInputPurpose == MassInputPurpose::Calibration) {
            calibrationReferenceGrams = value;
            if (calibrationCallback != nullptr) {
                calibrationCallback(massInputSide, value);
            }
        } else if (massInputPurpose == MassInputPurpose::DiagnosticLoad) {
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
        } else {
            markedSpine = value;
            if (spineMarkedCallback != nullptr) spineMarkedCallback(markedSpine);
            if (spineMarkedButtonLabel != nullptr) {
                char label[32];
                snprintf(label, sizeof(label), "MARKED %.0f", markedSpine);
                lv_label_set_text(spineMarkedButtonLabel, label);
            }
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
                : (purpose == MassInputPurpose::MarkedSpine
                    ? "ENTER MARKED SPINE"
                    : "ENTER ACTUAL TEST MASS (g)"),
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_align(heading, LV_ALIGN_TOP_MID, 0, 0);

        massInputTextArea = lv_textarea_create(massInputBox);
        lv_obj_set_size(massInputTextArea, 180, 38);
        lv_obj_align(massInputTextArea, LV_ALIGN_TOP_MID, 0, 25);
        lv_textarea_set_one_line(massInputTextArea, true);
        lv_textarea_set_accepted_chars(massInputTextArea, "0123456789.");
        lv_textarea_set_max_length(massInputTextArea, 8);
        lv_obj_add_event_cb(
            massInputTextArea,
            massInputFieldEvent,
            LV_EVENT_ALL,
            nullptr);

        if (
            (purpose == MassInputPurpose::Calibration
                && calibrationReferenceGrams > 0.0f)
            || (purpose == MassInputPurpose::MarkedSpine
                && markedSpine > 0.0f)
        ) {
            char currentMass[16];
            snprintf(
                currentMass,
                sizeof(currentMass),
                "%.3f",
                purpose == MassInputPurpose::MarkedSpine
                    ? markedSpine
                    : calibrationReferenceGrams);
            lv_textarea_set_text(massInputTextArea, currentMass);
        }

        massInputKeyboard = lv_keyboard_create(massInputBox);
        lv_obj_set_size(massInputKeyboard, 430, 160);
        lv_obj_align(massInputKeyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_mode(
            massInputKeyboard,
            LV_KEYBOARD_MODE_NUMBER);
        lv_keyboard_set_textarea(
            massInputKeyboard,
            massInputTextArea);
        lv_obj_add_event_cb(
            massInputKeyboard,
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

    lv_obj_t *createIconMenuButton(
        lv_obj_t *parent,
        const char *symbol,
        const char *caption,
        int x,
        int y,
        lv_event_cb_t callback)
    {
        lv_obj_t *button = lv_btn_create(parent);
        lv_obj_set_size(button, 205, 76);
        lv_obj_set_pos(button, x, y);
        lv_obj_set_style_radius(button, 9, LV_PART_MAIN);
        lv_obj_set_style_bg_color(button, lv_color_hex(COLOUR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(button, lv_color_hex(COLOUR_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *icon = createTextLabel(
            button, symbol, &lv_font_montserrat_20, lv_color_hex(COLOUR_ACCENT));
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 7);
        lv_obj_t *label = createTextLabel(
            button, caption, &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -8);
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
        lv_obj_set_size(headerContextLabel, 204, 24);
        lv_obj_set_pos(headerContextLabel, 174, 11);
        lv_obj_set_style_text_align(
            headerContextLabel,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);

        headerWifiButton = lv_btn_create(header);
        lv_obj_set_size(headerWifiButton, 38, 34);
        lv_obj_align(headerWifiButton, LV_ALIGN_RIGHT_MID, -54, 0);
        lv_obj_set_style_bg_opa(
            headerWifiButton,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            headerWifiButton,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_shadow_width(
            headerWifiButton,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            headerWifiButton,
            0,
            LV_PART_MAIN);
        lv_obj_add_event_cb(
            headerWifiButton,
            wifiButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);

        const int wifiArcSizes[3] = {12, 21, 30};
        for (uint8_t index = 0; index < 3; ++index) {
            headerWifiArcs[index] = lv_arc_create(headerWifiButton);
            lv_obj_set_size(
                headerWifiArcs[index],
                wifiArcSizes[index],
                wifiArcSizes[index]);
            lv_obj_align(
                headerWifiArcs[index],
                LV_ALIGN_CENTER,
                0,
                8);
            lv_arc_set_bg_angles(headerWifiArcs[index], 210, 330);
            lv_obj_set_style_arc_width(
                headerWifiArcs[index],
                2,
                LV_PART_MAIN);
            lv_obj_set_style_arc_color(
                headerWifiArcs[index],
                lv_color_hex(COLOUR_BORDER),
                LV_PART_MAIN);
            lv_obj_set_style_arc_opa(
                headerWifiArcs[index],
                LV_OPA_TRANSP,
                LV_PART_INDICATOR);
            lv_obj_remove_style(
                headerWifiArcs[index],
                nullptr,
                LV_PART_KNOB);
            lv_obj_clear_flag(
                headerWifiArcs[index],
                LV_OBJ_FLAG_CLICKABLE);
        }

        headerWifiDot = lv_obj_create(headerWifiButton);
        lv_obj_set_size(headerWifiDot, 5, 5);
        lv_obj_align(headerWifiDot, LV_ALIGN_BOTTOM_MID, 0, -3);
        lv_obj_set_style_radius(headerWifiDot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(
            headerWifiDot,
            lv_color_hex(0xFF4D4D),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(
            headerWifiDot,
            LV_OPA_COVER,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(headerWifiDot, 0, LV_PART_MAIN);
        lv_obj_clear_flag(headerWifiDot, LV_OBJ_FLAG_SCROLLABLE);

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

        createIconMenuButton(
            homePage, LV_SYMBOL_PLAY, "SPINE TEST", 25, 5, spineButtonEvent);
        createIconMenuButton(
            homePage, LV_SYMBOL_REFRESH, "SAS TEST", 250, 5, sasButtonEvent);
        createIconMenuButton(
            homePage, LV_SYMBOL_LIST, "WEIGH", 25, 87, weighButtonEvent);
        createIconMenuButton(
            homePage, LV_SYMBOL_SETTINGS, "SETTINGS", 250, 87, settingsButtonEvent);

        homeCalibrationLabel = createTextLabel(
            homePage,
            "CALIBRATION REQUIRED",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_set_pos(homeCalibrationLabel, 30, 172);

        homeHealthLabel = createTextLabel(
            homePage,
            "CHECKING LOAD CELLS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_REQUIRED));
        lv_obj_set_pos(homeHealthLabel, 250, 172);

        spinePage = lv_obj_create(screen);
        lv_obj_set_size(spinePage, 480, 228);
        lv_obj_set_pos(spinePage, 0, 44);
        lv_obj_set_style_bg_opa(spinePage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(spinePage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(spinePage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(spinePage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *spinePanel = lv_obj_create(spinePage);
        lv_obj_set_size(spinePanel, 440, 147);
        lv_obj_set_pos(spinePanel, 20, 5);
        stylePanel(spinePanel);
        spineModeLabel = createTextLabel(
            spinePanel, "SINGLE POSITION", &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_align(spineModeLabel, LV_ALIGN_TOP_LEFT, 14, 8);
        spineStateLabel = createTextLabel(
            spinePanel, "READY", &lv_font_montserrat_20,
            lv_color_hex(COLOUR_ACCENT));
        lv_obj_set_width(spineStateLabel, 190);
        lv_obj_set_style_text_align(spineStateLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(spineStateLabel, LV_ALIGN_TOP_MID, 0, 30);
        spineDetailLabel = createTextLabel(
            spinePanel, "Empty both supports, then press START",
            &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(spineDetailLabel, 410, 24);
        lv_obj_set_style_text_align(spineDetailLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(spineDetailLabel, LV_ALIGN_TOP_MID, 0, 58);
        lv_label_set_long_mode(spineDetailLabel, LV_LABEL_LONG_DOT);
        spineResultsLabel = createTextLabel(
            spinePanel, "", &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(spineResultsLabel, 410, 42);
        lv_obj_set_style_text_align(spineResultsLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(spineResultsLabel, LV_ALIGN_TOP_MID, 0, 82);
        lv_label_set_long_mode(spineResultsLabel, LV_LABEL_LONG_WRAP);
        spineProgressBar = lv_bar_create(spinePanel);
        lv_obj_set_size(spineProgressBar, 400, 8);
        lv_obj_align(spineProgressBar, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_bar_set_range(spineProgressBar, 0, 100);
        lv_bar_set_value(spineProgressBar, 0, LV_ANIM_OFF);

        lv_obj_t *spineBackButton = lv_btn_create(spinePage);
        lv_obj_set_size(spineBackButton, 90, 34);
        lv_obj_set_pos(spineBackButton, 20, 158);
        lv_obj_add_event_cb(spineBackButton, spineBackEvent, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *spineBackLabel = createTextLabel(
            spineBackButton, "< BACK", &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(spineBackLabel);

        spineMarkedButton = lv_btn_create(spinePage);
        lv_obj_set_size(spineMarkedButton, 125, 34);
        lv_obj_set_pos(spineMarkedButton, 116, 158);
        lv_obj_add_event_cb(spineMarkedButton, spineMarkedEvent, LV_EVENT_CLICKED, nullptr);
        spineMarkedButtonLabel = createTextLabel(
            spineMarkedButton, "SET MARKED", &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(spineMarkedButtonLabel);

        spineStartButton = lv_btn_create(spinePage);
        lv_obj_set_size(spineStartButton, 125, 34);
        lv_obj_set_pos(spineStartButton, 247, 158);
        lv_obj_add_event_cb(spineStartButton, spineStartEvent, LV_EVENT_CLICKED, nullptr);
        spineStartButtonLabel = createTextLabel(
            spineStartButton, "START TEST", &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(spineStartButtonLabel);

        spineCancelButton = lv_btn_create(spinePage);
        lv_obj_set_size(spineCancelButton, 82, 34);
        lv_obj_set_pos(spineCancelButton, 378, 158);
        lv_obj_add_event_cb(spineCancelButton, spineCancelEvent, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *spineCancelLabel = createTextLabel(
            spineCancelButton, "CANCEL", &lv_font_montserrat_12,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(spineCancelLabel);
        lv_obj_add_state(spineCancelButton, LV_STATE_DISABLED);

        lv_obj_t *spineMethodLabel = createTextLabel(
            spinePage,
            "Fixed 12.7 mm / automatic stable-force capture",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(spineMethodLabel, 440, 24);
        lv_obj_set_pos(spineMethodLabel, 20, 198);
        lv_obj_set_style_text_align(spineMethodLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

        weighPage = lv_obj_create(screen);
        lv_obj_set_size(weighPage, 480, 228);
        lv_obj_set_pos(weighPage, 0, 44);
        lv_obj_set_style_bg_opa(weighPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(weighPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(weighPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(weighPage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *weighPanel = lv_obj_create(weighPage);
        lv_obj_set_size(weighPanel, 420, 140);
        lv_obj_set_pos(weighPanel, 30, 8);
        stylePanel(weighPanel);

        weighSourceLabel = createTextLabel(
            weighPanel,
            "NOT READY",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_align(weighSourceLabel, LV_ALIGN_TOP_MID, 0, 10);

        lv_obj_t *weighReadingRow = lv_obj_create(weighPanel);
        lv_obj_set_size(weighReadingRow, 370, 50);
        lv_obj_align(weighReadingRow, LV_ALIGN_TOP_MID, 0, 35);
        lv_obj_set_style_bg_opa(
            weighReadingRow,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            weighReadingRow,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(weighReadingRow, 0, LV_PART_MAIN);
        lv_obj_clear_flag(weighReadingRow, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(weighReadingRow, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(weighReadingRow, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            weighReadingRow,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);
        lv_obj_add_event_cb(
            weighReadingRow,
            unitCycleEvent,
            LV_EVENT_CLICKED,
            nullptr);

        weighValueLabel = createTextLabel(
            weighReadingRow,
            "---",
            &lv_font_montserrat_30,
            lv_color_hex(COLOUR_TEXT));
        weighUnitLabel = createTextLabel(
            weighReadingRow,
            "g",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_MUTED));

        weighConversionLabel = createTextLabel(
            weighPanel,
            "",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_width(weighConversionLabel, 390);
        lv_obj_set_style_text_align(
            weighConversionLabel,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);
        lv_obj_align(weighConversionLabel, LV_ALIGN_TOP_MID, 0, 84);

        lv_obj_t *weighBackButton = lv_btn_create(weighPage);
        lv_obj_set_size(weighBackButton, 108, 34);
        lv_obj_set_pos(weighBackButton, 30, 154);
        lv_obj_add_event_cb(
            weighBackButton,
            homeButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *weighBackLabel = createTextLabel(
            weighBackButton,
            "< BACK",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(weighBackLabel);

        weighTareLeftButton = lv_btn_create(weighPanel);
        lv_obj_set_size(weighTareLeftButton, 96, 28);
        lv_obj_set_pos(weighTareLeftButton, 8, 104);
        lv_obj_add_event_cb(
            weighTareLeftButton,
            tareLeftButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *weighTareLeftLabel = createTextLabel(
            weighTareLeftButton,
            "TARE L",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(weighTareLeftLabel);

        weighTareRightButton = lv_btn_create(weighPanel);
        lv_obj_set_size(weighTareRightButton, 96, 28);
        lv_obj_set_pos(weighTareRightButton, 312, 104);
        lv_obj_add_event_cb(
            weighTareRightButton,
            tareRightButtonEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *weighTareRightLabel = createTextLabel(
            weighTareRightButton,
            "TARE R",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(weighTareRightLabel);

        weighInstructionLabel = createTextLabel(
            weighPage,
            "Tare and calibrate a live cassette first",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(weighInstructionLabel, 420, 28);
        lv_obj_set_pos(weighInstructionLabel, 30, 195);
        lv_obj_set_style_text_align(
            weighInstructionLabel,
            LV_TEXT_ALIGN_CENTER,
            LV_PART_MAIN);
        lv_label_set_long_mode(
            weighInstructionLabel,
            LV_LABEL_LONG_DOT);

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

        createMenuButton(
            settingsPage,
            "WI-FI & NETWORK",
            64,
            wifiButtonEvent);

        settingsCalibrationButton = createMenuButton(
            settingsPage,
            "CALIBRATION",
            120,
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
            174,
            diagnosticsButtonEvent);
        if (!developerMode) {
            lv_obj_add_flag(diagnosticsButton, LV_OBJ_FLAG_HIDDEN);
        }

        wifiPage = lv_obj_create(screen);
        lv_obj_set_size(wifiPage, 480, 228);
        lv_obj_set_pos(wifiPage, 0, 44);
        lv_obj_set_style_bg_opa(wifiPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(wifiPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(wifiPage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *wifiBack = lv_btn_create(wifiPage);
        lv_obj_set_size(wifiBack, 104, 34);
        lv_obj_set_pos(wifiBack, 14, 7);
        lv_obj_add_event_cb(
            wifiBack,
            wifiBackEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *wifiBackLabel = createTextLabel(
            wifiBack,
            "< SETTINGS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(wifiBackLabel);

        lv_obj_t *wifiSavedButton = lv_btn_create(wifiPage);
        lv_obj_set_size(wifiSavedButton, 160, 34);
        lv_obj_set_pos(wifiSavedButton, 122, 7);
        lv_obj_add_event_cb(
            wifiSavedButton,
            wifiSavedOpenEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *wifiSavedButtonLabel = createTextLabel(
            wifiSavedButton,
            "SAVED NETWORKS",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(wifiSavedButtonLabel);

        lv_obj_t *wifiConfigureButton = lv_btn_create(wifiPage);
        lv_obj_set_size(wifiConfigureButton, 174, 34);
        lv_obj_set_pos(wifiConfigureButton, 290, 7);
        lv_obj_add_event_cb(
            wifiConfigureButton,
            wifiConfigureEvent,
            LV_EVENT_CLICKED,
            nullptr);
        wifiConfigureButtonLabel = createTextLabel(
            wifiConfigureButton,
            "CHANGE NETWORK",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(wifiConfigureButtonLabel);

        lv_obj_t *viewePanel = lv_obj_create(wifiPage);
        lv_obj_set_size(viewePanel, 214, 170);
        lv_obj_set_pos(viewePanel, 20, 50);
        stylePanel(viewePanel);
        vieweNetworkLabel = createTextLabel(
            viewePanel,
            "VIEWE\nWaiting for network information",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(vieweNetworkLabel, 194, 150);
        lv_obj_set_pos(vieweNetworkLabel, 10, 9);
        lv_label_set_long_mode(vieweNetworkLabel, LV_LABEL_LONG_DOT);

        lv_obj_t *wroomPanel = lv_obj_create(wifiPage);
        lv_obj_set_size(wroomPanel, 214, 170);
        lv_obj_set_pos(wroomPanel, 246, 50);
        stylePanel(wroomPanel);
        wroomNetworkLabel = createTextLabel(
            wroomPanel,
            "WROOM\nWaiting for measurement node",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(wroomNetworkLabel, 194, 150);
        lv_obj_set_pos(wroomNetworkLabel, 10, 9);
        lv_label_set_long_mode(wroomNetworkLabel, LV_LABEL_LONG_DOT);

        wifiSavedPage = lv_obj_create(screen);
        lv_obj_set_size(wifiSavedPage, 480, 228);
        lv_obj_set_pos(wifiSavedPage, 0, 44);
        lv_obj_set_style_bg_opa(
            wifiSavedPage,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            wifiSavedPage,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiSavedPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(wifiSavedPage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *savedBack = lv_btn_create(wifiSavedPage);
        lv_obj_set_size(savedBack, 104, 34);
        lv_obj_set_pos(savedBack, 14, 7);
        lv_obj_add_event_cb(
            savedBack,
            wifiSavedBackEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *savedBackLabel = createTextLabel(
            savedBack,
            "< WI-FI",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(savedBackLabel);

        wifiSavedStatusLabel = createTextLabel(
            wifiSavedPage,
            "NO SAVED NETWORKS",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(wifiSavedStatusLabel, 330, 24);
        lv_obj_set_pos(wifiSavedStatusLabel, 130, 14);
        lv_obj_set_style_text_align(
            wifiSavedStatusLabel,
            LV_TEXT_ALIGN_RIGHT,
            LV_PART_MAIN);

        wifiSavedList = lv_obj_create(wifiSavedPage);
        lv_obj_set_size(wifiSavedList, 456, 176);
        lv_obj_set_pos(wifiSavedList, 12, 49);
        lv_obj_set_style_bg_opa(
            wifiSavedList,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            wifiSavedList,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiSavedList, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_right(wifiSavedList, 5, LV_PART_MAIN);
        lv_obj_set_scroll_dir(wifiSavedList, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(
            wifiSavedList,
            LV_SCROLLBAR_MODE_AUTO);

        for (
            size_t index = 0;
            index < MAX_WIFI_SAVED_PROFILES;
            ++index
        ) {
            wifiSavedRows[index] = lv_obj_create(wifiSavedList);
            lv_obj_set_size(wifiSavedRows[index], 432, 42);
            lv_obj_set_pos(wifiSavedRows[index], 4, index * 47);
            stylePanel(wifiSavedRows[index]);
            lv_obj_clear_flag(
                wifiSavedRows[index],
                LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(
                wifiSavedRows[index],
                LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(
                wifiSavedRows[index],
                wifiSavedProfileEvent,
                LV_EVENT_CLICKED,
                reinterpret_cast<void *>(
                    static_cast<uintptr_t>(index)));

            wifiSavedLabels[index] = createTextLabel(
                wifiSavedRows[index],
                "--",
                &lv_font_montserrat_14,
                lv_color_hex(COLOUR_TEXT));
            lv_obj_set_size(wifiSavedLabels[index], 292, 24);
            lv_obj_align(
                wifiSavedLabels[index],
                LV_ALIGN_LEFT_MID,
                8,
                0);
            lv_label_set_long_mode(
                wifiSavedLabels[index],
                LV_LABEL_LONG_DOT);

            wifiSavedForgetButtons[index] =
                lv_btn_create(wifiSavedRows[index]);
            lv_obj_set_size(
                wifiSavedForgetButtons[index],
                104,
                30);
            lv_obj_align(
                wifiSavedForgetButtons[index],
                LV_ALIGN_RIGHT_MID,
                -4,
                0);
            lv_obj_add_event_cb(
                wifiSavedForgetButtons[index],
                wifiSavedForgetEvent,
                LV_EVENT_CLICKED,
                reinterpret_cast<void *>(
                    static_cast<uintptr_t>(index)));
            lv_obj_t *forgetLabel = createTextLabel(
                wifiSavedForgetButtons[index],
                "FORGET",
                &lv_font_montserrat_12,
                lv_color_hex(COLOUR_TEXT));
            lv_obj_center(forgetLabel);
            lv_obj_add_flag(
                wifiSavedRows[index],
                LV_OBJ_FLAG_HIDDEN);
        }

        wifiProfilePage = lv_obj_create(screen);
        lv_obj_set_size(wifiProfilePage, 480, 228);
        lv_obj_set_pos(wifiProfilePage, 0, 44);
        lv_obj_set_style_bg_opa(
            wifiProfilePage,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            wifiProfilePage,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(
            wifiProfilePage,
            0,
            LV_PART_MAIN);
        lv_obj_clear_flag(
            wifiProfilePage,
            LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *profileBack = lv_btn_create(wifiProfilePage);
        lv_obj_set_size(profileBack, 118, 34);
        lv_obj_set_pos(profileBack, 14, 7);
        lv_obj_add_event_cb(
            profileBack,
            wifiProfileBackEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *profileBackLabel = createTextLabel(
            profileBack,
            "< SAVED",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(profileBackLabel);

        wifiProfileConnectButton = lv_btn_create(wifiProfilePage);
        lv_obj_set_size(wifiProfileConnectButton, 142, 34);
        lv_obj_set_pos(wifiProfileConnectButton, 324, 7);
        lv_obj_add_event_cb(
            wifiProfileConnectButton,
            wifiProfileConnectEvent,
            LV_EVENT_CLICKED,
            nullptr);
        wifiProfileConnectLabel = createTextLabel(
            wifiProfileConnectButton,
            "CONNECT",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_center(wifiProfileConnectLabel);

        lv_obj_t *profilePanel = lv_obj_create(wifiProfilePage);
        lv_obj_set_size(profilePanel, 440, 166);
        lv_obj_set_pos(profilePanel, 20, 52);
        stylePanel(profilePanel);

        lv_obj_t *networkCaption = createTextLabel(
            profilePanel,
            "NETWORK",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(networkCaption, 12, 8);

        wifiProfileSsidLabel = createTextLabel(
            profilePanel,
            "--",
            &lv_font_montserrat_18,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(wifiProfileSsidLabel, 290, 28);
        lv_obj_set_pos(wifiProfileSsidLabel, 12, 25);
        lv_label_set_long_mode(
            wifiProfileSsidLabel,
            LV_LABEL_LONG_DOT);

        wifiProfileStateLabel = createTextLabel(
            profilePanel,
            "STATUS: SAVED",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(wifiProfileStateLabel, 130, 24);
        lv_obj_set_pos(wifiProfileStateLabel, 296, 28);
        lv_obj_set_style_text_align(
            wifiProfileStateLabel,
            LV_TEXT_ALIGN_RIGHT,
            LV_PART_MAIN);

        lv_obj_t *passwordCaption = createTextLabel(
            profilePanel,
            "STORED PASSWORD",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(passwordCaption, 12, 68);

        lv_obj_t *passwordPanel = lv_obj_create(profilePanel);
        lv_obj_set_size(passwordPanel, 350, 42);
        lv_obj_set_pos(passwordPanel, 12, 88);
        lv_obj_set_style_bg_color(
            passwordPanel,
            lv_color_hex(COLOUR_BACKGROUND),
            LV_PART_MAIN);
        lv_obj_set_style_border_color(
            passwordPanel,
            lv_color_hex(COLOUR_BORDER),
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            passwordPanel,
            1,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(passwordPanel, 0, LV_PART_MAIN);
        lv_obj_clear_flag(
            passwordPanel,
            LV_OBJ_FLAG_SCROLLABLE);

        wifiProfilePasswordLabel = createTextLabel(
            passwordPanel,
            "",
            &lv_font_montserrat_16,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(wifiProfilePasswordLabel, 326, 24);
        lv_obj_align(
            wifiProfilePasswordLabel,
            LV_ALIGN_LEFT_MID,
            10,
            0);
        lv_label_set_long_mode(
            wifiProfilePasswordLabel,
            LV_LABEL_LONG_DOT);

        lv_obj_t *profileRevealButton =
            lv_btn_create(profilePanel);
        lv_obj_set_size(profileRevealButton, 48, 42);
        lv_obj_set_pos(profileRevealButton, 370, 88);
        lv_obj_add_event_cb(
            profileRevealButton,
            wifiProfileRevealEvent,
            LV_EVENT_CLICKED,
            nullptr);
        wifiProfileRevealIcon = createTextLabel(
            profileRevealButton,
            LV_SYMBOL_EYE_OPEN,
            &lv_font_montserrat_18,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_center(wifiProfileRevealIcon);

        lv_obj_t *profileNote = createTextLabel(
            profilePanel,
            "Stored locally on ArrowLab",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(profileNote, 12, 137);

        wifiNetworksPage = lv_obj_create(screen);
        lv_obj_set_size(wifiNetworksPage, 480, 228);
        lv_obj_set_pos(wifiNetworksPage, 0, 44);
        lv_obj_set_style_bg_opa(wifiNetworksPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(wifiNetworksPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiNetworksPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(wifiNetworksPage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *netBack = lv_btn_create(wifiNetworksPage);
        lv_obj_set_size(netBack, 100, 34);
        lv_obj_set_pos(netBack, 14, 7);
        lv_obj_add_event_cb(netBack, wifiNetworksBackEvent, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *netBackLabel = createTextLabel(
            netBack, "< BACK", &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_center(netBackLabel);

        lv_obj_t *refresh = lv_btn_create(wifiNetworksPage);
        lv_obj_set_size(refresh, 100, 34);
        lv_obj_set_pos(refresh, 366, 7);
        lv_obj_add_event_cb(refresh, wifiRefreshEvent, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *refreshLabel = createTextLabel(
            refresh, "REFRESH", &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_center(refreshLabel);

        wifiScanStatusLabel = createTextLabel(
            wifiNetworksPage,
            "SCANNING...",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_pos(wifiScanStatusLabel, 130, 15);

        wifiNetworkList = lv_obj_create(wifiNetworksPage);
        lv_obj_set_size(wifiNetworkList, 456, 176);
        lv_obj_set_pos(wifiNetworkList, 12, 49);
        lv_obj_set_style_bg_opa(
            wifiNetworkList,
            LV_OPA_TRANSP,
            LV_PART_MAIN);
        lv_obj_set_style_border_width(
            wifiNetworkList,
            0,
            LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiNetworkList, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_right(wifiNetworkList, 5, LV_PART_MAIN);
        lv_obj_set_scroll_dir(wifiNetworkList, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(
            wifiNetworkList,
            LV_SCROLLBAR_MODE_AUTO);

        for (
            size_t index = 0;
            index < MAX_WIFI_SCAN_RESULTS;
            ++index
        ) {
            wifiNetworkButtons[index] = lv_btn_create(wifiNetworkList);
            lv_obj_set_size(wifiNetworkButtons[index], 432, 38);
            lv_obj_set_pos(wifiNetworkButtons[index], 4, index * 43);
            lv_obj_add_event_cb(
                wifiNetworkButtons[index],
                wifiNetworkSelectEvent,
                LV_EVENT_CLICKED,
                reinterpret_cast<void *>(static_cast<uintptr_t>(index)));
            wifiNetworkButtonLabels[index] = createTextLabel(
                wifiNetworkButtons[index],
                "--",
                &lv_font_montserrat_14,
                lv_color_hex(COLOUR_TEXT));
            lv_obj_align(
                wifiNetworkButtonLabels[index],
                LV_ALIGN_LEFT_MID,
                6,
                0);
            lv_obj_add_flag(
                wifiNetworkButtons[index],
                LV_OBJ_FLAG_HIDDEN);
        }

        wifiPasswordPage = lv_obj_create(screen);
        lv_obj_set_size(wifiPasswordPage, 480, 228);
        lv_obj_set_pos(wifiPasswordPage, 0, 44);
        lv_obj_set_style_bg_opa(wifiPasswordPage, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(wifiPasswordPage, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(wifiPasswordPage, 0, LV_PART_MAIN);
        lv_obj_clear_flag(wifiPasswordPage, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *passwordBack = lv_btn_create(wifiPasswordPage);
        lv_obj_set_size(passwordBack, 86, 30);
        lv_obj_set_pos(passwordBack, 8, 4);
        lv_obj_add_event_cb(
            passwordBack,
            wifiPasswordBackEvent,
            LV_EVENT_CLICKED,
            nullptr);
        lv_obj_t *passwordBackLabel = createTextLabel(
            passwordBack, "< BACK", &lv_font_montserrat_14, lv_color_hex(COLOUR_TEXT));
        lv_obj_center(passwordBackLabel);

        wifiPasswordSsidLabel = createTextLabel(
            wifiPasswordPage,
            "CONNECT TO: --",
            &lv_font_montserrat_14,
            lv_color_hex(COLOUR_TEXT));
        lv_obj_set_size(wifiPasswordSsidLabel, 360, 22);
        lv_obj_set_pos(wifiPasswordSsidLabel, 106, 8);
        lv_label_set_long_mode(wifiPasswordSsidLabel, LV_LABEL_LONG_DOT);

        wifiPasswordTextArea = lv_textarea_create(wifiPasswordPage);
        lv_obj_set_size(wifiPasswordTextArea, 172, 34);
        lv_obj_set_pos(wifiPasswordTextArea, 8, 38);
        lv_textarea_set_one_line(wifiPasswordTextArea, true);
        lv_textarea_set_password_mode(wifiPasswordTextArea, true);
        lv_textarea_set_placeholder_text(wifiPasswordTextArea, "Wi-Fi password");
        lv_obj_add_event_cb(
            wifiPasswordTextArea,
            wifiPasswordFieldEvent,
            LV_EVENT_ALL,
            nullptr);

        wifiPasswordRevealButton = lv_btn_create(wifiPasswordPage);
        lv_obj_set_size(wifiPasswordRevealButton, 38, 34);
        lv_obj_set_pos(wifiPasswordRevealButton, 184, 38);
        lv_obj_add_event_cb(
            wifiPasswordRevealButton,
            wifiPasswordRevealEvent,
            LV_EVENT_CLICKED,
            nullptr);
        wifiPasswordRevealIcon = createTextLabel(
            wifiPasswordRevealButton,
            LV_SYMBOL_EYE_OPEN,
            &lv_font_montserrat_18,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_center(wifiPasswordRevealIcon);

        wifiPasswordStatusLabel = createTextLabel(
            wifiPasswordPage,
            "Enter password, then tap the keyboard checkmark",
            &lv_font_montserrat_12,
            lv_color_hex(COLOUR_MUTED));
        lv_obj_set_size(wifiPasswordStatusLabel, 242, 34);
        lv_obj_set_pos(wifiPasswordStatusLabel, 230, 38);
        lv_label_set_long_mode(wifiPasswordStatusLabel, LV_LABEL_LONG_WRAP);

        wifiKeyboard = lv_keyboard_create(wifiPasswordPage);
        lv_obj_set_size(wifiKeyboard, 464, 146);
        lv_obj_set_style_pad_all(wifiKeyboard, 3, LV_PART_MAIN);
        lv_obj_set_style_pad_row(wifiKeyboard, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_column(wifiKeyboard, 2, LV_PART_MAIN);
        lv_keyboard_set_textarea(wifiKeyboard, wifiPasswordTextArea);
        lv_obj_align(wifiKeyboard, LV_ALIGN_BOTTOM_MID, 0, -2);
        lv_obj_add_event_cb(
            wifiKeyboard,
            wifiKeyboardEvent,
            LV_EVENT_ALL,
            nullptr);

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

    void setUnitCycleCallback(UnitCycleCallback callback)
    {
        unitCycleCallback = callback;
    }

    void setSpineCallbacks(
        SpineStartCallback startCallback,
        SpineControlCallback cancelCallback,
        SpineControlCallback confirmClearCallback,
        SpineControlCallback confirmZeroCallback,
        SpineControlCallback restartCallback,
        SpineMarkedCallback markedCallback)
    {
        spineStartCallback = startCallback;
        spineCancelCallback = cancelCallback;
        spineConfirmClearCallback = confirmClearCallback;
        spineConfirmZeroCallback = confirmZeroCallback;
        spineRestartCallback = restartCallback;
        spineMarkedCallback = markedCallback;
    }

    void setCalibrationReferenceGrams(float grams)
    {
        calibrationReferenceGrams = grams;
    }

    void setWifiCallbacks(
        WifiScanCallback scanCallback,
        WifiConnectCallback connectCallback,
        WifiForgetCallback forgetCallback)
    {
        wifiScanCallback = scanCallback;
        wifiConnectCallback = connectCallback;
        wifiForgetCallback = forgetCallback;
    }

    void setWifiScanBusy()
    {
        if (wifiScanStatusLabel != nullptr) {
            lv_label_set_text(wifiScanStatusLabel, "SCANNING...");
        }
    }

    void setWifiScanResults(
        const char ssids[][33],
        const int16_t *rssiDbm,
        const bool *secured,
        size_t count)
    {
        wifiScannedCount =
            count > MAX_WIFI_SCAN_RESULTS
                ? MAX_WIFI_SCAN_RESULTS
                : count;
        for (
            size_t index = 0;
            index < MAX_WIFI_SCAN_RESULTS;
            ++index
        ) {
            if (index >= wifiScannedCount) {
                lv_obj_add_flag(
                    wifiNetworkButtons[index],
                    LV_OBJ_FLAG_HIDDEN);
                continue;
            }
            snprintf(
                wifiScannedSsids[index],
                sizeof(wifiScannedSsids[index]),
                "%s",
                ssids[index]);
            char text[72];
            snprintf(
                text,
                sizeof(text),
                "%s   %d dBm   %s",
                ssids[index],
                static_cast<int>(rssiDbm[index]),
                secured[index] ? "SECURED" : "OPEN");
            lv_label_set_text(wifiNetworkButtonLabels[index], text);
            lv_obj_clear_flag(
                wifiNetworkButtons[index],
                LV_OBJ_FLAG_HIDDEN);
        }
        if (wifiScanStatusLabel != nullptr) {
            lv_label_set_text(
                wifiScanStatusLabel,
                wifiScannedCount > 0
                    ? "SELECT A NETWORK"
                    : "NO NETWORKS FOUND");
        }
    }

    void setWifiSetupResult(
        bool busy,
        bool success,
        const char *message)
    {
        const uint32_t messageColour =
            success
                ? COLOUR_OK
                : busy
                    ? COLOUR_REQUIRED
                    : 0xFF4D4D;
        if (wifiPasswordStatusLabel != nullptr) {
            lv_label_set_text(
                wifiPasswordStatusLabel,
                message != nullptr ? message : "");
            lv_obj_set_style_text_color(
                wifiPasswordStatusLabel,
                lv_color_hex(messageColour),
                LV_PART_MAIN);
        }
        if (
            wifiProfileConnectionActive
            && wifiProfileStateLabel != nullptr
        ) {
            lv_label_set_text(
                wifiProfileStateLabel,
                message != nullptr ? message : "");
            lv_obj_set_style_text_color(
                wifiProfileStateLabel,
                lv_color_hex(messageColour),
                LV_PART_MAIN);
            lv_label_set_text(
                wifiProfileConnectLabel,
                busy ? "CONNECTING" : success ? "CONNECTED" : "RETRY");
            if (busy || success) {
                lv_obj_add_state(
                    wifiProfileConnectButton,
                    LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(
                    wifiProfileConnectButton,
                    LV_STATE_DISABLED);
                wifiProfileConnectionActive = false;
            }
        }
        if (wifiKeyboard != nullptr) {
            if (busy) {
                lv_obj_add_state(wifiKeyboard, LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(wifiKeyboard, LV_STATE_DISABLED);
            }
        }
        if (success) {
            wifiProfileConnectionActive = false;
            lv_textarea_set_text(wifiPasswordTextArea, "");
            showPage(wifiPage);
        }
    }

    void setWifiSavedCredentials(bool saved)
    {
        if (wifiConfigureButtonLabel != nullptr) {
            lv_label_set_text(
                wifiConfigureButtonLabel,
                saved ? "CHANGE NETWORK" : "SET UP WI-FI");
        }
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

    void setSensorHealth(
        bool measurementNodeOnline,
        bool leftLive,
        bool rightLive)
    {
        if (
            homeHealthLabel == nullptr
            || faultBar == nullptr
            || faultLabel == nullptr
        ) {
            return;
        }

        if (measurementNodeOnline && leftLive && rightLive) {
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

        const char *faultText = !measurementNodeOnline
            ? "FAULT: MEASUREMENT NODE OFFLINE"
            : (!leftLive && !rightLive
                ? "FAULT: LEFT + RIGHT LOAD CELLS"
                : (!leftLive
                    ? "FAULT: LEFT LOAD CELL"
                    : "FAULT: RIGHT LOAD CELL"));

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

    void setLoadConversions(
        LoadSide side,
        const char *text)
    {
        lv_obj_t *label =
            side == LoadSide::Left
                ? leftPanel.conversionLabel
                : rightPanel.conversionLabel;

        if (label != nullptr)
        {
            lv_label_set_text(label, text);
        }
    }

    void setWeighDisplay(
        const char *source,
        const char *primary,
        const char *unit,
        const char *secondary,
        const char *instruction)
    {
        if (weighSourceLabel != nullptr) {
            lv_label_set_text(weighSourceLabel, source);
        }
        if (weighValueLabel != nullptr) {
            lv_label_set_text(weighValueLabel, primary);
        }
        if (weighUnitLabel != nullptr) {
            lv_label_set_text(weighUnitLabel, unit);
        }
        if (weighConversionLabel != nullptr) {
            lv_label_set_text(weighConversionLabel, secondary);
        }
        if (weighInstructionLabel != nullptr) {
            lv_label_set_text(weighInstructionLabel, instruction);
        }
    }

    void setSpineDisplay(
        const char *state,
        const char *detail,
        const char *results,
        const char *primaryAction,
        uint8_t progressPercent,
        bool active,
        bool complete,
        bool clearConfirmationRequired,
        bool zeroConfirmationRequired)
    {
        spineRunActive = active;
        spineClearConfirmationRequired = clearConfirmationRequired;
        spineZeroConfirmationRequired = zeroConfirmationRequired;
        if (spineStateLabel != nullptr) lv_label_set_text(spineStateLabel, state);
        if (spineDetailLabel != nullptr) lv_label_set_text(spineDetailLabel, detail);
        if (spineResultsLabel != nullptr) lv_label_set_text(spineResultsLabel, results);
        if (spineProgressBar != nullptr) {
            lv_bar_set_value(spineProgressBar, progressPercent, LV_ANIM_OFF);
        }
        if (spineStartButton != nullptr) {
            lv_obj_clear_state(spineStartButton, LV_STATE_DISABLED);
        }
        if (spineMarkedButton != nullptr) {
            if (active && !complete) {
                lv_obj_add_state(spineMarkedButton, LV_STATE_DISABLED);
            } else {
                lv_obj_clear_state(spineMarkedButton, LV_STATE_DISABLED);
            }
        }
        if (spineCancelButton != nullptr) {
            if (active) lv_obj_clear_state(spineCancelButton, LV_STATE_DISABLED);
            else lv_obj_add_state(spineCancelButton, LV_STATE_DISABLED);
        }
        if (spineStartButtonLabel != nullptr) {
            lv_label_set_text(spineStartButtonLabel, primaryAction);
        }
        (void)complete;
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
            const lv_color_t tareColour = lv_color_hex(
                tareComplete && userTareConfirmed
                    ? COLOUR_OK
                    : COLOUR_REQUIRED);
            lv_obj_set_style_bg_color(
                panel.tareButton,
                tareColour,
                LV_PART_MAIN);

            lv_obj_t *weighTareButton = side == LoadSide::Left
                ? weighTareLeftButton
                : weighTareRightButton;
            if (weighTareButton != nullptr) {
                lv_obj_set_style_bg_color(
                    weighTareButton,
                    tareColour,
                    LV_PART_MAIN);
            }
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

    void firmwareUpdateDismissEvent(lv_event_t *event)
    {
        if (
            lv_event_get_code(event) == LV_EVENT_CLICKED
            && firmwareUpdateOverlay != nullptr
        ) {
            lv_obj_add_flag(
                firmwareUpdateOverlay,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

    void setFirmwareUpdateActive(bool active)
    {
        if (active) {
            if (firmwareUpdateOverlay == nullptr) {
                firmwareUpdateOverlay = lv_obj_create(lv_scr_act());
                lv_obj_set_size(firmwareUpdateOverlay, 480, 272);
                lv_obj_set_pos(firmwareUpdateOverlay, 0, 0);
                lv_obj_set_style_bg_color(
                    firmwareUpdateOverlay,
                    lv_color_hex(COLOUR_BACKGROUND),
                    LV_PART_MAIN);
                lv_obj_set_style_bg_opa(
                    firmwareUpdateOverlay,
                    LV_OPA_COVER,
                    LV_PART_MAIN);
                lv_obj_set_style_border_width(
                    firmwareUpdateOverlay,
                    0,
                    LV_PART_MAIN);
                lv_obj_set_style_radius(
                    firmwareUpdateOverlay,
                    0,
                    LV_PART_MAIN);
                lv_obj_clear_flag(
                    firmwareUpdateOverlay,
                    LV_OBJ_FLAG_SCROLLABLE);

                lv_obj_t *title = createTextLabel(
                    firmwareUpdateOverlay,
                    "FIRMWARE UPDATE",
                    &lv_font_montserrat_20,
                    lv_color_hex(COLOUR_ACCENT));
                lv_obj_align(title, LV_ALIGN_CENTER, 0, -28);

                firmwareUpdateMessage = createTextLabel(
                    firmwareUpdateOverlay,
                    "Preparing wireless update...\nDo not remove power\nScreen will turn off briefly",
                    &lv_font_montserrat_16,
                    lv_color_hex(COLOUR_TEXT));
                lv_obj_set_style_text_align(
                    firmwareUpdateMessage,
                    LV_TEXT_ALIGN_CENTER,
                    LV_PART_MAIN);
                lv_obj_align(
                    firmwareUpdateMessage,
                    LV_ALIGN_CENTER,
                    0,
                    18);

                firmwareUpdateDismissButton =
                    lv_btn_create(firmwareUpdateOverlay);
                lv_obj_set_size(
                    firmwareUpdateDismissButton,
                    120,
                    38);
                lv_obj_align(
                    firmwareUpdateDismissButton,
                    LV_ALIGN_CENTER,
                    0,
                    78);
                lv_obj_add_event_cb(
                    firmwareUpdateDismissButton,
                    firmwareUpdateDismissEvent,
                    LV_EVENT_CLICKED,
                    nullptr);
                lv_obj_t *dismissLabel = createTextLabel(
                    firmwareUpdateDismissButton,
                    "CONTINUE",
                    &lv_font_montserrat_14,
                    lv_color_hex(COLOUR_TEXT));
                lv_obj_center(dismissLabel);
            }

            if (firmwareUpdateMessage != nullptr) {
                lv_label_set_text(
                    firmwareUpdateMessage,
                    "Preparing wireless update...\nDo not remove power\nScreen will turn off briefly");
                lv_obj_set_style_text_color(
                    firmwareUpdateMessage,
                    lv_color_hex(COLOUR_TEXT),
                    LV_PART_MAIN);
            }
            if (firmwareUpdateDismissButton != nullptr) {
                lv_obj_add_flag(
                    firmwareUpdateDismissButton,
                    LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_clear_flag(
                firmwareUpdateOverlay,
                LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(firmwareUpdateOverlay);
        } else if (firmwareUpdateOverlay != nullptr) {
            lv_obj_add_flag(
                firmwareUpdateOverlay,
                LV_OBJ_FLAG_HIDDEN);
        }
    }

    void setFirmwareUpdateFailed()
    {
        if (firmwareUpdateOverlay == nullptr) {
            setFirmwareUpdateActive(true);
        }

        if (firmwareUpdateMessage != nullptr) {
            lv_label_set_text(
                firmwareUpdateMessage,
                "UPDATE FAILED\nArrowLab remains operational\nCheck PlatformIO and retry");
            lv_obj_set_style_text_color(
                firmwareUpdateMessage,
                lv_color_hex(0xFF4D4D),
                LV_PART_MAIN);
        }
        if (firmwareUpdateDismissButton != nullptr) {
            lv_obj_clear_flag(
                firmwareUpdateDismissButton,
                LV_OBJ_FLAG_HIDDEN);
        }
        if (firmwareUpdateOverlay != nullptr) {
            lv_obj_clear_flag(
                firmwareUpdateOverlay,
                LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(firmwareUpdateOverlay);
        }
    }

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
        const char *wroomMac)
    {
        const auto quality = [](int16_t rssi) {
            if (rssi >= -55) return "EXCELLENT";
            if (rssi >= -67) return "GOOD";
            if (rssi >= -75) return "FAIR";
            return "WEAK";
        };

        char text[256];
        const bool arrowLabConnected =
            vieweConnected && wroomOnline && wroomConnected;
        const int16_t arrowLabRssi =
            vieweRssiDbm < wroomRssiDbm
                ? vieweRssiDbm
                : wroomRssiDbm;
        updateHeaderWifiStrength(
            arrowLabConnected,
            arrowLabRssi);

        if (vieweNetworkLabel != nullptr) {
            snprintf(
                text,
                sizeof(text),
                "VIEWE  %s\n%s\nSSID: %s\nIP: %s\nRSSI: %d dBm\nSIGNAL: %s\nMAC: %s",
                vieweConnected ? "ONLINE" : "OFFLINE",
                vieweHostname != nullptr ? vieweHostname : "--",
                vieweConnected && vieweSsid != nullptr ? vieweSsid : "--",
                vieweConnected && vieweIp != nullptr ? vieweIp : "--",
                static_cast<int>(vieweRssiDbm),
                vieweConnected ? quality(vieweRssiDbm) : "--",
                vieweMac != nullptr && vieweMac[0] != '\0'
                    ? vieweMac
                    : "--");
            lv_label_set_text(vieweNetworkLabel, text);
        }

        if (wroomNetworkLabel != nullptr) {
            snprintf(
                text,
                sizeof(text),
                "WROOM  %s\n%s\nSSID: %s\nIP: %s\nRSSI: %d dBm\nSIGNAL: %s\nMAC: %s",
                wroomOnline
                    ? (wroomConnected ? "ONLINE" : "WI-FI OFF")
                    : "NODE OFFLINE",
                wroomHostname != nullptr && wroomHostname[0] != '\0'
                    ? wroomHostname
                    : "arrowlab-measurement",
                wroomConnected && wroomSsid != nullptr ? wroomSsid : "--",
                wroomConnected && wroomIp != nullptr ? wroomIp : "--",
                static_cast<int>(wroomRssiDbm),
                wroomConnected ? quality(wroomRssiDbm) : "--",
                wroomMac != nullptr && wroomMac[0] != '\0'
                    ? wroomMac
                    : "--");
            lv_label_set_text(wroomNetworkLabel, text);
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
