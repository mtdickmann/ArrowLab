#include "MeasurementNodeClient.h"

#include <driver/i2c.h>

#include <cmath>

namespace
{
    constexpr i2c_port_t SHARED_I2C_PORT = I2C_NUM_0;
    constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(20);
}

bool MeasurementNodeClient::begin()
{
    hasPacket_ = false;
    freshPacket_ = false;
    lastValidPacketTime_ = 0;
    return true;
}

bool MeasurementNodeClient::poll(uint32_t currentTime)
{
    freshPacket_ = false;
    ArrowLabProtocol::StatusPacket incoming;
    auto *destination = reinterpret_cast<uint8_t *>(&incoming);

    const size_t expected = sizeof(incoming);
    const esp_err_t result = i2c_master_read_from_device(
        SHARED_I2C_PORT,
        ArrowLabProtocol::I2C_ADDRESS,
        destination,
        expected,
        I2C_TIMEOUT_TICKS);

    if (result != ESP_OK) {
        return false;
    }

    if (!ArrowLabProtocol::valid(incoming)) {
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

    return i2c_master_write_to_device(
        SHARED_I2C_PORT,
        ArrowLabProtocol::I2C_ADDRESS,
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(packet),
        I2C_TIMEOUT_TICKS) == ESP_OK;
}
