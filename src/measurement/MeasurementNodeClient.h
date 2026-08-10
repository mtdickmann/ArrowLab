#pragma once

#include <Arduino.h>

#include "protocol/MeasurementProtocol.h"

class MeasurementNodeClient
{
public:
    static constexpr uint32_t LINK_TIMEOUT_MS = 1500;

    bool begin();
    bool poll(uint32_t currentTime);

    bool requestTare(ArrowLabProtocol::Side side);
    bool prepareCalibration(
        ArrowLabProtocol::Side side,
        float referenceGrams);
    bool startCalibration(ArrowLabProtocol::Side side);

    bool connected(uint32_t currentTime) const;
    bool freshPacket() const;
    const ArrowLabProtocol::StatusPacket &status() const;
    const ArrowLabProtocol::ChannelStatus &channel(
        ArrowLabProtocol::Side side) const;

private:
    bool send(
        ArrowLabProtocol::CommandType command,
        ArrowLabProtocol::Side side,
        int32_t referenceMilliGrams);

    ArrowLabProtocol::StatusPacket status_;
    uint32_t lastValidPacketTime_ = 0;
    uint16_t commandSequence_ = 0;
    bool hasPacket_ = false;
    bool freshPacket_ = false;
};
