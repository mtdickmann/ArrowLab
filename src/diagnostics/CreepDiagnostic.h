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
    enum class State
    {
        Idle,
        Taring,
        AwaitingLoad,
        Running,
        Complete
    };

    static constexpr uint32_t SAMPLE_INTERVAL_MS = 30000;
    static constexpr uint32_t RUN_DURATION_MS = 30UL * 60UL * 1000UL;
    static constexpr long LOAD_DETECT_THRESHOLD_COUNTS = 2000;
    static constexpr uint8_t LOAD_CONFIRM_SAMPLES = 5;

    bool start(
        DiagnosticSide side,
        float testMassGrams,
        bool zeroBaseline,
        LoadCellChannel &left,
        LoadCellChannel &right
    );

    void cancel();
    bool finishSession();
    bool zeroBaselineComplete(DiagnosticSide side) const;

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
    uint32_t elapsedMs(uint32_t currentTime) const;
    uint8_t progressPercent(uint32_t currentTime) const;

private:
    const LoadCellChannel &selectedSensor(
        const LoadCellChannel &left,
        const LoadCellChannel &right
    ) const;

    void beginRunning(
        uint32_t currentTime,
        const LoadCellChannel &sensor
    );

    void emitSample(
        uint32_t elapsedMs,
        const LoadCellChannel &sensor
    ) const;

    State state_ = State::Idle;
    DiagnosticSide side_ = DiagnosticSide::Left;
    float testMassGrams_ = 0.0f;
    bool zeroBaseline_ = false;
    uint8_t loadConfirmSamples_ = 0;
    uint32_t runStartTime_ = 0;
    uint32_t nextSampleTime_ = 0;
    uint32_t completedElapsedMs_ = 0;
    bool leftZeroBaselineComplete_ = false;
    bool rightZeroBaselineComplete_ = false;
};
