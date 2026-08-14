#pragma once

#include <cstddef>

#include "ArrowLabConfig.h"

namespace MassDisplay
{
    ArrowLabConfig::MassUnit nextUnit(ArrowLabConfig::MassUnit unit);
    const char *unitText(ArrowLabConfig::MassUnit unit);
    unsigned int decimalPlaces(ArrowLabConfig::MassUnit unit);
    float convertFromGrams(float grams, ArrowLabConfig::MassUnit unit);

    void formatPrimary(
        char *buffer,
        size_t bufferSize,
        float grams,
        ArrowLabConfig::MassUnit unit);

    void formatSecondary(
        char *buffer,
        size_t bufferSize,
        float grams,
        ArrowLabConfig::MassUnit primaryUnit);
}
