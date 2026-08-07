#pragma once

#include <Arduino.h>
#include <HX711.h>

class LoadCellChannel
{
public:
    LoadCellChannel(
        const char *name,
        uint8_t dtPin,
        uint8_t sckPin
    );

    void begin();
    bool read(uint32_t currentTime);
    bool isLive(
        uint32_t currentTime,
        uint32_t timeoutMs
    ) const;

    void startUserTare();
    void startDiagnosticTare();
    void updateCalibrationLoadDetection(
        long thresholdCounts
    );
    void resetCalibrationLoadDetection();
    bool startCalibration(float referenceGrams);

    const char *name() const;
    long rawValue() const;
    long zeroedValue() const;
    long filteredZeroedValue() const;
    bool tareComplete() const;
    bool userTareConfirmed() const;
    bool calibrationLoadDetected() const;
    bool calibrationInProgress() const;
    bool calibrationRunSucceeded() const;
    bool calibrated() const;
    float calibrationFactor() const;
    float calibrationReferenceGrams() const;
    float grams() const;
    float rawGrams() const;
    void restoreCalibration(float factor, float referenceGrams);
    void clearCalibration();

private:
    static constexpr uint8_t TARE_SAMPLE_COUNT = 20;
    static constexpr uint8_t CALIBRATION_SAMPLE_COUNT = 20;
    static constexpr uint8_t CALIBRATION_LOAD_CONFIRM_SAMPLES = 5;
    static constexpr uint8_t FILTER_SAMPLE_COUNT = 15;

    void startTare(bool confirmAsUserTare);
    void updateTare();
    void updateMeasurementFilter();
    void updateCalibration();

    HX711 hx711_;
    const char *name_;
    uint8_t dtPin_;
    uint8_t sckPin_;

    long rawValue_ = 0;
    long tareOffset_ = 0;
    long zeroedValue_ = 0;
    long filteredZeroedValue_ = 0;
    long filterSamples_[FILTER_SAMPLE_COUNT] = {};
    uint8_t filterSampleCount_ = 0;
    uint8_t filterWriteIndex_ = 0;

    int64_t tareAccumulator_ = 0;
    uint8_t tareSamples_ = 0;
    bool tareComplete_ = false;
    bool userTareConfirmed_ = false;
    bool confirmUserTareOnCompletion_ = false;

    uint8_t calibrationLoadDetectSamples_ = 0;
    bool calibrationLoadDetected_ = false;
    int64_t calibrationAccumulator_ = 0;
    uint8_t calibrationSamples_ = 0;
    float calibrationReferenceGrams_ = 0.0f;
    float pendingCalibrationReferenceGrams_ = 0.0f;
    float calibrationFactor_ = 0.0f;
    bool calibrationInProgress_ = false;
    bool calibrationRunSucceeded_ = false;
    bool calibrated_ = false;

    bool hasReading_ = false;
    uint32_t lastReadingTime_ = 0;
};
