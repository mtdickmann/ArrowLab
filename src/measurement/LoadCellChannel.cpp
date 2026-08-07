#include "LoadCellChannel.h"

LoadCellChannel::LoadCellChannel(
    const char *name,
    uint8_t dtPin,
    uint8_t sckPin
)
    : name_(name),
      dtPin_(dtPin),
      sckPin_(sckPin)
{
}

void LoadCellChannel::begin()
{
    Serial.printf(
        "Initializing %s HX711: DT=%u, SCK=%u\n",
        name_,
        dtPin_,
        sckPin_
    );

    hx711_.begin(dtPin_, sckPin_);

    /*
     * Prevent a disconnected DT input from floating low and
     * falsely appearing as a ready HX711.
     */
    pinMode(dtPin_, INPUT_PULLUP);
}

bool LoadCellChannel::read(uint32_t currentTime)
{
    /*
     * A real HX711 briefly becomes not-ready between conversions.
     * Therefore, not-ready is not immediately treated as an error.
     */
    if (!hx711_.is_ready()) {
        return false;
    }

    rawValue_ = hx711_.read();
    hasReading_ = true;
    lastReadingTime_ = currentTime;

    updateTare();
    updateMeasurementFilter();
    updateCalibration();

    if (!tareComplete_) {
        Serial.printf(
            "%s raw: %ld  taring: %u/%u\n",
            name_,
            rawValue_,
            tareSamples_,
            TARE_SAMPLE_COUNT
        );
    }

    return true;
}

bool LoadCellChannel::isLive(
    uint32_t currentTime,
    uint32_t timeoutMs
) const
{
    if (!hasReading_) {
        return false;
    }

    return (
        currentTime - lastReadingTime_
        <= timeoutMs
    );
}

void LoadCellChannel::startUserTare()
{
    startTare(true);
}

void LoadCellChannel::startDiagnosticTare()
{
    /*
     * Diagnostic ZERO/LOAD runs need a fresh offset, but that tare
     * must never satisfy the separate calibration-platform tare
     * requirement shown by the Calibration screen.
     */
    startTare(false);
}

void LoadCellChannel::startTare(bool confirmAsUserTare)
{
    tareOffset_ = 0;
    zeroedValue_ = 0;
    filteredZeroedValue_ = 0;
    filterSampleCount_ = 0;
    filterWriteIndex_ = 0;
    tareAccumulator_ = 0;
    tareSamples_ = 0;
    tareComplete_ = false;
    userTareConfirmed_ = false;
    confirmUserTareOnCompletion_ = confirmAsUserTare;
    calibrationLoadDetectSamples_ = 0;
    calibrationLoadDetectedTime_ = 0;

    // A new tare cancels any calibration sample run, but does not
    // erase an already established calibration factor.
    calibrationAccumulator_ = 0;
    calibrationSamples_ = 0;
    // A tare changes only the zero offset. An established counts/g
    // calibration and its reference mass remain valid.
    if (!calibrated_) {
        calibrationReferenceGrams_ = 0.0f;
    }
    calibrationInProgress_ = false;
}

void LoadCellChannel::updateCalibrationLoadDetection(
    long thresholdCounts
)
{
    if (
        !tareComplete_
        || !userTareConfirmed_
        || calibrationInProgress_
        || thresholdCounts <= 0
    ) {
        calibrationLoadDetectSamples_ = 0;
        calibrationLoadDetectedTime_ = 0;
        return;
    }

    int64_t magnitude =
        static_cast<int64_t>(zeroedValue_);

    if (magnitude < 0) {
        magnitude = -magnitude;
    }

    if (magnitude < thresholdCounts) {
        calibrationLoadDetectSamples_ = 0;
        calibrationLoadDetectedTime_ = 0;
        return;
    }

    if (
        calibrationLoadDetectSamples_
        < CALIBRATION_LOAD_CONFIRM_SAMPLES
    ) {
        calibrationLoadDetectSamples_++;
    }

    if (
        calibrationLoadDetectSamples_
            >= CALIBRATION_LOAD_CONFIRM_SAMPLES
        && calibrationLoadDetectedTime_ == 0
    ) {
        calibrationLoadDetectedTime_ =
            lastReadingTime_;

        Serial.printf(
            "%s calibration load detected: %ld counts\n",
            name_,
            zeroedValue_
        );
    }
}

