#include "InstrumentStorage.h"

#include "Version.h"

namespace
{
    constexpr char STORAGE_NAMESPACE[] = "arrowlab_state";

    uint32_t firmwareVersionCode()
    {
        return (
            static_cast<uint32_t>(Version::MAJOR) * 10000UL
            + static_cast<uint32_t>(Version::MINOR) * 100UL
            + static_cast<uint32_t>(Version::PATCH)
        );
    }
}

bool InstrumentStorage::begin()
{
    ready_ = preferences_.begin(STORAGE_NAMESPACE, false);
    return ready_;
}

const char *InstrumentStorage::key(
    StoredLoadSide side,
    const char *leftKey,
    const char *rightKey
) const
{
    return side == StoredLoadSide::Left
        ? leftKey
        : rightKey;
}

bool InstrumentStorage::loadCalibration(
    StoredLoadSide side,
    StoredCalibration &record
)
{
    record = StoredCalibration{};

    if (!ready_) {
        return false;
    }

    const bool valid = preferences_.getBool(
        key(side, "lc_ok", "rc_ok"),
        false
    );

    const uint32_t storedVersion = preferences_.getUInt(
        key(side, "lc_ver", "rc_ver"),
        0
    );

    const float factor = preferences_.getFloat(
        key(side, "lc_fac", "rc_fac"),
        0.0f
    );

    const float reference = preferences_.getFloat(
        key(side, "lc_ref", "rc_ref"),
        0.0f
    );

    if (
        !valid
        || storedVersion != firmwareVersionCode()
        || factor == 0.0f
        || reference <= 0.0f
    ) {
        return false;
    }

    record.valid = true;
    record.factor = factor;
    record.referenceGrams = reference;
    return true;
}

bool InstrumentStorage::saveCalibration(
    StoredLoadSide side,
    float factor,
    float referenceGrams
)
{
    if (
        !ready_
        || factor == 0.0f
        || referenceGrams <= 0.0f
    ) {
        return false;
    }

    const size_t factorBytes = preferences_.putFloat(
        key(side, "lc_fac", "rc_fac"),
        factor
    );

    const size_t referenceBytes = preferences_.putFloat(
        key(side, "lc_ref", "rc_ref"),
        referenceGrams
    );

    const size_t versionBytes = preferences_.putUInt(
        key(side, "lc_ver", "rc_ver"),
        firmwareVersionCode()
    );

    const size_t validBytes = preferences_.putBool(
        key(side, "lc_ok", "rc_ok"),
        true
    );

    return (
        factorBytes > 0
        && referenceBytes > 0
        && versionBytes > 0
        && validBytes > 0
    );
}
