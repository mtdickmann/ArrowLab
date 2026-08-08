#include "calibration/CalibrationController.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace
{
    void feed(
        MeasurementChannel &channel,
        CalibrationController &controller,
        CalibrationSide side,
        long raw,
        uint32_t &time,
        int samples
    )
    {
        for (int index = 0; index < samples; ++index) {
            channel.onRawSample(raw, time);
            controller.onFreshReading(side);
            controller.update(time);
            time += 100;
        }
    }
}

int main()
{
    InstrumentStorage storage;
    MeasurementChannel left;
    MeasurementChannel right;
    CalibrationController controller(left, right, storage);
    uint32_t time = 0;

    controller.begin();
    controller.requestTare(CalibrationSide::Left);
    controller.update(time);
    feed(left, controller, CalibrationSide::Left, 100000, time, 20);
    assert(left.userTareConfirmed());

    controller.requestCalibration(CalibrationSide::Left, 1000.0f);
    controller.update(time);
    feed(left, controller, CalibrationSide::Left, 1100000, time, 30);
    assert(
        controller.status(CalibrationSide::Left, time).stage
        == CalibrationController::Stage::ReadyToCalibrate);

    controller.requestCalibration(CalibrationSide::Left, 0.0f);
    controller.update(time);
    time += 30000;
    controller.update(time);
    feed(left, controller, CalibrationSide::Left, 1100000, time, 20);

    assert(left.calibrated());
    assert(std::fabs(left.countsPerGram() - 1000.0f) < 1.0f);
    assert(std::fabs(left.heldGrams() - 1000.0f) < 0.001f);

    StoredCalibration saved;
    assert(storage.loadCalibration(StoredLoadSide::Left, saved));
    assert(std::fabs(saved.factor - left.countsPerGram()) < 0.001f);

    // A power-cycle equivalent restores K but deliberately not the physical
    // zero reference.  A new deliberate tare makes the channel usable again.
    MeasurementChannel restoredLeft;
    MeasurementChannel restoredRight;
    CalibrationController restoredController(
        restoredLeft,
        restoredRight,
        storage);
    restoredController.begin();
    assert(restoredLeft.calibrated());
    assert(!restoredLeft.tareComplete());
    restoredController.requestTare(CalibrationSide::Left);
    restoredController.update(time);
    feed(
        restoredLeft,
        restoredController,
        CalibrationSide::Left,
        100000,
        time,
        20);
    assert(restoredLeft.tareComplete());
    assert(restoredLeft.calibrated());

    std::cout << "CalibrationController tests passed\n";
    return 0;
}
