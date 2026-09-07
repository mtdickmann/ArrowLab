#pragma once

#include <Arduino.h>

#include "measurement/MeasurementChannel.h"
#include "storage/InstrumentStorage.h"

enum class CalibrationSide
{
    Left,
    Right
};

/*
 * The only owner of calibration workflow and counts-per-gram calculation.
 * MeasurementChannel supplies one zero-referenced raw stream; calibration
 * never creates another tare or adjusts an already-adjusted value again.
 */
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
        MeasurementChannel &left,
        MeasurementChannel &right,
        InstrumentStorage &storage);

    void begin();
    void update(uint32_t currentTime);
    void onFreshReading(CalibrationSide side);

    void requestTare(CalibrationSide side);
    void requestCalibration(CalibrationSide side, float referenceGrams);
    ChannelStatus status(CalibrationSide side, uint32_t currentTime) const;

private:
    struct ChannelWorkflow
    {
        Stage stage = Stage::NeedsTare;
        float referenceGrams = 0.0f;
        uint32_t settleStartedAt = 0;
        uint8_t loadConfirmSamples = 0;
        int64_t calibrationAccumulator = 0;
        uint8_t calibrationSamples = 0;
    };

    static constexpr uint32_t SETTLE_TIME_MS = 30000;
    static constexpr long LOAD_THRESHOLD_COUNTS = 250000;
    static constexpr uint8_t LOAD_CONFIRM_SAMPLES = 5;
    static constexpr uint8_t CALIBRATION_SAMPLE_COUNT = 20;

    MeasurementChannel &measurement(CalibrationSide side);
    const MeasurementChannel &measurement(CalibrationSide side) const;
    ChannelWorkflow &workflow(CalibrationSide side);
    const ChannelWorkflow &workflow(CalibrationSide side) const;
    StoredLoadSide storedSide(CalibrationSide side) const;
    void restore(CalibrationSide side);
    void processRequests(uint32_t currentTime);
    bool startTare(CalibrationSide side);
    bool performCalibrationAction(
        CalibrationSide side,
        float referenceGrams,
        uint32_t currentTime);
    void updateChannel(CalibrationSide side, uint32_t currentTime);
    void finishCalibration(CalibrationSide side);

    MeasurementChannel &left_;
    MeasurementChannel &right_;
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
