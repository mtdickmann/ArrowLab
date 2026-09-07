#include <cassert>
#include <cmath>

#include "spine/SpineCalculation.h"
#include "spine/SpineTestController.h"

namespace
{
    bool near(float actual, float expected, float tolerance = 0.01f)
    {
        return std::fabs(actual - expected) <= tolerance;
    }
}

int main()
{
    assert(near(SpineCalculation::equivalentSpine(880.0f), 500.0f));
    assert(near(SpineCalculation::equivalentSpine(440.0f), 1000.0f));
    assert(near(SpineCalculation::markedDifference(520.0f, 500.0f), 20.0f));
    assert(near(
        SpineCalculation::markedDifferencePercent(520.0f, 500.0f),
        4.0f));

    SpineTestController controller;
    SpineTestController::Inputs inputs;
    assert(!controller.start(1, inputs, 0));
    assert(controller.stage() == SpineTestController::Stage::Fault);

    inputs.leftLive = true;
    inputs.rightLive = true;
    inputs.leftCalibrated = true;
    inputs.rightCalibrated = true;
    assert(controller.start(1, inputs, 0));
    assert(controller.stage() == SpineTestController::Stage::AwaitingClear);
    assert(controller.takeAction() == SpineTestController::Action::None);
    assert(controller.confirmSupportsClear(1));
    assert(controller.takeAction() == SpineTestController::Action::TareLeft);

    inputs.leftTareConfirmed = true;
    controller.update(inputs, 10);
    assert(controller.takeAction() == SpineTestController::Action::TareRight);

    inputs.rightTareConfirmed = true;
    controller.update(inputs, 20);
    assert(controller.stage() == SpineTestController::Stage::AwaitingArrow);

    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 30);
    for (uint32_t time = 40; time <= 1100; time += 10) {
        inputs.combinedInstantaneousGrams =
            time % 20 == 0 ? 25.1f : 24.9f;
        controller.update(inputs, time);
    }
    assert(
        controller.stage()
        == SpineTestController::Stage::AwaitingPlungerZero);
    assert(near(controller.arrowMassGrams(), 25.0f));

    // The operator must explicitly establish the 12.7 mm travel datum at the
    // top of the arrow before a force capture can begin.
    assert(controller.confirmPlungerZero(1110));
    assert(controller.stage() == SpineTestController::Stage::ReadyToPress);

    // A partial press never starts the stable-force timer.
    inputs.combinedInstantaneousGrams = 200.0f;
    controller.update(inputs, 1120);
    assert(controller.stage() == SpineTestController::Stage::ReadyToPress);

    inputs.combinedInstantaneousGrams = 905.0f;
    controller.update(inputs, 1130);
    assert(controller.stage() == SpineTestController::Stage::Holding);

    // Backing away from the stop cannot begin a fresh hold around a lower,
    // sticky plunger force. A confirmed full release is required first.
    inputs.combinedInstantaneousGrams = 500.0f;
    controller.update(inputs, 1500);
    assert(
        controller.stage()
        == SpineTestController::Stage::AwaitingRetryRelease);
    controller.update(inputs, 5000);
    assert(
        controller.stage()
        == SpineTestController::Stage::AwaitingRetryRelease);

    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 5010);
    controller.update(inputs, 5520);
    assert(controller.stage() == SpineTestController::Stage::ReadyToPress);
    assert(controller.holdPercent(1500) == 0);

    inputs.combinedInstantaneousGrams = 905.0f;
    controller.update(inputs, 5530);
    assert(controller.stage() == SpineTestController::Stage::Holding);

    // RESTART during a plunge keeps the established arrow reference/top-zero,
    // but will not arm another capture until the force has fully released.
    controller.restartAttempt(5540);
    assert(
        controller.stage()
        == SpineTestController::Stage::AwaitingRetryRelease);
    assert(near(controller.arrowMassGrams(), 25.0f));
    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 5550);
    controller.update(inputs, 6060);
    assert(controller.stage() == SpineTestController::Stage::ReadyToPress);

    inputs.combinedInstantaneousGrams = 905.0f;
    controller.update(inputs, 6070);
    assert(controller.stage() == SpineTestController::Stage::Holding);
    for (uint32_t time = 6080; time < 9090; time += 10) {
        inputs.combinedInstantaneousGrams = 905.5f;
        controller.update(inputs, time);
    }
    assert(controller.stage() == SpineTestController::Stage::AwaitingRelease);

    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 9100);
    controller.update(inputs, 9610);
    assert(controller.stage() == SpineTestController::Stage::Complete);
    assert(near(controller.positionForceGrams(0), 880.5f, 0.1f));

    // A removed arrow is reported live as zero and the procedure returns to
    // arrow detection without requiring Cancel.
    assert(controller.start(1, inputs, 10000));
    assert(controller.confirmSupportsClear(10001));
    controller.takeAction();
    inputs.leftTareConfirmed = false;
    inputs.rightTareConfirmed = false;
    controller.update(inputs, 10010);
    inputs.leftTareConfirmed = true;
    controller.update(inputs, 10020);
    controller.takeAction();
    inputs.rightTareConfirmed = true;
    controller.update(inputs, 10030);
    inputs.combinedInstantaneousGrams = 13.0f;
    controller.update(inputs, 10040);
    for (uint32_t time = 10050; time <= 11100; time += 10) {
        controller.update(inputs, time);
    }
    assert(
        controller.stage()
        == SpineTestController::Stage::AwaitingPlungerZero);
    inputs.combinedInstantaneousGrams = 0.0f;
    controller.update(inputs, 11110);
    assert(controller.stage() == SpineTestController::Stage::AwaitingArrow);
    assert(near(controller.arrowMassGrams(), 0.0f));

    // The same acquisition rules must survive all four indexed SAS
    // positions.  Earlier captures remain intact while each release advances
    // to the next orientation.
    SpineTestController sas;
    SpineTestController::Inputs sasInputs;
    sasInputs.leftLive = true;
    sasInputs.rightLive = true;
    sasInputs.leftCalibrated = true;
    sasInputs.rightCalibrated = true;
    uint32_t sasTime = 8000;
    assert(sas.start(4, sasInputs, sasTime));
    assert(sas.confirmSupportsClear(sasTime += 1));
    assert(sas.takeAction() == SpineTestController::Action::TareLeft);

    sasInputs.leftTareConfirmed = true;
    sas.update(sasInputs, sasTime += 10);
    assert(sas.takeAction() == SpineTestController::Action::TareRight);
    sasInputs.rightTareConfirmed = true;
    sas.update(sasInputs, sasTime += 10);

    sasInputs.combinedInstantaneousGrams = 15.0f;
    sas.update(sasInputs, sasTime += 10);
    for (uint32_t elapsed = 10; elapsed <= 1050; elapsed += 10) {
        sas.update(sasInputs, sasTime += 10);
    }
    assert(sas.stage() == SpineTestController::Stage::AwaitingPlungerZero);
    assert(sas.confirmPlungerZero(sasTime += 10));

    const float sasForces[4] = {870.0f, 890.0f, 880.0f, 900.0f};
    for (uint8_t position = 0; position < 4; ++position) {
        sasInputs.combinedInstantaneousGrams = 15.0f + sasForces[position];
        sas.update(sasInputs, sasTime += 10);
        assert(sas.stage() == SpineTestController::Stage::Holding);
        for (uint32_t elapsed = 10; elapsed <= 3010; elapsed += 10) {
            sas.update(sasInputs, sasTime += 10);
        }
        assert(sas.stage() == SpineTestController::Stage::AwaitingRelease);

        sasInputs.combinedInstantaneousGrams = 15.0f;
        sas.update(sasInputs, sasTime += 10);
        sas.update(sasInputs, sasTime += 510);
        assert(sas.currentPosition() == position + 1);
        assert(near(sas.positionForceGrams(position), sasForces[position]));
        if (position < 3) {
            assert(sas.stage() == SpineTestController::Stage::ReadyToPress);
        }
    }
    assert(sas.stage() == SpineTestController::Stage::Complete);
}
