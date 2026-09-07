#include "SpineTestController.h"

#include <algorithm>
#include <cmath>

#include "ArrowLabConfig.h"

bool SpineTestController::start(
    uint8_t positionCount,
    const Inputs &inputs,
    uint32_t now)
{
    if ((positionCount != 1 && positionCount != 4) || !healthy(inputs)) {
        enter(Stage::Fault, now);
        return false;
    }

    positionCount_ = positionCount;
    currentPosition_ = 0;
    arrowMassGrams_ = 0.0f;
    liveAppliedForceGrams_ = 0.0f;
    for (float &force : positionForces_) force = 0.0f;
    pendingAction_ = Action::None;
    enter(Stage::AwaitingClear, now);
    return true;
}

void SpineTestController::cancel()
{
    stage_ = Stage::Idle;
    pendingAction_ = Action::None;
}

bool SpineTestController::confirmSupportsClear(uint32_t now)
{
    if (stage_ != Stage::AwaitingClear) return false;
    pendingAction_ = Action::TareLeft;
    enter(Stage::TaringLeft, now);
    return true;
}

bool SpineTestController::confirmPlungerZero(uint32_t now)
{
    if (stage_ != Stage::AwaitingPlungerZero) return false;
    enter(Stage::ReadyToPress, now);
    return true;
}

void SpineTestController::restartAttempt(uint32_t now)
{
    switch (stage_) {
    case Stage::AwaitingClear:
        break;
    case Stage::TaringLeft:
    case Stage::TaringRight:
        pendingAction_ = Action::None;
        enter(Stage::AwaitingClear, now);
        break;
    case Stage::AwaitingArrow:
    case Stage::StabilizingArrow:
        arrowMassGrams_ = 0.0f;
        liveAppliedForceGrams_ = 0.0f;
        enter(Stage::AwaitingArrow, now);
        break;
    case Stage::AwaitingPlungerZero:
        arrowMassGrams_ = 0.0f;
        liveAppliedForceGrams_ = 0.0f;
        enter(Stage::AwaitingArrow, now);
        break;
    case Stage::ReadyToPress:
        positionForces_[currentPosition_] = 0.0f;
        liveAppliedForceGrams_ = 0.0f;
        enter(Stage::ReadyToPress, now);
        break;
    case Stage::Holding:
    case Stage::AwaitingRelease:
    case Stage::AwaitingRetryRelease:
        positionForces_[currentPosition_] = 0.0f;
        enter(Stage::AwaitingRetryRelease, now);
        break;
    case Stage::Complete:
    case Stage::Fault:
    case Stage::Idle:
        break;
    }
}

