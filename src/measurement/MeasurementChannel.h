#pragma once

#include <cstddef>
#include <cstdint>

/*
 * Operational weighing state for one raw load-cell channel.
 *
 * The displayed result is event based:
 *   - a deliberate tare establishes zero;
 *   - a slow raw tracker follows creep/drift while the display is held;
 *   - a genuine step freezes the pre-change raw position;
 *   - the new state is averaged for at most ten seconds;
 *   - only the before/after difference changes the held result.
 *
 * Calibration and diagnostics do not get alternative weighing maths.
 */
class MeasurementChannel
{
public:
    enum class State
    {
        NeedsTare,
        Taring,
        Tracking,
        AcquiringChange
    };

    static constexpr uint8_t TARE_SAMPLE_COUNT = 20;
    static constexpr uint8_t FILTER_SAMPLE_COUNT = 15;
    static constexpr uint8_t CHANGE_CONFIRM_SAMPLES = 4;
    static constexpr uint8_t STABLE_SAMPLE_COUNT = 20;
    static constexpr uint8_t ACQUISITION_SAMPLE_CAPACITY = 100;
    static constexpr long CHANGE_THRESHOLD_COUNTS = 300;
    static constexpr long STABLE_RANGE_COUNTS = 300;
    static constexpr float RETURN_TO_ZERO_ABSOLUTE_GRAMS = 0.5f;
    static constexpr float RETURN_TO_ZERO_RELATIVE = 0.002f;
    static constexpr uint32_t MIN_ACQUISITION_MS = 2000;
    static constexpr uint32_t MAX_ACQUISITION_MS = 10000;

    void onRawSample(long rawCount, uint32_t currentTime);
    void startTare(bool confirmAsUserTare = true);

    State state() const;
    bool tareComplete() const;
    bool userTareConfirmed() const;
    bool changeInProgress() const;

    long rawValue() const;
    long tareReference() const;
    long zeroedRaw() const;
    long filteredRaw() const;
    long filteredZeroedRaw() const;
    long trackedRaw() const;
    long heldRawCounts() const;

    bool calibrated() const;
    float countsPerGram() const;
    float calibrationReferenceGrams() const;
    float heldGrams() const;
    float instantaneousGrams() const;
    void applyCalibration(float countsPerGram, float referenceGrams);
    void restoreCalibration(float countsPerGram, float referenceGrams);
    void clearCalibration();

private:
    static long magnitude(long value);
    static long robustAverage(const long *values, uint8_t count);
    static long sampleRange(const long *values, uint8_t count);

    void resetFilter();
    void updateFilter(long rawCount);
    void updateTare(long rawCount);
    void updateTracking(uint32_t currentTime);
    void beginChange(uint32_t currentTime);
    void updateChange(uint32_t currentTime);
    void completeChange();

    State state_ = State::NeedsTare;
    long rawValue_ = 0;

    long filterSamples_[FILTER_SAMPLE_COUNT] = {};
    uint8_t filterSampleCount_ = 0;
    uint8_t filterWriteIndex_ = 0;
    long filteredRaw_ = 0;

    int64_t tareAccumulator_ = 0;
    uint8_t tareSampleCount_ = 0;
    long tareReference_ = 0;
    bool userTareConfirmed_ = false;
    bool confirmUserTareOnCompletion_ = false;

    int64_t trackedRawScaled_ = 0;
    long heldRawCounts_ = 0;
    float heldGrams_ = 0.0f;
    int8_t candidateDirection_ = 0;
    uint8_t candidateSamples_ = 0;

    long preChangeRaw_ = 0;
    long acquisitionSamples_[ACQUISITION_SAMPLE_CAPACITY] = {};
    uint8_t acquisitionSampleCount_ = 0;
    uint32_t acquisitionStartedAt_ = 0;

    float countsPerGram_ = 0.0f;
    float calibrationReferenceGrams_ = 0.0f;
    bool calibrated_ = false;
};
