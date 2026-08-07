#pragma once

#include <Arduino.h>

#include "measurement/LoadCellChannel.h"
#include "storage/InstrumentStorage.h"

enum class CalibrationSide
{
    Left,
    Right
};

class CalibrationController
{
public:
    enum class Stage
    {
        NeedsTare,
        Taring,
        Ready,
        AwaitingLoad,
        ReadyToCalibrate,
        Settling,
        Sampling
    };

    struct ChannelStatus
    {
        Stage stage = Stage::NeedsTare;
        bool tareComplete = false;
        bool calibrated = false;
        float referenceGrams = 0.0f;
        uint32_t settleRemainingSeconds = 0;
        uint8_t settlePercent = 0;
    };

    CalibrationController(
        LoadCellChannel &left,
        LoadCellChannel &right,
        InstrumentStorage &storage
    );

    void begin();
    void update(uint32_t currentTime);
    void onFreshReading(CalibrationSide side);

    void requestTare(CalibrationSide side);
    void requestCalibration(
        CalibrationSide side,
        float referenceGrams
    );
    void resetChannel(CalibrationSide side);

    ChannelStatus status(
        CalibrationSide side,
        uint32_t currentTime
    ) const;

private:
    struct ChannelWorkflow
    {
        Stage stage = Stage::NeedsTare;
        float referenceGrams = 0.0f;
        uint32_t settleStartedAt = 0;
        bool calibrationWasRunning = false;
    };

    static constexpr uint32_t SETTLE_TIME_MS = 30000;
    static constexpr long LOAD_THRESHOLD_COUNTS = 250000;

    LoadCellChannel &sensor(CalibrationSide side);
    const LoadCellChannel &sensor(CalibrationSide side) const;
    ChannelWorkflow &workflow(CalibrationSide side);
    const ChannelWorkflow &workflow(CalibrationSide side) const;
    StoredLoadSide storedSide(CalibrationSide side) const;
    void restore(CalibrationSide side);
    void processRequests(uint32_t currentTime);
    bool startTare(CalibrationSide side);
    bool performCalibrationAction(
        CalibrationSide side,
        float referenceGrams,
        uint32_t currentTime
    );
    void updateChannel(CalibrationSide side, uint32_t currentTime);

    LoadCellChannel &left_;
    LoadCellChannel &right_;
    InstrumentStorage &storage_;
    ChannelWorkflow leftWorkflow_;
    ChannelWorkflow rightWorkflow_;
    volatile bool leftTareRequested_ = false;
    volatile bool rightTareRequested_ = false;
    volatile bool leftCalibrationRequested_ = false;
    volatile bool rightCalibrationRequested_ = false;
    float leftRequestedReferenceGrams_ = 0.0f;
    float rightRequestedReferenceGrams_ = 0.0f;
};
