#include "SpineTestController.h"

#include <algorithm>

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
    pendingAction_ = Action::TareLeft;
    enter(Stage::TaringLeft, now);
    return true;
}

void SpineTestController::cancel()
{
    stage_ = Stage::Idle;
    pendingAction_ = Action::None;
}

void SpineTestController::update(const Inputs &inputs, uint32_t now)
{
    if (stage_ == Stage::Idle || stage_ == Stage::Complete) return;
    if (!healthy(inputs)) {
        enter(Stage::Fault, now);
        return;
    }

    const float total = inputs.combinedInstantaneousGrams;
    liveAppliedForceGrams_ = std::max(0.0f, total - arrowMassGrams_);

    switch (stage_) {
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
        if (inputs.combinedHeldGrams >= ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            enter(Stage::StabilizingArrow, now);
        }
        break;
    case Stage::StabilizingArrow:
        if (inputs.combinedHeldGrams < ArrowLabConfig::ARROW_PRESENT_GRAMS) {
            enter(Stage::AwaitingArrow, now);
        } else if (now - stageStartedAt_
                >= ArrowLabConfig::ARROW_STABILITY_TIME_MS) {
            arrowMassGrams_ = inputs.combinedHeldGrams;
            enter(Stage::ReadyToPress, now);
        }
        break;
    case Stage::ReadyToPress:
        if (liveAppliedForceGrams_
                >= ArrowLabConfig::SPINE_MINIMUM_APPLIED_FORCE_GRAMS) {
            enter(Stage::Holding, now);
            stableMinimumGrams_ = liveAppliedForceGrams_;
            stableMaximumGrams_ = liveAppliedForceGrams_;
            stableTotalGrams_ = liveAppliedForceGrams_;
            stableSampleCount_ = 1;
        }
        break;
    case Stage::Holding:
        stableMinimumGrams_ = std::min(
            stableMinimumGrams_, liveAppliedForceGrams_);
        stableMaximumGrams_ = std::max(
            stableMaximumGrams_, liveAppliedForceGrams_);
        if (stableMaximumGrams_ - stableMinimumGrams_
                > ArrowLabConfig::SPINE_STABILITY_BAND_GRAMS) {
            enter(Stage::Holding, now);
            stableMinimumGrams_ = liveAppliedForceGrams_;
            stableMaximumGrams_ = liveAppliedForceGrams_;
            stableTotalGrams_ = liveAppliedForceGrams_;
            stableSampleCount_ = 1;
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

bool SpineTestController::healthy(const Inputs &inputs) const
{
    return inputs.leftLive && inputs.rightLive
        && inputs.leftCalibrated && inputs.rightCalibrated;
}
