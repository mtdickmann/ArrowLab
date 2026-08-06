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
    updateCalibration();

    if (tareComplete_) {
        Serial.printf(
            "%s raw: %ld  zeroed: %ld\n",
            name_,
            rawValue_,
            zeroedValue_
        );
    } else {
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
    tareOffset_ = 0;
    zeroedValue_ = 0;
    tareAccumulator_ = 0;
    tareSamples_ = 0;
    tareComplete_ = false;
    userTareConfirmed_ = false;
    confirmUserTareOnCompletion_ = true;

    // A new tare cancels any calibration sample run, but does not
    // erase an already established calibration factor.
    calibrationAccumulator_ = 0;
    calibrationSamples_ = 0;
    calibrationReferenceGrams_ = 0.0f;
    calibrationInProgress_ = false;
}

bool LoadCellChannel::startCalibration(float referenceGrams)
{
    if (
        !tareComplete_
        || !userTareConfirmed_
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
        confirmUserTareOnCompletion_ = false;
    }

    Serial.printf(
        "%s tare complete: offset=%ld (%u samples)\n",
        name_,
        tareOffset_,
        TARE_SAMPLE_COUNT
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

    calibrationAccumulator_ += zeroedValue_;
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

bool LoadCellChannel::tareComplete() const
{
    return tareComplete_;
}

bool LoadCellChannel::userTareConfirmed() const
{
    return userTareConfirmed_;
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

float LoadCellChannel::grams() const
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
