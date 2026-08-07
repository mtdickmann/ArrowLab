#include "LoadCellChannel.h"

LoadCellChannel::LoadCellChannel(
    const char *name,
    uint8_t dtPin,
    uint8_t sckPin
)
    : name_(name), dtPin_(dtPin), sckPin_(sckPin)
{
}

void LoadCellChannel::begin()
{
    Serial.printf(
        "Initializing %s HX711: DT=%u, SCK=%u\n",
        name_,
        dtPin_,
        sckPin_);

    hx711_.begin(dtPin_, sckPin_);

    // A disconnected DT input must not float low and impersonate an HX711.
    pinMode(dtPin_, INPUT_PULLUP);
}

bool LoadCellChannel::read(uint32_t currentTime)
{
    if (!hx711_.is_ready()) {
        return false;
    }

    rawValue_ = hx711_.read();
    hasReading_ = true;
    lastReadingTime_ = currentTime;
    return true;
}

bool LoadCellChannel::isLive(
    uint32_t currentTime,
    uint32_t timeoutMs
) const
{
    return hasReading_
        && currentTime - lastReadingTime_ <= timeoutMs;
}

const char *LoadCellChannel::name() const
{
    return name_;
}

long LoadCellChannel::rawValue() const
{
    return rawValue_;
}
