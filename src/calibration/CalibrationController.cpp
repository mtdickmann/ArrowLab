#include "CalibrationController.h"

#include <climits>

namespace
{
    long magnitude(long value)
    {
        if (value == LONG_MIN) return LONG_MAX;
        return value < 0 ? -value : value;
    }
}

CalibrationController::CalibrationController(
    MeasurementChannel &left,
    MeasurementChannel &right,
    InstrumentStorage &storage
)
    : left_(left), right_(right), storage_(storage)
{
}

void CalibrationController::begin()
{
    restore(CalibrationSide::Left);
    restore(CalibrationSide::Right);
}

void CalibrationController::restore(CalibrationSide side)
{
    StoredCalibration record;
    if (!storage_.loadCalibration(storedSide(side), record)) {
        return;
    }

    measurement(side).restoreCalibration(
        record.factor,
        record.referenceGrams);
    workflow(side).referenceGrams = record.referenceGrams;

    Serial.printf(
        "Restored %s calibration from NVS\n",
        side == CalibrationSide::Left ? "LEFT" : "RIGHT");
}

void CalibrationController::update(uint32_t currentTime)
{
    processRequests(currentTime);
    updateChannel(CalibrationSide::Left, currentTime);
    updateChannel(CalibrationSide::Right, currentTime);
}

void CalibrationController::processRequests(uint32_t currentTime)
{
    if (leftTareRequested_) {
        leftTareRequested_ = false;
        startTare(CalibrationSide::Left);
    }
    if (rightTareRequested_) {
        rightTareRequested_ = false;
        startTare(CalibrationSide::Right);
    }
    if (leftCalibrationRequested_) {
        leftCalibrationRequested_ = false;
        performCalibrationAction(
            CalibrationSide::Left,
            leftRequestedReferenceGrams_,
            currentTime);
    }
    if (rightCalibrationRequested_) {
        rightCalibrationRequested_ = false;
        performCalibrationAction(
            CalibrationSide::Right,
            rightRequestedReferenceGrams_,
            currentTime);
    }
}

void CalibrationController::updateChannel(
    CalibrationSide side,
    uint32_t currentTime
)
{
    MeasurementChannel &channel = measurement(side);
    ChannelWorkflow &state = workflow(side);

    if (state.stage == Stage::Taring && channel.tareComplete()) {
        state.stage = Stage::Ready;
    }

    if (
        state.stage == Stage::Settling
        && currentTime - state.settleStartedAt >= SETTLE_TIME_MS
    ) {
        state.stage = Stage::Sampling;
        state.calibrationAccumulator = 0;
        state.calibrationSamples = 0;
    }
}

void CalibrationController::onFreshReading(CalibrationSide side)
{
    MeasurementChannel &channel = measurement(side);
    ChannelWorkflow &state = workflow(side);

    if (state.stage == Stage::AwaitingLoad) {
        if (
            magnitude(channel.filteredZeroedRaw())
            >= LOAD_THRESHOLD_COUNTS
        ) {
            if (state.loadConfirmSamples < LOAD_CONFIRM_SAMPLES) {
                ++state.loadConfirmSamples;
            }
            if (state.loadConfirmSamples >= LOAD_CONFIRM_SAMPLES) {
                state.stage = Stage::ReadyToCalibrate;
            }
        } else {
            state.loadConfirmSamples = 0;
        }
        return;
    }

    if (state.stage != Stage::Sampling) {
        return;
    }

    state.calibrationAccumulator += channel.filteredZeroedRaw();
    ++state.calibrationSamples;

    if (state.calibrationSamples >= CALIBRATION_SAMPLE_COUNT) {
        finishCalibration(side);
    }
}

void CalibrationController::finishCalibration(CalibrationSide side)
{
    MeasurementChannel &channel = measurement(side);
    ChannelWorkflow &state = workflow(side);
    const float averageCounts =
        static_cast<float>(state.calibrationAccumulator)
        / CALIBRATION_SAMPLE_COUNT;

    if (averageCounts == 0.0f || state.referenceGrams <= 0.0f) {
        state.stage = Stage::ReadyToCalibrate;
        Serial.printf(
            "WARNING: %s calibration failed: invalid span\n",
            side == CalibrationSide::Left ? "LEFT" : "RIGHT");
        return;
    }

    const float countsPerGram =
        averageCounts / state.referenceGrams;
    channel.applyCalibration(countsPerGram, state.referenceGrams);
    state.stage = Stage::Ready;
    state.loadConfirmSamples = 0;

    const bool stored = storage_.saveCalibration(
        storedSide(side),
        countsPerGram,
        state.referenceGrams);

    Serial.printf(
        "%s calibration complete: %.3f counts/g "
        "(average=%.1f, reference=%.1f g, stored=%s)\n",
        side == CalibrationSide::Left ? "LEFT" : "RIGHT",
        countsPerGram,
        averageCounts,
        state.referenceGrams,
        stored ? "yes" : "no");
}

