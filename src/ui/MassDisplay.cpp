#include "MassDisplay.h"

#include <cstdio>

namespace MassDisplay
{
    ArrowLabConfig::MassUnit nextUnit(ArrowLabConfig::MassUnit unit)
    {
        switch (unit) {
        case ArrowLabConfig::MassUnit::Grams:
            return ArrowLabConfig::MassUnit::Grains;
        case ArrowLabConfig::MassUnit::Grains:
            return ArrowLabConfig::MassUnit::Ounces;
        case ArrowLabConfig::MassUnit::Ounces:
        default:
            return ArrowLabConfig::MassUnit::Grams;
        }
    }

    const char *unitText(ArrowLabConfig::MassUnit unit)
    {
        switch (unit) {
        case ArrowLabConfig::MassUnit::Grains:
            return "gr";
        case ArrowLabConfig::MassUnit::Ounces:
            return "oz";
        case ArrowLabConfig::MassUnit::Grams:
        default:
            return "g";
        }
    }

    unsigned int decimalPlaces(ArrowLabConfig::MassUnit unit)
    {
        switch (unit) {
        case ArrowLabConfig::MassUnit::Grains:
            return ArrowLabConfig::GRAINS_DECIMAL_PLACES;
        case ArrowLabConfig::MassUnit::Ounces:
            return ArrowLabConfig::OUNCES_DECIMAL_PLACES;
        case ArrowLabConfig::MassUnit::Grams:
        default:
            return ArrowLabConfig::GRAMS_DECIMAL_PLACES;
        }
    }

    float convertFromGrams(float grams, ArrowLabConfig::MassUnit unit)
    {
        switch (unit) {
        case ArrowLabConfig::MassUnit::Grains:
            return grams * ArrowLabConfig::GRAINS_PER_GRAM;
        case ArrowLabConfig::MassUnit::Ounces:
            return grams * ArrowLabConfig::OUNCES_PER_GRAM;
        case ArrowLabConfig::MassUnit::Grams:
        default:
            return grams;
        }
    }

    void formatPrimary(
        char *buffer,
        size_t bufferSize,
        float grams,
        ArrowLabConfig::MassUnit unit)
    {
        snprintf(
            buffer,
            bufferSize,
            "%.*f",
            static_cast<int>(decimalPlaces(unit)),
            convertFromGrams(grams, unit));
    }

    namespace
    {
        void appendValue(
            char *buffer,
            size_t bufferSize,
            size_t &used,
            float grams,
            ArrowLabConfig::MassUnit unit)
        {
            if (used >= bufferSize) {
                return;
            }

            const int written = snprintf(
                buffer + used,
                bufferSize - used,
                used == 0 ? "%.*f %s" : "   %.*f %s",
                static_cast<int>(decimalPlaces(unit)),
                convertFromGrams(grams, unit),
                unitText(unit));

            if (written > 0) {
                used += static_cast<size_t>(written);
            }
        }
    }

    void formatSecondary(
        char *buffer,
        size_t bufferSize,
        float grams,
        ArrowLabConfig::MassUnit primaryUnit)
    {
        if (bufferSize == 0) {
            return;
        }

        buffer[0] = '\0';
        size_t used = 0;

        if (primaryUnit != ArrowLabConfig::MassUnit::Grams) {
            appendValue(
                buffer,
                bufferSize,
                used,
                grams,
                ArrowLabConfig::MassUnit::Grams);
        }
        if (primaryUnit != ArrowLabConfig::MassUnit::Grains) {
            appendValue(
                buffer,
                bufferSize,
                used,
                grams,
                ArrowLabConfig::MassUnit::Grains);
        }
        if (primaryUnit != ArrowLabConfig::MassUnit::Ounces) {
            appendValue(
                buffer,
                bufferSize,
                used,
                grams,
                ArrowLabConfig::MassUnit::Ounces);
        }
    }
}
