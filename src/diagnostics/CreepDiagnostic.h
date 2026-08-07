#pragma once

#include <Arduino.h>

#include "measurement/LoadCellChannel.h"

enum class DiagnosticSide
{
    Left,
    Right
};

class CreepDiagnostic
{
public:
    CreepDiagnostic();

    enum class State
    {
        Idle,
        WaitingForHost,
        CapturingReference,
        AwaitingLoad,
        Running,
        AwaitingSave,
        Complete
    };

    static constexpr uint32_t INITIAL_REFERENCE_SAMPLE_MS = 10000;
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 30000;
    static constexpr uint32_t RUN_DURATION_MS = 30UL * 60UL * 1000UL;
    static constexpr uint32_t HOST_TIMEOUT_MS = 5000;
    static constexpr float MAX_TEST_MASS_GRAMS = 1850.0f;
    static constexpr long LOAD_DETECT_THRESHOLD_COUNTS = 2000;
    static constexpr uint8_t LOAD_CONFIRM_SAMPLES = 5;
    static constexpr uint8_t SAMPLE_CAPACITY = 62;
    static constexpr uint8_t REFERENCE_SAMPLE_COUNT = 20;

    bool start(
        DiagnosticSide side,
        float testMassGrams,
        bool zeroBaseline,
        uint32_t currentTime
    );

    void cancel();
    bool finishSession();
    void handleHostCommand(const char *line, uint32_t currentTime);

    void update(
        uint32_t currentTime,
        bool leftFresh,
        bool rightFresh,
        const LoadCellChannel &left,
        const LoadCellChannel &right
    );

    State state() const;
    DiagnosticSide side() const;
    float testMassGrams() const;
    bool zeroBaseline() const;
    bool hostConnected(uint32_t currentTime) const;
    bool awaitingSave() const;
    uint8_t bufferedSampleCount() const;
    uint32_t elapsedMs(uint32_t currentTime) const;
    uint8_t progressPercent(uint32_t currentTime) const;

private:
    struct Sample
    {
        uint32_t elapsedMs = 0;
        long rawCount = 0;
        long deltaCount = 0;
    };

    const LoadCellChannel &selectedSensor(
        const LoadCellChannel &left,
        const LoadCellChannel &right
    ) const;

    void beginReferenceCapture();
    void beginRunning(uint32_t currentTime, const LoadCellChannel &sensor);
    void captureSample(uint32_t elapsedMs, const LoadCellChannel &sensor);
    void emitHeader() const;
    void emitSample(uint8_t sampleIndex, const Sample &sample) const;
    void replayBufferedRun() const;
    uint32_t nextTargetElapsedMs() const;
    const char *sideText() const;
    const char *runTypeText() const;

    State state_ = State::Idle;
    DiagnosticSide side_ = DiagnosticSide::Left;
    float testMassGrams_ = 0.0f;
    bool zeroBaseline_ = false;
    uint8_t loadConfirmSamples_ = 0;
    uint32_t runStartTime_ = 0;
    uint32_t completedElapsedMs_ = 0;
    uint32_t runId_ = 0;
    uint32_t bootId_ = 0;
    bool hostSeen_ = false;
    uint32_t lastHostMessageTime_ = 0;
    int64_t referenceAccumulator_ = 0;
    uint8_t referenceSampleCount_ = 0;
    long runReferenceRaw_ = 0;

    Sample samples_[SAMPLE_CAPACITY];
    uint8_t sampleCount_ = 0;
};
