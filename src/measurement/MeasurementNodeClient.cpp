#include "MeasurementNodeClient.h"

#include <Wire.h>

#include <cmath>

bool MeasurementNodeClient::begin()
{
    hasPacket_ = false;
    freshPacket_ = false;
    lastValidPacketTime_ = 0;
    return Wire.begin(SDA_PIN, SCL_PIN, BUS_FREQUENCY_HZ);
}

bool MeasurementNodeClient::poll(uint32_t currentTime)
{
    freshPacket_ = false;
    ArrowLabProtocol::StatusPacket incoming;
    auto *destination = reinterpret_cast<uint8_t *>(&incoming);

    const size_t expected = sizeof(incoming);
    const size_t received = Wire.requestFrom(
        ArrowLabProtocol::I2C_ADDRESS,
        expected,
        true);

    if (received != expected) {
        while (Wire.available() > 0) Wire.read();
        return false;
    }

    size_t index = 0;
    while (Wire.available() > 0 && index < expected) {
        destination[index++] = static_cast<uint8_t>(Wire.read());
    }

    if (index != expected || !ArrowLabProtocol::valid(incoming)) {
        return false;
    }

    freshPacket_ = !hasPacket_ || incoming.sequence != status_.sequence;
    status_ = incoming;
    hasPacket_ = true;
    lastValidPacketTime_ = currentTime;
    return true;
}

bool MeasurementNodeClient::requestTare(ArrowLabProtocol::Side side)
{
    return send(ArrowLabProtocol::CommandType::Tare, side, 0);
}

bool MeasurementNodeClient::prepareCalibration(
    ArrowLabProtocol::Side side,
    float referenceGrams)
{
    if (referenceGrams <= 0.0f) return false;
    return send(
        ArrowLabProtocol::CommandType::PrepareCalibration,
        side,
        static_cast<int32_t>(std::lround(referenceGrams * 1000.0f)));
}

bool MeasurementNodeClient::startCalibration(ArrowLabProtocol::Side side)
{
    return send(ArrowLabProtocol::CommandType::StartCalibration, side, 0);
}

bool MeasurementNodeClient::connected(uint32_t currentTime) const
{
    return hasPacket_
        && currentTime - lastValidPacketTime_ <= LINK_TIMEOUT_MS;
}

bool MeasurementNodeClient::freshPacket() const
{
    return freshPacket_;
}

const ArrowLabProtocol::StatusPacket &MeasurementNodeClient::status() const
{
    return status_;
}

const ArrowLabProtocol::ChannelStatus &MeasurementNodeClient::channel(
    ArrowLabProtocol::Side side) const
{
    return side == ArrowLabProtocol::Side::Left
        ? status_.left
        : status_.right;
}

bool MeasurementNodeClient::send(
    ArrowLabProtocol::CommandType command,
    ArrowLabProtocol::Side side,
    int32_t referenceMilliGrams)
{
    ArrowLabProtocol::CommandPacket packet;
    packet.sequence = ++commandSequence_;
    packet.command = static_cast<uint8_t>(command);
    packet.side = static_cast<uint8_t>(side);
    packet.referenceMilliGrams = referenceMilliGrams;
    ArrowLabProtocol::seal(packet);

    Wire.beginTransmission(ArrowLabProtocol::I2C_ADDRESS);
    Wire.write(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(packet));
    return Wire.endTransmission(true) == 0;
}
