#pragma once

#include <cstdint>

namespace ArrowLabConfig
{
    enum class MeasurementLinkMode : uint8_t
    {
        // Proven configuration. It preserves display and touch, but occupies
        // the VIEWE onboard SD-card MOSI/SCK pins.
        VieweGpio11And12 = 0,

        // Experimental half-duplex link: VIEWE GPIO17 to WROOM GPIO8.
        // Fit one external 4.7 kOhm pull-up from the shared signal to 3.3 V.
        VieweGpio17OneWire = 1,

        // Experimental full-duplex link using the VIEWE UART0 pins. NEVER
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

    // Stable, strongly typed names consumed by the firmware. Do not edit these
    // aliases; change their validated CONFIGURED_* sources in ArrowLab.conf.
    constexpr MeasurementLinkMode MEASUREMENT_LINK_MODE =
        static_cast<MeasurementLinkMode>(CONFIGURED_MEASUREMENT_LINK_MODE);
    constexpr bool DEVELOPER_MODE_DEFAULT_ENABLED =
        CONFIGURED_DEVELOPER_MODE_DEFAULT_ENABLED;

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