void CalibrationController::requestTare(CalibrationSide side)
{
    if (side == CalibrationSide::Left) {
        leftTareRequested_ = true;
    } else {
        rightTareRequested_ = true;
    }
}

bool CalibrationController::startTare(CalibrationSide side)
{
    ChannelWorkflow &state = workflow(side);
    const ChannelWorkflow &other = workflow(
        side == CalibrationSide::Left
            ? CalibrationSide::Right
            : CalibrationSide::Left);

    if (
        state.stage == Stage::Settling
        || state.stage == Stage::Sampling
        || other.stage == Stage::Settling
        || other.stage == Stage::Sampling
    ) {
        return false;
    }

    measurement(side).startTare(true);
    state.stage = Stage::Taring;
    state.referenceGrams = measurement(side).calibrationReferenceGrams();
    state.settleStartedAt = 0;
    state.loadConfirmSamples = 0;
    state.calibrationAccumulator = 0;
    state.calibrationSamples = 0;
    return true;
}

void CalibrationController::requestCalibration(
    CalibrationSide side,
    float referenceGrams
)
{
    if (side == CalibrationSide::Left) {
        leftRequestedReferenceGrams_ = referenceGrams;
        leftCalibrationRequested_ = true;
    } else {
        rightRequestedReferenceGrams_ = referenceGrams;
        rightCalibrationRequested_ = true;
    }
}

bool CalibrationController::performCalibrationAction(
    CalibrationSide side,
    float referenceGrams,
    uint32_t currentTime
)
{
    MeasurementChannel &channel = measurement(side);
    ChannelWorkflow &state = workflow(side);
    const ChannelWorkflow &other = workflow(
        side == CalibrationSide::Left
            ? CalibrationSide::Right
            : CalibrationSide::Left);

    if (!channel.userTareConfirmed()) {
        return false;
    }

    if (referenceGrams > 0.0f && state.stage == Stage::Ready) {
        state.referenceGrams = referenceGrams;
        state.loadConfirmSamples = 0;
        state.stage = Stage::AwaitingLoad;
        return true;
    }

    if (referenceGrams <= 0.0f && state.stage == Stage::ReadyToCalibrate) {
        if (
            other.stage == Stage::Settling
            || other.stage == Stage::Sampling
        ) {
            return false;
        }
        state.settleStartedAt = currentTime;
        state.stage = Stage::Settling;
        return true;
    }

    return false;
}

CalibrationController::ChannelStatus CalibrationController::status(
    CalibrationSide side,
    uint32_t currentTime
) const
{
    const ChannelWorkflow &state = workflow(side);
    const MeasurementChannel &channel = measurement(side);
    ChannelStatus result;
    result.stage = state.stage;
    result.tareComplete = channel.userTareConfirmed();
    result.calibrated = channel.calibrated();
    result.referenceGrams = state.referenceGrams;

    if (state.stage == Stage::Settling) {
        const uint32_t elapsed = currentTime - state.settleStartedAt;
        if (elapsed < SETTLE_TIME_MS) {
            result.settleRemainingSeconds =
                (SETTLE_TIME_MS - elapsed + 999) / 1000;
            result.settlePercent = static_cast<uint8_t>(
                (static_cast<uint64_t>(elapsed) * 100)
                / SETTLE_TIME_MS);
        } else {
            result.settlePercent = 100;
        }
    }

    return result;
}

MeasurementChannel &CalibrationController::measurement(
    CalibrationSide side
)
{
    return side == CalibrationSide::Left ? left_ : right_;
}

const MeasurementChannel &CalibrationController::measurement(
    CalibrationSide side
) const
{
    return side == CalibrationSide::Left ? left_ : right_;
}

CalibrationController::ChannelWorkflow &CalibrationController::workflow(
    CalibrationSide side
)
{
    return side == CalibrationSide::Left
        ? leftWorkflow_
        : rightWorkflow_;
}

const CalibrationController::ChannelWorkflow &
CalibrationController::workflow(CalibrationSide side) const
{
    return side == CalibrationSide::Left
        ? leftWorkflow_
        : rightWorkflow_;
}

StoredLoadSide CalibrationController::storedSide(
    CalibrationSide side
) const
{
    return side == CalibrationSide::Left
        ? StoredLoadSide::Left
        : StoredLoadSide::Right;
}
