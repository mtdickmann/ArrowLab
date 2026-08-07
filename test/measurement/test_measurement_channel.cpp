#include "measurement/MeasurementChannel.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace
{
    constexpr long EMPTY_RAW = 100000;
    constexpr float COUNTS_PER_GRAM = 1000.0f;

    void feed(
        MeasurementChannel &channel,
        long raw,
        uint32_t &time,
        int samples,
        long driftPerSample = 0
    )
    {
        for (int index = 0; index < samples; ++index) {
            channel.onRawSample(raw, time);
            raw += driftPerSample;
            time += 100;
        }
    }

    void expectNear(float actual, float expected, float tolerance)
    {
        assert(std::fabs(actual - expected) <= tolerance);
    }
}

int main()
{
    MeasurementChannel channel;
    uint32_t time = 0;

    channel.startTare();
    feed(channel, EMPTY_RAW, time, 20);
    assert(channel.tareComplete());
    assert(channel.userTareConfirmed());
    assert(channel.heldRawCounts() == 0);

    // Calibration anchors the known mass and persistent factor once.
    feed(channel, EMPTY_RAW + 1000000, time, 40);
    channel.applyCalibration(COUNTS_PER_GRAM, 1000.0f);
    expectNear(channel.heldGrams(), 1000.0f, 0.001f);

    // Slow creep follows the private raw tracker but never moves the display.
    long driftingRaw = EMPTY_RAW + 1000000;
    for (int index = 0; index < 600; ++index) {
        channel.onRawSample(driftingRaw, time);
        if (index % 6 == 0) ++driftingRaw;
        time += 100;
    }
    expectNear(channel.heldGrams(), 1000.0f, 0.001f);

    // A discrete removal is measured against the currently tracked raw state.
    feed(channel, EMPTY_RAW + 100, time, 140);
    expectNear(channel.heldGrams(), 0.0f, 0.001f);

    // A new, different object produces a fresh event-held result.
    feed(channel, EMPTY_RAW + 20000, time, 140);
    expectNear(channel.heldGrams(), 20.0f, 0.5f);

    // Replacing it with a 50 g object adds only the before/after difference.
    feed(channel, EMPTY_RAW + 50000, time, 140);
    expectNear(channel.heldGrams(), 50.0f, 0.6f);

    // Observed sub-threshold HX711 jitter must not create a new load event.
    for (int index = 0; index < 200; ++index) {
        const long jitter = index % 2 == 0 ? 180 : -180;
        channel.onRawSample(EMPTY_RAW + 50000 + jitter, time);
        time += 100;
    }
    expectNear(channel.heldGrams(), 50.0f, 0.6f);

    // An unsettled change is still bounded by the ten-second maximum.
    for (int index = 0; index < 105; ++index) {
        const long unsettled = index % 2 == 0 ? 60000 : 61000;
        channel.onRawSample(EMPTY_RAW + unsettled, time);
        time += 100;
    }
    assert(!channel.changeInProgress());
    expectNear(channel.heldGrams(), 60.5f, 1.0f);

    // Deliberate tare replaces zero but retains persistent calibration.
    channel.startTare();
    feed(channel, EMPTY_RAW + 50000, time, 20);
    expectNear(channel.heldGrams(), 0.0f, 0.001f);
    assert(channel.heldRawCounts() == 0);
    assert(channel.calibrated());

    // Opposite electrical polarity must produce the same positive mass.
    MeasurementChannel inverted;
    inverted.startTare();
    feed(inverted, EMPTY_RAW, time, 20);
    feed(inverted, EMPTY_RAW - 1000000, time, 40);
    inverted.applyCalibration(-COUNTS_PER_GRAM, 1000.0f);
    expectNear(inverted.heldGrams(), 1000.0f, 0.001f);
    feed(inverted, EMPTY_RAW, time, 140);
    expectNear(inverted.heldGrams(), 0.0f, 0.001f);
    feed(inverted, EMPTY_RAW - 20000, time, 140);
    expectNear(inverted.heldGrams(), 20.0f, 0.5f);

    std::cout << "MeasurementChannel tests passed\n";
    return 0;
}
