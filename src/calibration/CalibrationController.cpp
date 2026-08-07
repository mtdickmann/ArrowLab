#include "CalibrationController.h"

CalibrationController::CalibrationController(
    LoadCellChannel &left,
    LoadCellChannel &right,
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

    sensor(side).restoreCalibration(
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
    LoadCellChannel &loadCell = sensor(side);
    ChannelWorkflow &state = workflow(side);

    if (state.stage == Stage::Taring && loadCell.tareComplete()) {
        state.stage = Stage::Ready;
    }

    if (
        state.stage == Stage::AwaitingLoad
        && loadCell.calibrationLoadDetected()
    ) {
        state.stage = Stage::ReadyToCalibrate;
    }

    if (
        state.stage == Stage::Settling
        && currentTime - state.settleStartedAt >= SETTLE_TIME_MS
    ) {
        if (loadCell.startCalibration(state.referenceGrams)) {
            state.stage = Stage::Sampling;
            state.calibrationWasRunning = true;
        } else {
            state.stage = Stage::ReadyToCalibrate;
        }
    }

    if (
        state.stage == Stage::Sampling
        && state.calibrationWasRunning
        && !loadCell.calibrationInProgress()
    ) {
        state.calibrationWasRunning = false;

        if (
            loadCell.calibrationRunSucceeded()
            && loadCell.calibrated()
        ) {
            state.stage = Stage::Ready;
            const bool stored = storage_.saveCalibration(
                storedSide(side),
                loadCell.calibrationFactor(),
                loadCell.calibrationReferenceGrams());
            if (stored) {
                Serial.printf(
                    "Stored %s calibration in NVS\n",
                    side == CalibrationSide::Left ? "LEFT" : "RIGHT");
            } else {
                Serial.printf(
                    "WARNING: %s calibration is valid for this session "
                    "but was not stored\n",
                    side == CalibrationSide::Left ? "LEFT" : "RIGHT");
            }
        } else {
            state.stage = Stage::ReadyToCalibrate;
            Serial.printf(
                "WARNING: %s calibration failed\n",
                side == CalibrationSide::Left ? "LEFT" : "RIGHT");
        }
    }
}

void CalibrationController::onFreshReading(CalibrationSide side)
{
    if (workflow(side).stage == Stage::AwaitingLoad) {
        sensor(side).updateCalibrationLoadDetection(
            LOAD_THRESHOLD_COUNTS);
    }
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

    sensor(side).startUserTare();
    state.stage = Stage::Taring;
    state.referenceGrams = sensor(side).calibrationReferenceGrams();
    state.settleStartedAt = 0;
    state.calibrationWasRunning = false;
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
    LoadCellChannel &loadCell = sensor(side);
    ChannelWorkflow &state = workflow(side);
    const ChannelWorkflow &other = workflow(
        side == CalibrationSide::Left
            ? CalibrationSide::Right
            : CalibrationSide::Left);

    if (!loadCell.tareComplete() || !loadCell.userTareConfirmed()) {
        return false;
    }

    if (referenceGrams > 0.0f && state.stage == Stage::Ready) {
        state.referenceGrams = referenceGrams;
        loadCell.resetCalibrationLoadDetection();
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

void CalibrationController::resetChannel(CalibrationSide side)
{
    sensor(side).clearCalibration();
    sensor(side).resetCalibrationLoadDetection();
    storage_.resetChannel(storedSide(side));
    workflow(side) = ChannelWorkflow{};
    if (side == CalibrationSide::Left) {
        leftTareRequested_ = false;
        leftCalibrationRequested_ = false;
    } else {
        rightTareRequested_ = false;
        rightCalibrationRequested_ = false;
    }
}

CalibrationController::ChannelStatus CalibrationController::status(
    CalibrationSide side,
    uint32_t currentTime
) const
{
    const ChannelWorkflow &state = workflow(side);
    const LoadCellChannel &loadCell = sensor(side);
    ChannelStatus result;
    result.stage = state.stage;
    result.tareComplete =
        loadCell.tareComplete() && loadCell.userTareConfirmed();
    result.calibrated = loadCell.calibrated();
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

LoadCellChannel &CalibrationController::sensor(CalibrationSide side)
{
    return side == CalibrationSide::Left ? left_ : right_;
}

const LoadCellChannel &CalibrationController::sensor(
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
