#pragma once

#include <Arduino.h>
#include <Preferences.h>

enum class StoredLoadSide
{
    Left,
    Right
};

struct StoredCalibration
{
    bool valid = false;
    float factor = 0.0f;
    float referenceGrams = 0.0f;
};

class InstrumentStorage
{
public:
    bool begin();

    bool loadCalibration(
        StoredLoadSide side,
        StoredCalibration &record
    );

    bool saveCalibration(
        StoredLoadSide side,
        float factor,
        float referenceGrams
    );

private:
    const char *key(
        StoredLoadSide side,
        const char *leftKey,
        const char *rightKey
    ) const;

    Preferences preferences_;
    bool ready_ = false;
};
