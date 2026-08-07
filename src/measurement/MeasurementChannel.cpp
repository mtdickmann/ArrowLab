#include "MeasurementChannel.h"

#include <climits>
#include <cmath>

namespace
{
    constexpr int64_t TRACK_SCALE = 256;
    constexpr int64_t TRACK_DIVISOR = 16;
}

void MeasurementChannel::onRawSample(
    long rawCount,
    uint32_t currentTime
)
{
    rawValue_ = rawCount;

    if (state_ == State::Taring) {
        updateTare(rawCount);
        return;
    }

    updateFilter(rawCount);

    if (state_ == State::Tracking) {
        updateTracking(currentTime);
    } else if (state_ == State::AcquiringChange) {
        updateChange(currentTime);
    }
}

void MeasurementChannel::startTare(bool confirmAsUserTare)
{
    state_ = State::Taring;
    tareAccumulator_ = 0;
    tareSampleCount_ = 0;
    userTareConfirmed_ = false;
    confirmUserTareOnCompletion_ = confirmAsUserTare;
    tareReference_ = 0;
    heldRawCounts_ = 0;
    heldGrams_ = 0.0f;
    candidateDirection_ = 0;
    candidateSamples_ = 0;
    acquisitionSampleCount_ = 0;
    acquisitionStartedAt_ = 0;
    resetFilter();
}

MeasurementChannel::State MeasurementChannel::state() const
{
    return state_;
}

bool MeasurementChannel::tareComplete() const
{
    return state_ == State::Tracking
        || state_ == State::AcquiringChange;
}

bool MeasurementChannel::userTareConfirmed() const
{
    return tareComplete() && userTareConfirmed_;
}

bool MeasurementChannel::changeInProgress() const
{
    return state_ == State::AcquiringChange;
}

long MeasurementChannel::rawValue() const
{
    return rawValue_;
}

long MeasurementChannel::tareReference() const
{
    return tareReference_;
}

long MeasurementChannel::zeroedRaw() const
{
    return tareComplete() ? rawValue_ - tareReference_ : 0;
}

long MeasurementChannel::filteredRaw() const
{
    return filteredRaw_;
}

long MeasurementChannel::filteredZeroedRaw() const
{
    return tareComplete() ? filteredRaw_ - tareReference_ : 0;
}

long MeasurementChannel::trackedRaw() const
{
    return static_cast<long>(trackedRawScaled_ / TRACK_SCALE);
}

long MeasurementChannel::heldRawCounts() const
{
    return heldRawCounts_;
}

bool MeasurementChannel::calibrated() const
{
    return calibrated_;
}

float MeasurementChannel::countsPerGram() const
{
    return countsPerGram_;
}

float MeasurementChannel::calibrationReferenceGrams() const
{
    return calibrationReferenceGrams_;
}

float MeasurementChannel::heldGrams() const
{
    return calibrated_ ? heldGrams_ : 0.0f;
}

float MeasurementChannel::instantaneousGrams() const
{
    if (!calibrated_ || countsPerGram_ == 0.0f || !tareComplete()) {
        return 0.0f;
    }

    return static_cast<float>(zeroedRaw()) / countsPerGram_;
}

void MeasurementChannel::applyCalibration(
    float countsPerGram,
    float referenceGrams
)
{
    if (countsPerGram == 0.0f || referenceGrams <= 0.0f) {
        return;
    }

    countsPerGram_ = countsPerGram;
    calibrationReferenceGrams_ = referenceGrams;
    calibrated_ = true;

    // The calibration mass is known by definition at this instant.  Anchor
    // the held result to that known value and continue raw tracking from the
    // same instant; no 10 s/30 s timing mismatch is carried into the display.
    heldGrams_ = referenceGrams;
    heldRawCounts_ = static_cast<long>(
        countsPerGram_ * calibrationReferenceGrams_);
    trackedRawScaled_ =
        static_cast<int64_t>(filteredRaw_) * TRACK_SCALE;
    candidateDirection_ = 0;
    candidateSamples_ = 0;
    acquisitionSampleCount_ = 0;
    state_ = State::Tracking;
}

void MeasurementChannel::restoreCalibration(
    float countsPerGram,
    float referenceGrams
)
{
    if (countsPerGram == 0.0f || referenceGrams <= 0.0f) {
        clearCalibration();
        return;
    }

    countsPerGram_ = countsPerGram;
    calibrationReferenceGrams_ = referenceGrams;
    calibrated_ = true;
}

void MeasurementChannel::clearCalibration()
{
    countsPerGram_ = 0.0f;
    calibrationReferenceGrams_ = 0.0f;
    heldGrams_ = 0.0f;
    calibrated_ = false;
}

long MeasurementChannel::magnitude(long value)
{
    if (value == LONG_MIN) {
        return LONG_MAX;
    }
    return value < 0 ? -value : value;
}

long MeasurementChannel::robustAverage(
    const long *values,
    uint8_t count
)
{
    if (count == 0) {
        return 0;
    }

    long ordered[ACQUISITION_SAMPLE_CAPACITY];
    for (uint8_t index = 0; index < count; ++index) {
        ordered[index] = values[index];
    }

    for (uint8_t index = 1; index < count; ++index) {
        const long value = ordered[index];
        uint8_t position = index;
        while (position > 0 && ordered[position - 1] > value) {
            ordered[position] = ordered[position - 1];
            --position;
        }
        ordered[position] = value;
    }

    const uint8_t trim = count >= 20 ? count / 5 : 0;
    int64_t total = 0;
    for (uint8_t index = trim; index < count - trim; ++index) {
        total += ordered[index];
    }
    return static_cast<long>(total / (count - 2 * trim));
}

