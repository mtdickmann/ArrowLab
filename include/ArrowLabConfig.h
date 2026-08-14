#pragma once

#include <cstdint>

namespace ArrowLabConfig
{
    enum class MassUnit : uint8_t
    {
        Grams = 0,
        Grains = 1,
        Ounces = 2
    };

    enum class MeasurementLinkMode : uint8_t
    {
        // Proven fallback. It preserves display and touch, but occupies the
        // VIEWE onboard SD-card MOSI/SCK pins.
        VieweGpio11And12 = 0,

        // Production half-duplex link: VIEWE GPIO17 to WROOM GPIO8.
        // Fit one external 4.7 kOhm pull-up from the shared signal to 3.3 V.
        VieweGpio17OneWire = 1,

        // UNTESTED full-duplex option using the VIEWE UART0 pins. NEVER
        // connect WROOM TX while the VIEWE USB/CH340 interface is attached:
        // the CH340 TX and WROOM TX would drive VIEWE RX simultaneously.
        VieweGpio43And44 = 2
    };

    // Human-editable installation/development choices live separately from
    // the implementation below, in the printer.cfg-style ArrowLab.conf file.
    #include "ArrowLab.conf"

    static_assert(
        CONFIGURED_MEASUREMENT_LINK_MODE <= 2,
        "ArrowLab.conf: measurement link mode must be 0, 1 or 2");
    static_assert(
        CONFIGURED_OPERATIONAL_WEIGHING_TIME_MS >= 500
            && CONFIGURED_OPERATIONAL_WEIGHING_TIME_MS <= 10000,
        "ArrowLab.conf: operational weighing time must be 500..10000 ms");
    static_assert(
        CONFIGURED_PRIMARY_MASS_UNIT <= 2,
        "ArrowLab.conf: primary mass unit must be 0, 1 or 2");
    static_assert(
        CONFIGURED_GRAMS_DECIMAL_PLACES <= 3,
        "ArrowLab.conf: gram decimal places must be 0..3");
    static_assert(
        CONFIGURED_GRAINS_DECIMAL_PLACES <= 3,
        "ArrowLab.conf: grain decimal places must be 0..3");
    static_assert(
        CONFIGURED_OUNCES_DECIMAL_PLACES <= 4,
        "ArrowLab.conf: ounce decimal places must be 0..4");
    static_assert(
        CONFIGURED_SPINE_HOLD_TIME_MS >= 1000,
        "ArrowLab.conf: spine hold time must be at least 1000 ms");
    static_assert(
        CONFIGURED_SPINE_STABILITY_BAND_GRAMS > 0.0f,
        "ArrowLab.conf: spine stability band must be positive");
    static_assert(
        CONFIGURED_SPINE_BASELINE_TRACKING_BAND_GRAMS > 0.0f
            && CONFIGURED_SPINE_BASELINE_TRACKING_BAND_GRAMS
                < CONFIGURED_SPINE_MINIMUM_APPLIED_FORCE_GRAMS,
        "ArrowLab.conf: spine baseline band must be positive and below trigger");
    static_assert(
        CONFIGURED_ARROW_STABILITY_BAND_GRAMS > 0.0f,
        "ArrowLab.conf: arrow stability band must be positive");

    // Stable, strongly typed names consumed by the firmware. Do not edit these
    // aliases; change their validated CONFIGURED_* sources in ArrowLab.conf.
    constexpr MeasurementLinkMode MEASUREMENT_LINK_MODE =
        static_cast<MeasurementLinkMode>(CONFIGURED_MEASUREMENT_LINK_MODE);
    constexpr bool DEVELOPER_MODE_DEFAULT_ENABLED =
        CONFIGURED_DEVELOPER_MODE_DEFAULT_ENABLED;
    constexpr uint32_t OPERATIONAL_WEIGHING_TIME_MS =
        CONFIGURED_OPERATIONAL_WEIGHING_TIME_MS;
    constexpr MassUnit PRIMARY_MASS_UNIT =
        static_cast<MassUnit>(CONFIGURED_PRIMARY_MASS_UNIT);
    constexpr uint8_t GRAMS_DECIMAL_PLACES =
        CONFIGURED_GRAMS_DECIMAL_PLACES;
    constexpr uint8_t GRAINS_DECIMAL_PLACES =
        CONFIGURED_GRAINS_DECIMAL_PLACES;
    constexpr uint8_t OUNCES_DECIMAL_PLACES =
        CONFIGURED_OUNCES_DECIMAL_PLACES;
    constexpr uint32_t SPINE_HOLD_TIME_MS =
        CONFIGURED_SPINE_HOLD_TIME_MS;
    constexpr float SPINE_STABILITY_BAND_GRAMS =
        CONFIGURED_SPINE_STABILITY_BAND_GRAMS;
    constexpr float SPINE_MINIMUM_APPLIED_FORCE_GRAMS =
        CONFIGURED_SPINE_MINIMUM_APPLIED_FORCE_GRAMS;
    constexpr float SPINE_BASELINE_TRACKING_BAND_GRAMS =
        CONFIGURED_SPINE_BASELINE_TRACKING_BAND_GRAMS;
    constexpr float SPINE_RELEASE_FORCE_GRAMS =
        CONFIGURED_SPINE_RELEASE_FORCE_GRAMS;
    constexpr uint32_t SPINE_RELEASE_TIME_MS =
        CONFIGURED_SPINE_RELEASE_TIME_MS;
    constexpr float ARROW_PRESENT_GRAMS =
        CONFIGURED_ARROW_PRESENT_GRAMS;
    constexpr uint32_t ARROW_STABILITY_TIME_MS =
        CONFIGURED_ARROW_STABILITY_TIME_MS;
    constexpr float ARROW_STABILITY_BAND_GRAMS =
        CONFIGURED_ARROW_STABILITY_BAND_GRAMS;

    constexpr float GRAINS_PER_GRAM = 15.4323583529f;
    constexpr float OUNCES_PER_GRAM = 0.03527396195f;

    constexpr uint32_t MEASUREMENT_LINK_BAUD = 115200;
    constexpr uint32_t MEASUREMENT_STATUS_INTERVAL_MS = 50;

    constexpr bool measurementLinkIsOneWire()
    {
        return MEASUREMENT_LINK_MODE
            == MeasurementLinkMode::VieweGpio17OneWire;
    }

    constexpr int8_t vieweMeasurementRxPin()
    {
        return MEASUREMENT_LINK_MODE
                == MeasurementLinkMode::VieweGpio11And12
            ? 11
            : MEASUREMENT_LINK_MODE
                    == MeasurementLinkMode::VieweGpio17OneWire
                ? 17
                : 44;
    }

    constexpr int8_t vieweMeasurementTxPin()
    {
        return MEASUREMENT_LINK_MODE
                == MeasurementLinkMode::VieweGpio11And12
            ? 12
            : MEASUREMENT_LINK_MODE
                    == MeasurementLinkMode::VieweGpio17OneWire
                ? 17
                : 43;
    }

    constexpr int8_t wroomMeasurementRxPin()
    {
        return 8;
    }

    constexpr int8_t wroomMeasurementTxPin()
    {
        return measurementLinkIsOneWire() ? 8 : 9;
    }
}