bool LoadCellChannel::startCalibration(
    float referenceGrams,
    uint32_t currentTime,
    uint32_t settleTimeMs
)
{
    if (
        !calibrationReady(currentTime, settleTimeMs)
        || calibrationInProgress_
        || referenceGrams <= 0.0f
    ) {
        return false;
    }

    calibrationAccumulator_ = 0;
    calibrationSamples_ = 0;
    calibrationReferenceGrams_ = referenceGrams;
    calibrationInProgress_ = true;

    Serial.printf(
        "%s calibration started: reference=%.1f g\n",
        name_,
        calibrationReferenceGrams_
    );

    return true;
}

void LoadCellChannel::updateTare()
{
    if (tareComplete_) {
        zeroedValue_ =
            rawValue_ - tareOffset_;
        return;
    }

    tareAccumulator_ += rawValue_;
    tareSamples_++;

    if (tareSamples_ < TARE_SAMPLE_COUNT) {
        return;
    }

    tareOffset_ = static_cast<long>(
        tareAccumulator_ / TARE_SAMPLE_COUNT
    );

    zeroedValue_ = 0;
    tareComplete_ = true;

    if (confirmUserTareOnCompletion_) {
        userTareConfirmed_ = true;
        calibrationLoadDetectSamples_ = 0;
        calibrationLoadDetectedTime_ = 0;
        confirmUserTareOnCompletion_ = false;
    }

    Serial.printf(
        "%s tare complete: offset=%ld (%u samples)\n",
        name_,
        tareOffset_,
        TARE_SAMPLE_COUNT
    );
}

void LoadCellChannel::updateMeasurementFilter()
{
    if (!tareComplete_) {
        return;
    }

    filterSamples_[filterWriteIndex_] = zeroedValue_;
    filterWriteIndex_ = static_cast<uint8_t>(
        (filterWriteIndex_ + 1) % FILTER_SAMPLE_COUNT
    );

    if (filterSampleCount_ < FILTER_SAMPLE_COUNT) {
        filterSampleCount_++;
    }

    long ordered[FILTER_SAMPLE_COUNT];
    for (uint8_t index = 0; index < filterSampleCount_; index++) {
        ordered[index] = filterSamples_[index];
    }

    for (uint8_t index = 1; index < filterSampleCount_; index++) {
        const long value = ordered[index];
        uint8_t position = index;

        while (position > 0 && ordered[position - 1] > value) {
            ordered[position] = ordered[position - 1];
            position--;
        }

        ordered[position] = value;
    }

    uint8_t trim = 0;
    if (filterSampleCount_ >= FILTER_SAMPLE_COUNT) {
        trim = 3;
    } else if (filterSampleCount_ >= 9) {
        trim = 2;
    } else if (filterSampleCount_ >= 5) {
        trim = 1;
    }

    int64_t accumulator = 0;
    const uint8_t end = filterSampleCount_ - trim;
    for (uint8_t index = trim; index < end; index++) {
        accumulator += ordered[index];
    }

    const uint8_t retainedCount = end - trim;
    filteredZeroedValue_ = static_cast<long>(
        accumulator / retainedCount
    );
}

void LoadCellChannel::updateCalibration()
{
    if (
        !calibrationInProgress_
        || !tareComplete_
    ) {
        return;
    }

    calibrationAccumulator_ += filteredZeroedValue_;
    calibrationSamples_++;

    if (
        calibrationSamples_
        < CALIBRATION_SAMPLE_COUNT
    ) {
        return;
    }

    const float averageCounts =
        static_cast<float>(calibrationAccumulator_)
        / CALIBRATION_SAMPLE_COUNT;

    if (averageCounts == 0.0f) {
        calibrationInProgress_ = false;

        Serial.printf(
            "%s calibration failed: zero signal\n",
            name_
        );
        return;
    }

    calibrationFactor_ =
        averageCounts / calibrationReferenceGrams_;

    calibrated_ = true;
    calibrationInProgress_ = false;
    calibrationLoadDetectSamples_ = 0;
    calibrationLoadDetectedTime_ = 0;

    Serial.printf(
        "%s calibration complete: %.3f counts/g "
        "(average=%.1f counts, reference=%.1f g)\n",
        name_,
        calibrationFactor_,
        averageCounts,
        calibrationReferenceGrams_
    );
}

