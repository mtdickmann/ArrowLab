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
    assert(controller.takeAction() == SpineTestController::Action::TareLeft);

    inputs.leftTareConfirmed = true;
    controller.update(inputs, 10);
    assert(controller.takeAction() == SpineTestController::Action::TareRight);

    inputs.rightTareConfirmed = true;
    controller.update(inputs, 20);
    assert(controller.stage() == SpineTestController::Stage::AwaitingArrow);

    inputs.combinedHeldGrams = 25.0f;
    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 30);
    controller.update(inputs, 1100);
    assert(controller.stage() == SpineTestController::Stage::ReadyToPress);
    assert(near(controller.arrowMassGrams(), 25.0f));

    inputs.combinedInstantaneousGrams = 905.0f;
    controller.update(inputs, 1110);
    assert(controller.stage() == SpineTestController::Stage::Holding);
    for (uint32_t time = 1120; time < 4130; time += 10) {
        inputs.combinedInstantaneousGrams = 905.5f;
        controller.update(inputs, time);
    }
    assert(controller.stage() == SpineTestController::Stage::AwaitingRelease);

    inputs.combinedInstantaneousGrams = 25.0f;
    controller.update(inputs, 4140);
    controller.update(inputs, 4650);
    assert(controller.stage() == SpineTestController::Stage::Complete);
    assert(near(controller.positionForceGrams(0), 880.5f, 0.1f));
}