void SpineTestController::update(const Inputs &inputs, uint32_t now)
{
    if (stage_ == Stage::Idle || stage_ == Stage::Complete) return;
    if (!healthy(inputs)) {
        enter(Stage::Fault, now);
        return;
    }

    const float total = std::max(0.0f, inputs.combinedInstantaneousGrams);
    liveAppliedForceGrams_ = std::max(0.0f, total - arrowMassGrams_);

    switch (stage_) {
    case Stage::AwaitingClear:
        break;
    case Stage::TaringLeft:
        if (inputs.leftTareConfirmed) {
            pendingAction_ = Action::TareRight;
            enter(Stage::TaringRight, now);
        }
        break;
    case Stage::TaringRight:
        if (inputs.rightTareConfirmed) enter(Stage::AwaitingArrow, now);
        break;
    case Stage::AwaitingArrow:
        arrowMassGrams_ = total;
        liveAppliedForceGrams_ = 0.0f;
        if (total >= ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            beginArrowStability(total, now);
        }
        break;
    case Stage::StabilizingArrow:
        arrowMassGrams_ = total;
        liveAppliedForceGrams_ = 0.0f;
        if (total < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            arrowMassGrams_ = 0.0f;
            enter(Stage::AwaitingArrow, now);
            break;
        }

        arrowStableMinimumGrams_ = std::min(
            arrowStableMinimumGrams_, total);
        arrowStableMaximumGrams_ = std::max(
            arrowStableMaximumGrams_, total);
        if (arrowStableMaximumGrams_ - arrowStableMinimumGrams_
                > ArrowLabConfig::ARROW_STABILITY_BAND_GRAMS) {
            beginArrowStability(total, now);
            break;
        }

        arrowStableTotalGrams_ += total;
        ++arrowStableSampleCount_;
        if (now - stageStartedAt_
                >= ArrowLabConfig::ARROW_STABILITY_TIME_MS) {
            arrowMassGrams_ = static_cast<float>(
                arrowStableTotalGrams_ / arrowStableSampleCount_);
            enter(Stage::AwaitingPlungerZero, now);
        }
        break;
    case Stage::AwaitingPlungerZero:
        if (total < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            arrowMassGrams_ = 0.0f;
            liveAppliedForceGrams_ = 0.0f;
            enter(Stage::AwaitingArrow, now);
        } else if (std::abs(total - arrowMassGrams_)
                > ArrowLabConfig::ARROW_STABILITY_BAND_GRAMS) {
            beginArrowStability(total, now);
        }
        break;
    case Stage::ReadyToPress:
        if (total < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            arrowMassGrams_ = 0.0f;
            liveAppliedForceGrams_ = 0.0f;
            enter(Stage::AwaitingArrow, now);
            break;
        }
        if (liveAppliedForceGrams_
                <= ArrowLabConfig::SPINE_BASELINE_TRACKING_BAND_GRAMS) {
            arrowMassGrams_ += (total - arrowMassGrams_) / 8.0f;
            liveAppliedForceGrams_ = 0.0f;
        }
        if (liveAppliedForceGrams_
                >= ArrowLabConfig::SPINE_MINIMUM_APPLIED_FORCE_GRAMS) {
            beginForceHold(liveAppliedForceGrams_, now);
        }
        break;
    case Stage::Holding:
        if (total < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            arrowMassGrams_ = 0.0f;
            liveAppliedForceGrams_ = 0.0f;
            enter(Stage::AwaitingArrow, now);
            break;
        }
        if (liveAppliedForceGrams_
                < ArrowLabConfig::SPINE_MINIMUM_APPLIED_FORCE_GRAMS) {
            enter(Stage::AwaitingRetryRelease, now);
            break;
        }
        holdPeakGrams_ = std::max(holdPeakGrams_, liveAppliedForceGrams_);
        if (holdPeakGrams_ - liveAppliedForceGrams_
                >= ArrowLabConfig::SPINE_HOLD_ABORT_DROP_GRAMS) {
            enter(Stage::AwaitingRetryRelease, now);
            break;
        }
        stableMinimumGrams_ = std::min(
            stableMinimumGrams_, liveAppliedForceGrams_);
        stableMaximumGrams_ = std::max(
            stableMaximumGrams_, liveAppliedForceGrams_);
        if (stableMaximumGrams_ - stableMinimumGrams_
                > ArrowLabConfig::SPINE_STABILITY_BAND_GRAMS) {
            beginForceHold(liveAppliedForceGrams_, now);
            break;
        }
        stableTotalGrams_ += liveAppliedForceGrams_;
        ++stableSampleCount_;
        if (now - stageStartedAt_ >= ArrowLabConfig::SPINE_HOLD_TIME_MS) {
            positionForces_[currentPosition_] = static_cast<float>(
                stableTotalGrams_ / stableSampleCount_);
            enter(Stage::AwaitingRelease, now);
        }
        break;
    case Stage::AwaitingRetryRelease:
        if (total < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            arrowMassGrams_ = 0.0f;
            liveAppliedForceGrams_ = 0.0f;
            enter(Stage::AwaitingArrow, now);
        } else if (liveAppliedForceGrams_
                > ArrowLabConfig::SPINE_RELEASE_FORCE_GRAMS) {
            stageStartedAt_ = now;
        } else if (now - stageStartedAt_
                >= ArrowLabConfig::SPINE_RELEASE_TIME_MS) {
            liveAppliedForceGrams_ = 0.0f;
            enter(Stage::ReadyToPress, now);
        }
        break;
    case Stage::AwaitingRelease:
        if (liveAppliedForceGrams_
                > ArrowLabConfig::SPINE_RELEASE_FORCE_GRAMS) {
            stageStartedAt_ = now;
        } else if (now - stageStartedAt_
                >= ArrowLabConfig::SPINE_RELEASE_TIME_MS) {
            ++currentPosition_;
            if (currentPosition_ >= positionCount_) {
                enter(Stage::Complete, now);
            } else {
                enter(Stage::ReadyToPress, now);
            }
        }
        break;
    case Stage::Fault:
    case Stage::Idle:
    case Stage::Complete:
        break;
    }
}

SpineTestController::Action SpineTestController::takeAction()
{
    const Action action = pendingAction_;
    pendingAction_ = Action::None;
    return action;
}

SpineTestController::Stage SpineTestController::stage() const { return stage_; }
uint8_t SpineTestController::positionCount() const { return positionCount_; }
uint8_t SpineTestController::currentPosition() const { return currentPosition_; }

uint8_t SpineTestController::holdPercent(uint32_t now) const
{
    if (stage_ != Stage::Holding) return 0;
    const uint32_t elapsed = now - stageStartedAt_;
    return static_cast<uint8_t>(std::min<uint32_t>(
        100, elapsed * 100 / ArrowLabConfig::SPINE_HOLD_TIME_MS));
}

float SpineTestController::arrowMassGrams() const { return arrowMassGrams_; }
float SpineTestController::liveAppliedForceGrams() const
{
    return liveAppliedForceGrams_;
}
float SpineTestController::positionForceGrams(uint8_t index) const
{
    return index < 4 ? positionForces_[index] : 0.0f;
}

void SpineTestController::enter(Stage stage, uint32_t now)
{
    stage_ = stage;
    stageStartedAt_ = now;
}

void SpineTestController::beginArrowStability(
    float totalGrams,
    uint32_t now)
{
    enter(Stage::StabilizingArrow, now);
    arrowStableMinimumGrams_ = totalGrams;
    arrowStableMaximumGrams_ = totalGrams;
    arrowStableTotalGrams_ = totalGrams;
    arrowStableSampleCount_ = 1;
}

void SpineTestController::beginForceHold(
    float appliedGrams,
    uint32_t now)
{
    enter(Stage::Holding, now);
    stableMinimumGrams_ = appliedGrams;
    stableMaximumGrams_ = appliedGrams;
    holdPeakGrams_ = appliedGrams;
    stableTotalGrams_ = appliedGrams;
    stableSampleCount_ = 1;
}

bool SpineTestController::healthy(const Inputs &inputs) const
{
    return inputs.leftLive && inputs.rightLive
        && inputs.leftCalibrated && inputs.rightCalibrated;
}
