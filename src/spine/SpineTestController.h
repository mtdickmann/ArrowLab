#pragma once

#include <cstdint>

class SpineTestController
{
public:
    enum class Stage : uint8_t
    {
        Idle,
        TaringLeft,
        TaringRight,
        AwaitingArrow,
        StabilizingArrow,
        ReadyToPress,
        Holding,
        AwaitingRelease,
        Complete,
        Fault
    };

    enum class Action : uint8_t
    {
        None,
        TareLeft,
        TareRight
    };

    struct Inputs
    {
        bool leftLive = false;
        bool rightLive = false;
        bool leftCalibrated = false;
        bool rightCalibrated = false;
        bool leftTareConfirmed = false;
        bool rightTareConfirmed = false;
        float combinedInstantaneousGrams = 0.0f;
        float combinedHeldGrams = 0.0f;
    };

    bool start(uint8_t positionCount, const Inputs &inputs, uint32_t now);
    void cancel();
    void update(const Inputs &inputs, uint32_t now);
    Action takeAction();

    Stage stage() const;
    uint8_t positionCount() const;
    uint8_t currentPosition() const;
    uint8_t holdPercent(uint32_t now) const;
    float arrowMassGrams() const;
    float liveAppliedForceGrams() const;
    float positionForceGrams(uint8_t index) const;

private:
    void enter(Stage stage, uint32_t now);
    bool healthy(const Inputs &inputs) const;

    Stage stage_ = Stage::Idle;
    Action pendingAction_ = Action::None;
    uint8_t positionCount_ = 1;
    uint8_t currentPosition_ = 0;
    uint32_t stageStartedAt_ = 0;
    float arrowMassGrams_ = 0.0f;
    float liveAppliedForceGrams_ = 0.0f;
    float stableMinimumGrams_ = 0.0f;
    float stableMaximumGrams_ = 0.0f;
    double stableTotalGrams_ = 0.0;
    uint32_t stableSampleCount_ = 0;
    float positionForces_[4] = {};
};
