#include "CreepDiagnostic.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <esp_system.h>

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

CreepDiagnostic::CreepDiagnostic()
    : bootId_(esp_random())
{
    if (bootId_ == 0) {
        bootId_ = 1;
    }
}

bool CreepDiagnostic::start(
    DiagnosticSide side,
    float testMassGrams,
    bool zeroBaseline,
    LoadCellChannel &left,
    LoadCellChannel &right,
    uint32_t currentTime
)
{
    if (
        state_ == State::WaitingForHost
        || state_ == State::Taring
        || state_ == State::AwaitingLoad
        || state_ == State::Running
        || state_ == State::AwaitingSave
    ) {
        return false;
    }

    side_ = side;
    LoadCellChannel &sensor = selectedSensor(left, right);

    if (
        !zeroBaseline
        && (
            testMassGrams <= 0.0f
            || testMassGrams > MAX_TEST_MASS_GRAMS
        )
    ) {
        Serial.printf(
            "AL_DIAG,EVENT,%s,%.3f,MASS_REJECTED\n",
            sideText(),
            testMassGrams
        );
        return false;
    }

    if (!zeroBaseline && !baselineCaptured(side_)) {
        Serial.printf(
            "AL_DIAG,EVENT,%s,%.3f,BASELINE_REQUIRED\n",
            sideText(),
            testMassGrams
        );
        return false;
    }

    if (!zeroBaseline && !sensor.calibrated()) {
        Serial.printf(
            "AL_DIAG,EVENT,%s,%.3f,CALIBRATION_REQUIRED\n",
            sideText(),
            testMassGrams
        );
        return false;
    }

    testMassGrams_ = zeroBaseline ? 0.0f : testMassGrams;
    zeroBaseline_ = zeroBaseline;
    loadConfirmSamples_ = 0;
    runStartTime_ = 0;
    completedElapsedMs_ = 0;
    sampleCount_ = 0;
    pendingSensor_ = &sensor;
    runId_++;
    state_ = State::WaitingForHost;

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,WAITING_FOR_HOST\n",
        sideText(),
        testMassGrams_
    );

    if (hostConnected(currentTime)) {
        beginTare();
    }

    return true;
}

void CreepDiagnostic::cancel()
{
    if (state_ == State::Idle) {
        return;
    }

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,CANCELLED\n",
        sideText(),
        testMassGrams_
    );

    state_ = State::Idle;
    loadConfirmSamples_ = 0;
    runStartTime_ = 0;
    completedElapsedMs_ = 0;
    pendingSensor_ = nullptr;
    sampleCount_ = 0;
}

bool CreepDiagnostic::finishSession()
{
    if (
        state_ == State::WaitingForHost
        || state_ == State::Taring
        || state_ == State::AwaitingLoad
        || state_ == State::Running
        || state_ == State::AwaitingSave
    ) {
        return false;
    }

    Serial.println("AL_DIAG,EVENT,SESSION_COMPLETE");
    state_ = State::Idle;
    testMassGrams_ = 0.0f;
    zeroBaseline_ = false;
    completedElapsedMs_ = 0;
    pendingSensor_ = nullptr;
    sampleCount_ = 0;
    return true;
}

