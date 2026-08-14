#include "measurement/LoadAggregation.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    using namespace LoadAggregation;

    Channel left;
    Channel right;
    assert(combine(left, right).source == Source::NotReady);

    left.ready = true;
    assert(combine(left, right).source == Source::NoLoad);

    left.heldRawCounts = 20000;
    left.heldMilliGrams = 21000;
    Result result = combine(left, right);
    assert(result.source == Source::Left);
    assert(result.totalMilliGrams == 21000);

    right.ready = true;
    right.heldRawCounts = 45000;
    right.heldMilliGrams = 46000;
    result = combine(left, right);
    assert(result.source == Source::LeftAndRight);
    assert(result.totalMilliGrams == 67000);
    assert(std::strcmp(sourceText(result.source), "LEFT + RIGHT") == 0);

    left.heldRawCounts = 0;
    left.heldMilliGrams = 0;
    result = combine(left, right);
    assert(result.source == Source::Right);
    assert(result.totalMilliGrams == 46000);

    std::cout << "Load aggregation tests passed\n";
    return 0;
}