const char *LoadCellChannel::name() const
{
    return name_;
}

long LoadCellChannel::rawValue() const
{
    return rawValue_;
}

long LoadCellChannel::zeroedValue() const
{
    return zeroedValue_;
}

long LoadCellChannel::filteredZeroedValue() const
{
    return filteredZeroedValue_;
}

bool LoadCellChannel::tareComplete() const
{
    return tareComplete_;
}

bool LoadCellChannel::userTareConfirmed() const
{
    return userTareConfirmed_;
}

bool LoadCellChannel::calibrationReady(
    uint32_t currentTime,
    uint32_t settleTimeMs
) const
{
    if (
        !tareComplete_
        || !userTareConfirmed_
        || calibrationLoadDetectedTime_ == 0
    ) {
        return false;
    }

    return (
        currentTime - calibrationLoadDetectedTime_
        >= settleTimeMs
    );
}

bool LoadCellChannel::calibrationLoadDetected() const
{
    return calibrationLoadDetectedTime_ != 0;
}

uint32_t LoadCellChannel::calibrationSettleRemainingSeconds(
    uint32_t currentTime,
    uint32_t settleTimeMs
) const
{
    if (calibrationLoadDetectedTime_ == 0) {
        return settleTimeMs / 1000;
    }

    const uint32_t elapsed =
        currentTime - calibrationLoadDetectedTime_;

    if (elapsed >= settleTimeMs) {
        return 0;
    }

    return (settleTimeMs - elapsed + 999) / 1000;
}

uint8_t LoadCellChannel::calibrationSettlePercent(
    uint32_t currentTime,
    uint32_t settleTimeMs
) const
{
    if (
        calibrationLoadDetectedTime_ == 0
        || settleTimeMs == 0
    ) {
        return 0;
    }

    const uint32_t elapsed =
        currentTime - calibrationLoadDetectedTime_;

    if (elapsed >= settleTimeMs) {
        return 100;
    }

    return static_cast<uint8_t>(
        (static_cast<uint64_t>(elapsed) * 100)
        / settleTimeMs
    );
}

bool LoadCellChannel::calibrationInProgress() const
{
    return calibrationInProgress_;
}

bool LoadCellChannel::calibrated() const
{
    return calibrated_;
}

float LoadCellChannel::calibrationFactor() const
{
    return calibrationFactor_;
}

float LoadCellChannel::calibrationReferenceGrams() const
{
    return calibrationReferenceGrams_;
}

void LoadCellChannel::restoreCalibration(
    float factor,
    float referenceGrams
)
{
    if (factor == 0.0f || referenceGrams <= 0.0f) {
        clearCalibration();
        return;
    }

    calibrationFactor_ = factor;
    calibrationReferenceGrams_ = referenceGrams;
    calibrated_ = true;
    calibrationInProgress_ = false;
}

void LoadCellChannel::clearCalibration()
{
    calibrationAccumulator_ = 0;
    calibrationSamples_ = 0;
    calibrationReferenceGrams_ = 0.0f;
    calibrationFactor_ = 0.0f;
    calibrationInProgress_ = false;
    calibrated_ = false;
}

float LoadCellChannel::grams() const
{
    if (
        !calibrated_
        || calibrationFactor_ == 0.0f
    ) {
        return 0.0f;
    }

    return (
        static_cast<float>(filteredZeroedValue_)
        / calibrationFactor_
    );
}

float LoadCellChannel::rawGrams() const
{
    if (
        !calibrated_
        || calibrationFactor_ == 0.0f
    ) {
        return 0.0f;
    }

    return (
        static_cast<float>(zeroedValue_)
        / calibrationFactor_
    );
}
