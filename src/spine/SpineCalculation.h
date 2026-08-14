#pragma once

#include <cstdint>

namespace SpineCalculation
{
    constexpr float ATA_REFERENCE_FORCE_GRAMS = 880.0f;
    constexpr float ATA_REFERENCE_SPINE = 500.0f;

    inline float equivalentSpine(float appliedForceGrams)
    {
        return appliedForceGrams > 0.0f
            ? ATA_REFERENCE_SPINE * ATA_REFERENCE_FORCE_GRAMS
                / appliedForceGrams
            : 0.0f;
    }

    inline float markedDifference(float measuredSpine, float markedSpine)
    {
        return measuredSpine - markedSpine;
    }

    inline float markedDifferencePercent(
        float measuredSpine,
        float markedSpine)
    {
        return markedSpine > 0.0f
            ? markedDifference(measuredSpine, markedSpine)
                * 100.0f / markedSpine
            : 0.0f;
    }
}
