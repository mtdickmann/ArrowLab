#include "CreepDiagnostic.h"

#include <climits>

namespace
{
    bool timeReached(uint32_t now, uint32_t target)
    {
        return static_cast<int32_t>(now - target) >= 0;
    }

    long magnitude(long value)
    {
        if (value == LONG_MIN) {
            return LONG_MAX;
        }

        return value < 0 ? -value : value;
    }
}

bool CreepDiagnostic::start(
    DiagnosticSide side,
    float testMassGrams,
    bool zeroBaseline,
    LoadCellChannel &left,
    LoadCellChannel &right
)
{
    if (
        state_ == State::Taring
        || state_ == State::AwaitingLoad
        || state_ == State::Running
    ) {
        return false;
    }

    if (!zeroBaseline && testMassGrams <= 0.0f) {
        return false;
    }

    if (
        !zeroBaseline
        && !zeroBaselineComplete(side)
    ) {
        Serial.printf(
            "AL_DIAG,EVENT,%s,%.3f,ZERO_BASELINE_REQUIRED\n",
            side == DiagnosticSide::Left ? "LEFT" : "RIGHT",
            testMassGrams
        );
        return false;
    }

    side_ = side;
    testMassGrams_ = zeroBaseline ? 0.0f : testMassGrams;
    zeroBaseline_ = zeroBaseline;
    loadConfirmSamples_ = 0;
    runStartTime_ = 0;
    nextSampleTime_ = 0;
    completedElapsedMs_ = 0;
    state_ = State::Taring;

    LoadCellChannel &sensor =
        side_ == DiagnosticSide::Left
            ? left
            : right;

    sensor.startUserTare();

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,TARE_STARTED\n",
        side_ == DiagnosticSide::Left ? "LEFT" : "RIGHT",
        testMassGrams_
    );

    return true;
}

void CreepDiagnostic::cancel()
{
    if (state_ == State::Idle) {
        return;
    }

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,CANCELLED\n",
        side_ == DiagnosticSide::Left ? "LEFT" : "RIGHT",
        testMassGrams_
    );

    state_ = State::Idle;
    loadConfirmSamples_ = 0;
    runStartTime_ = 0;
    nextSampleTime_ = 0;
    completedElapsedMs_ = 0;
}

bool CreepDiagnostic::finishSession()
{
    if (
        state_ == State::Taring
        || state_ == State::AwaitingLoad
        || state_ == State::Running
    ) {
        return false;
    }

    Serial.println(
        "AL_DIAG,EVENT,SESSION_COMPLETE"
    );

    state_ = State::Idle;
    leftZeroBaselineComplete_ = false;
    rightZeroBaselineComplete_ = false;
    testMassGrams_ = 0.0f;
    zeroBaseline_ = false;
    completedElapsedMs_ = 0;

    return true;
}

bool CreepDiagnostic::zeroBaselineComplete(
    DiagnosticSide side
) const
{
    return side == DiagnosticSide::Left
        ? leftZeroBaselineComplete_
        : rightZeroBaselineComplete_;
}

