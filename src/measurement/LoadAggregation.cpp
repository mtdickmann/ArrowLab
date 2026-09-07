#include "LoadAggregation.h"

namespace LoadAggregation
{
    Result combine(const Channel &left, const Channel &right)
    {
        if (!left.ready && !right.ready) {
            return {Source::NotReady, 0};
        }

        const bool leftActive = left.ready && left.heldRawCounts != 0;
        const bool rightActive = right.ready && right.heldRawCounts != 0;

        if (leftActive && rightActive) {
            return {
                Source::LeftAndRight,
                left.heldMilliGrams + right.heldMilliGrams};
        }
        if (leftActive) {
            return {Source::Left, left.heldMilliGrams};
        }
        if (rightActive) {
            return {Source::Right, right.heldMilliGrams};
        }

        return {Source::NoLoad, 0};
    }

    const char *sourceText(Source source)
    {
        switch (source) {
        case Source::Left:
            return "LEFT CASSETTE";
        case Source::Right:
            return "RIGHT CASSETTE";
        case Source::LeftAndRight:
            return "LEFT + RIGHT";
        case Source::NoLoad:
            return "NO LOAD";
        case Source::NotReady:
        default:
            return "NOT READY";
        }
    }
}
