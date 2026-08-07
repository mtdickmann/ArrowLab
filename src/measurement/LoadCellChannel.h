#pragma once

#include <Arduino.h>
#include <HX711.h>

/*
 * Physical HX711 channel only.
 *
 * This class deliberately knows nothing about tare, calibration, filtering,
 * displayed mass or diagnostics.  Every fresh conversion is exposed as the
 * original signed ADC count so higher layers cannot accidentally feed an
 * already-adjusted value back into another calculation.
 */
class LoadCellChannel
{
public:
    LoadCellChannel(const char *name, uint8_t dtPin, uint8_t sckPin);

    void begin();
    bool read(uint32_t currentTime);
    bool isLive(uint32_t currentTime, uint32_t timeoutMs) const;

    const char *name() const;
    long rawValue() const;

private:
    HX711 hx711_;
    const char *name_;
    uint8_t dtPin_;
    uint8_t sckPin_;
    long rawValue_ = 0;
    bool hasReading_ = false;
    uint32_t lastReadingTime_ = 0;
};