void CreepDiagnostic::handleHostCommand(
    const char *line,
    uint32_t currentTime
)
{
    if (line == nullptr) {
        return;
    }

    if (
        strcmp(line, "AL_HOST,HELLO,2") == 0
        || strcmp(line, "AL_HOST,HEARTBEAT,2") == 0
    ) {
        hostSeen_ = true;
        lastHostMessageTime_ = currentTime;

        if (strcmp(line, "AL_HOST,HELLO,2") == 0) {
            Serial.println("AL_DIAG,EVENT,HOST_READY,2");

            if (state_ == State::AwaitingSave) {
                replayBufferedRun();
            }
        }

        if (state_ == State::WaitingForHost) {
            beginTare();
        }

        return;
    }

    if (strncmp(line, "AL_HOST,REPLAY,", 15) == 0) {
        char *end = nullptr;
        const uint32_t requestedBoot = static_cast<uint32_t>(
            strtoul(line + 15, &end, 10)
        );

        if (end == nullptr || *end != ',') {
            return;
        }

        const uint32_t requestedRun = static_cast<uint32_t>(
            strtoul(end + 1, nullptr, 10)
        );

        if (
            requestedBoot == bootId_
            &&
            requestedRun == runId_
            && (
                state_ == State::AwaitingSave
                || state_ == State::Complete
            )
        ) {
            replayBufferedRun();
        }

        return;
    }

    if (strncmp(line, "AL_HOST,ACK,", 12) != 0) {
        return;
    }

    char *end = nullptr;
    const uint32_t acknowledgedBoot = static_cast<uint32_t>(
        strtoul(line + 12, &end, 10)
    );

    if (end == nullptr || *end != ',') {
        return;
    }

    char *runEnd = nullptr;
    const uint32_t acknowledgedRun = static_cast<uint32_t>(
        strtoul(end + 1, &runEnd, 10)
    );

    if (runEnd == nullptr || *runEnd != ',') {
        return;
    }

    const uint32_t acknowledgedSamples = static_cast<uint32_t>(
        strtoul(runEnd + 1, nullptr, 10)
    );

    if (
        state_ != State::AwaitingSave
        || acknowledgedBoot != bootId_
        || acknowledgedRun != runId_
        || acknowledgedSamples != sampleCount_
    ) {
        return;
    }

    if (zeroBaseline_) {
        setBaselineCaptured(side_, true);
    }

    state_ = State::Complete;

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,RUN_SAVED,%lu,%lu,%u\n",
        sideText(),
        testMassGrams_,
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_),
        sampleCount_
    );
}

void CreepDiagnostic::setBaselineCaptured(
    DiagnosticSide side,
    bool captured
)
{
    if (side == DiagnosticSide::Left) {
        leftBaselineCaptured_ = captured;
    } else {
        rightBaselineCaptured_ = captured;
    }
}