void CreepDiagnostic::update(
    uint32_t currentTime,
    bool leftFresh,
    bool rightFresh,
    const LoadCellChannel &left,
    const LoadCellChannel &right
)
{
    if (
        state_ == State::Idle
        || state_ == State::Complete
    ) {
        return;
    }

    const bool selectedFresh =
        side_ == DiagnosticSide::Left
            ? leftFresh
            : rightFresh;

    if (!selectedFresh) {
        return;
    }

    const LoadCellChannel &sensor =
        selectedSensor(left, right);

    if (state_ == State::Taring) {
        if (!sensor.tareComplete()) {
            return;
        }

        if (zeroBaseline_) {
            beginRunning(currentTime, sensor);
        } else {
            state_ = State::AwaitingLoad;
            loadConfirmSamples_ = 0;

            Serial.printf(
                "AL_DIAG,EVENT,%s,%.3f,PLACE_WEIGHT\n",
                side_ == DiagnosticSide::Left
                    ? "LEFT"
                    : "RIGHT",
                testMassGrams_
            );
        }

        return;
    }

    if (state_ == State::AwaitingLoad) {
        if (
            magnitude(sensor.zeroedValue())
            < LOAD_DETECT_THRESHOLD_COUNTS
        ) {
            loadConfirmSamples_ = 0;
            return;
        }

        if (loadConfirmSamples_ < LOAD_CONFIRM_SAMPLES) {
            loadConfirmSamples_++;
        }

        if (loadConfirmSamples_ >= LOAD_CONFIRM_SAMPLES) {
            beginRunning(currentTime, sensor);
        }

        return;
    }

    if (state_ != State::Running) {
        return;
    }

    const uint32_t elapsed =
        currentTime - runStartTime_;

    if (timeReached(currentTime, nextSampleTime_)) {
        emitSample(elapsed, sensor);
        nextSampleTime_ += SAMPLE_INTERVAL_MS;
    }

    if (elapsed >= RUN_DURATION_MS) {
        completedElapsedMs_ = elapsed;
        state_ = State::Complete;

        if (zeroBaseline_) {
            if (side_ == DiagnosticSide::Left) {
                leftZeroBaselineComplete_ = true;
            } else {
                rightZeroBaselineComplete_ = true;
            }
        }

        Serial.printf(
            "AL_DIAG,EVENT,%s,%.3f,COMPLETE\n",
            side_ == DiagnosticSide::Left
                ? "LEFT"
                : "RIGHT",
            testMassGrams_
        );
    }
}

CreepDiagnostic::State CreepDiagnostic::state() const
{
    return state_;
}

DiagnosticSide CreepDiagnostic::side() const
{
    return side_;
}

float CreepDiagnostic::testMassGrams() const
{
    return testMassGrams_;
}

bool CreepDiagnostic::zeroBaseline() const
{
    return zeroBaseline_;
}

uint32_t CreepDiagnostic::elapsedMs(
    uint32_t currentTime
) const
{
    if (state_ == State::Running) {
        const uint32_t elapsed =
            currentTime - runStartTime_;

        return elapsed > RUN_DURATION_MS
            ? RUN_DURATION_MS
            : elapsed;
    }

    if (state_ == State::Complete) {
        return completedElapsedMs_ > RUN_DURATION_MS
            ? RUN_DURATION_MS
            : completedElapsedMs_;
    }

    return 0;
}

uint8_t CreepDiagnostic::progressPercent(
    uint32_t currentTime
) const
{
    const uint32_t elapsed = elapsedMs(currentTime);

    if (
        state_ != State::Running
        && state_ != State::Complete
    ) {
        return 0;
    }

    if (elapsed >= RUN_DURATION_MS) {
        return 100;
    }

    return static_cast<uint8_t>(
        (static_cast<uint64_t>(elapsed) * 100)
        / RUN_DURATION_MS
    );
}

const LoadCellChannel &CreepDiagnostic::selectedSensor(
    const LoadCellChannel &left,
    const LoadCellChannel &right
) const
{
    return side_ == DiagnosticSide::Left
        ? left
        : right;
}

void CreepDiagnostic::beginRunning(
    uint32_t currentTime,
    const LoadCellChannel &sensor
)
{
    state_ = State::Running;
    runStartTime_ = currentTime;
    nextSampleTime_ =
        currentTime + SAMPLE_INTERVAL_MS;
    completedElapsedMs_ = 0;

    Serial.println(
        "AL_DIAG,HEADER,side,test_mass_g,elapsed_ms,"
        "raw_count,zeroed_count,calculated_g,"
        "calibration_factor"
    );

    emitSample(0, sensor);

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,RUNNING\n",
        side_ == DiagnosticSide::Left
            ? "LEFT"
            : "RIGHT",
        testMassGrams_
    );
}

void CreepDiagnostic::emitSample(
    uint32_t elapsed,
    const LoadCellChannel &sensor
) const
{
    Serial.printf(
        "AL_DIAG,DATA,%s,%.3f,%lu,%ld,%ld,%.4f,%.6f\n",
        side_ == DiagnosticSide::Left
            ? "LEFT"
            : "RIGHT",
        testMassGrams_,
        static_cast<unsigned long>(elapsed),
        sensor.rawValue(),
        sensor.zeroedValue(),
        sensor.grams(),
        sensor.calibrationFactor()
    );
}