long MeasurementChannel::sampleRange(
    const long *values,
    uint8_t count
)
{
    if (count == 0) {
        return 0;
    }

    long minimum = values[0];
    long maximum = values[0];
    for (uint8_t index = 1; index < count; ++index) {
        if (values[index] < minimum) minimum = values[index];
        if (values[index] > maximum) maximum = values[index];
    }
    return maximum - minimum;
}

void MeasurementChannel::resetFilter()
{
    filterSampleCount_ = 0;
    filterWriteIndex_ = 0;
    filteredRaw_ = rawValue_;
}

void MeasurementChannel::updateFilter(long rawCount)
{
    filterSamples_[filterWriteIndex_] = rawCount;
    filterWriteIndex_ = static_cast<uint8_t>(
        (filterWriteIndex_ + 1) % FILTER_SAMPLE_COUNT);

    if (filterSampleCount_ < FILTER_SAMPLE_COUNT) {
        ++filterSampleCount_;
    }

    filteredRaw_ = robustAverage(
        filterSamples_,
        filterSampleCount_);
}

void MeasurementChannel::updateTare(long rawCount)
{
    tareAccumulator_ += rawCount;
    ++tareSampleCount_;

    if (tareSampleCount_ < TARE_SAMPLE_COUNT) {
        return;
    }

    tareReference_ = static_cast<long>(
        tareAccumulator_ / TARE_SAMPLE_COUNT);
    rawValue_ = rawCount;
    resetFilter();
    updateFilter(rawCount);
    trackedRawScaled_ =
        static_cast<int64_t>(tareReference_) * TRACK_SCALE;
    heldRawCounts_ = 0;
    heldGrams_ = 0.0f;
    userTareConfirmed_ = confirmUserTareOnCompletion_;
    confirmUserTareOnCompletion_ = false;
    state_ = State::Tracking;
}

void MeasurementChannel::updateTracking(uint32_t currentTime)
{
    if (filterSampleCount_ < FILTER_SAMPLE_COUNT) {
        trackedRawScaled_ =
            static_cast<int64_t>(filteredRaw_) * TRACK_SCALE;
        return;
    }

    const long tracker = trackedRaw();
    // Compare the fresh conversion with the slow tracker so an abrupt step is
    // frozen before the display filter has time to drag the reference along.
    const long difference = rawValue_ - tracker;
    const int8_t direction = difference > 0 ? 1 : -1;

    if (magnitude(difference) >= CHANGE_THRESHOLD_COUNTS) {
        if (candidateDirection_ == direction) {
            ++candidateSamples_;
        } else {
            candidateDirection_ = direction;
            candidateSamples_ = 1;
        }

        if (candidateSamples_ >= CHANGE_CONFIRM_SAMPLES) {
            beginChange(currentTime);
        }
        return;
    }

    candidateDirection_ = 0;
    candidateSamples_ = 0;

    // Slow background tracker: follows creep/drift but not a genuine step.
    const int64_t target =
        static_cast<int64_t>(filteredRaw_) * TRACK_SCALE;
    trackedRawScaled_ +=
        (target - trackedRawScaled_) / TRACK_DIVISOR;
}

void MeasurementChannel::beginChange(uint32_t currentTime)
{
    preChangeRaw_ = trackedRaw();
    acquisitionSampleCount_ = 0;
    acquisitionStartedAt_ = currentTime;
    candidateDirection_ = 0;
    candidateSamples_ = 0;
    state_ = State::AcquiringChange;
}

void MeasurementChannel::updateChange(uint32_t currentTime)
{
    if (acquisitionSampleCount_ < ACQUISITION_SAMPLE_CAPACITY) {
        acquisitionSamples_[acquisitionSampleCount_++] = rawValue_;
    }

    const uint32_t elapsed = currentTime - acquisitionStartedAt_;
    bool stable = false;

    if (
        elapsed >= MIN_ACQUISITION_MS
        && acquisitionSampleCount_ >= STABLE_SAMPLE_COUNT
    ) {
        stable = sampleRange(
            acquisitionSamples_
                + acquisitionSampleCount_ - STABLE_SAMPLE_COUNT,
            STABLE_SAMPLE_COUNT) <= STABLE_RANGE_COUNTS;
    }

    if (!stable && elapsed < MAX_ACQUISITION_MS) {
        return;
    }

    completeChange();
}

void MeasurementChannel::completeChange()
{
    const uint8_t count = acquisitionSampleCount_;
    const uint8_t windowCount =
        count > STABLE_SAMPLE_COUNT ? STABLE_SAMPLE_COUNT : count;
    const long newRaw = robustAverage(
        acquisitionSamples_ + count - windowCount,
        windowCount);
    const long deltaCounts = newRaw - preChangeRaw_;

    heldRawCounts_ += deltaCounts;
    if (calibrated_ && countsPerGram_ != 0.0f) {
        const float previousHeldGrams = heldGrams_;
        heldGrams_ +=
            static_cast<float>(deltaCounts) / countsPerGram_;

        const float zeroTolerance = std::fmax(
            RETURN_TO_ZERO_ABSOLUTE_GRAMS,
            std::fabs(previousHeldGrams) * RETURN_TO_ZERO_RELATIVE);
        const bool movingTowardZero =
            std::fabs(heldGrams_) < std::fabs(previousHeldGrams);

        if (movingTowardZero && std::fabs(heldGrams_) <= zeroTolerance) {
            heldGrams_ = 0.0f;
            heldRawCounts_ = 0;
        }
    }

    trackedRawScaled_ =
        static_cast<int64_t>(newRaw) * TRACK_SCALE;
    acquisitionSampleCount_ = 0;
    acquisitionStartedAt_ = 0;
    state_ = State::Tracking;
}
