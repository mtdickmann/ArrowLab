#pragma once

#include <cstdint>

namespace LoadAggregation
{
    enum class Source
    {
        NotReady,
        NoLoad,
        Left,
        Right,
        LeftAndRight
    };

    struct Channel
    {
        bool ready = false;
        int32_t heldRawCounts = 0;
        int32_t heldMilliGrams = 0;
    };

    struct Result
    {
        Source source = Source::NotReady;
        int32_t totalMilliGrams = 0;
    };

    Result combine(const Channel &left, const Channel &right);
    const char *sourceText(Source source);
}