bool CreepDiagnostic::baselineCaptured(DiagnosticSide side) const
{
    return side == DiagnosticSide::Left
        ? leftBaselineCaptured_
        : rightBaselineCaptured_;
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
        || state_ == State::AwaitingSave
        || state_ == State::WaitingForHost
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

    const LoadCellChannel &sensor = selectedSensor(left, right);

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
                sideText(),
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

    const uint32_t elapsed = currentTime - runStartTime_;

    if (
        sampleCount_ < SAMPLE_CAPACITY
        && timeReached(elapsed, nextTargetElapsedMs())
    ) {
        captureSample(elapsed, sensor);
    }

    if (sampleCount_ < SAMPLE_CAPACITY) {
        return;
    }

    completedElapsedMs_ = elapsed;
    state_ = State::AwaitingSave;

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,RUN_BUFFERED,%lu,%lu,%u\n",
        sideText(),
        testMassGrams_,
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_),
        sampleCount_
    );

    if (hostConnected(currentTime)) {
        replayBufferedRun();
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

bool CreepDiagnostic::hostConnected(uint32_t currentTime) const
{
    return hostSeen_
        && currentTime - lastHostMessageTime_ <= HOST_TIMEOUT_MS;
}

bool CreepDiagnostic::awaitingSave() const
{
    return state_ == State::AwaitingSave;
}

uint8_t CreepDiagnostic::bufferedSampleCount() const
{
    return sampleCount_;
}

uint32_t CreepDiagnostic::elapsedMs(uint32_t currentTime) const
{
    if (state_ == State::Running) {
        const uint32_t elapsed = currentTime - runStartTime_;
        return elapsed > RUN_DURATION_MS ? RUN_DURATION_MS : elapsed;
    }

    if (
        state_ == State::AwaitingSave
        || state_ == State::Complete
    ) {
        return completedElapsedMs_ > RUN_DURATION_MS
            ? RUN_DURATION_MS
            : completedElapsedMs_;
    }

    return 0;
}

uint8_t CreepDiagnostic::progressPercent(uint32_t currentTime) const
{
    const uint32_t elapsed = elapsedMs(currentTime);

    if (
        state_ != State::Running
        && state_ != State::AwaitingSave
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

LoadCellChannel &CreepDiagnostic::selectedSensor(
    LoadCellChannel &left,
    LoadCellChannel &right
) const
{
    return side_ == DiagnosticSide::Left ? left : right;
}

const LoadCellChannel &CreepDiagnostic::selectedSensor(
    const LoadCellChannel &left,
    const LoadCellChannel &right
) const
{
    return side_ == DiagnosticSide::Left ? left : right;
}

void CreepDiagnostic::beginTare()
{
    if (pendingSensor_ == nullptr) {
        return;
    }

    state_ = State::Taring;
    pendingSensor_->startDiagnosticTare();

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,TARE_STARTED\n",
        sideText(),
        testMassGrams_
    );
}

void CreepDiagnostic::beginRunning(
    uint32_t currentTime,
    const LoadCellChannel &sensor
)
{
    state_ = State::Running;
    runStartTime_ = currentTime;
    completedElapsedMs_ = 0;
    sampleCount_ = 0;

    emitHeader();
    captureSample(0, sensor);

    Serial.printf(
        "AL_DIAG,EVENT,%s,%.3f,RUNNING,%lu,%lu\n",
        sideText(),
        testMassGrams_,
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_)
    );
}

void CreepDiagnostic::captureSample(
    uint32_t elapsed,
    const LoadCellChannel &sensor
)
{
    if (sampleCount_ >= SAMPLE_CAPACITY) {
        return;
    }

    Sample &sample = samples_[sampleCount_];
    sample.elapsedMs = elapsed;
    sample.rawCount = sensor.rawValue();
    sample.zeroedCount = sensor.zeroedValue();
    sample.calculatedGrams = sensor.grams();
    sample.calibrationFactor = sensor.calibrationFactor();

    if (hostConnected(millis())) {
        emitSample(sampleCount_, sample);
    }

    sampleCount_++;
}

void CreepDiagnostic::emitHeader() const
{
    Serial.println(
        "AL_DIAG,HEADER,boot_id,run_id,sample_index,side,run_type,"
        "test_mass_g,elapsed_ms,raw_count,zeroed_count,"
        "calculated_g,calibration_factor"
    );
}

void CreepDiagnostic::emitSample(
    uint8_t sampleIndex,
    const Sample &sample
) const
{
    Serial.printf(
        "AL_DIAG,DATA,%lu,%lu,%u,%s,%s,%.3f,%lu,%ld,%ld,%.4f,%.6f\n",
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_),
        sampleIndex,
        sideText(),
        runTypeText(),
        testMassGrams_,
        static_cast<unsigned long>(sample.elapsedMs),
        sample.rawCount,
        sample.zeroedCount,
        sample.calculatedGrams,
        sample.calibrationFactor
    );
}

void CreepDiagnostic::replayBufferedRun() const
{
    Serial.printf(
        "AL_DIAG,EVENT,REPLAY_BEGIN,%lu,%lu,%u\n",
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_),
        sampleCount_
    );

    emitHeader();

    for (uint8_t index = 0; index < sampleCount_; index++) {
        emitSample(index, samples_[index]);
    }

    Serial.printf(
        "AL_DIAG,EVENT,REPLAY_END,%lu,%lu,%u\n",
        static_cast<unsigned long>(bootId_),
        static_cast<unsigned long>(runId_),
        sampleCount_
    );
}

uint32_t CreepDiagnostic::nextTargetElapsedMs() const
{
    if (sampleCount_ == 0) {
        return 0;
    }

    if (sampleCount_ == 1) {
        return INITIAL_REFERENCE_SAMPLE_MS;
    }

    return static_cast<uint32_t>(sampleCount_ - 1)
        * SAMPLE_INTERVAL_MS;
}

const char *CreepDiagnostic::sideText() const
{
    return side_ == DiagnosticSide::Left ? "LEFT" : "RIGHT";
}

const char *CreepDiagnostic::runTypeText() const
{
    return zeroBaseline_ ? "ZERO" : "LOAD";
}
