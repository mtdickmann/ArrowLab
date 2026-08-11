#include "MeasurementNodeClient.h"

#include <cmath>
#include <cstring>

namespace
{
    constexpr uint32_t NODE_UART_BAUD = 115200;
    // Reuse the former right-HX711 pins for this UART trial. That channel
    // proved stable before metrology moved to the WROOM, so GPIO11/12 are
    // the best known pair to test without involving display or touch pins.
    constexpr int8_t NODE_UART_RX_PIN = 11;
    constexpr int8_t NODE_UART_TX_PIN = 12;
    constexpr uint8_t STATUS_MAGIC_LOW =
        ArrowLabProtocol::STATUS_MAGIC & 0xFF;
    constexpr uint8_t STATUS_MAGIC_HIGH =
        ArrowLabProtocol::STATUS_MAGIC >> 8;
}

bool MeasurementNodeClient::begin()
{
    hasPacket_ = false;
    freshPacket_ = false;
    lastValidPacketTime_ = 0;
    receiveLength_ = 0;
    nodeSerial_.begin(
        NODE_UART_BAUD,
        SERIAL_8N1,
        NODE_UART_RX_PIN,
        NODE_UART_TX_PIN);
    return true;
}

bool MeasurementNodeClient::poll(uint32_t currentTime)
{
    freshPacket_ = false;
    while (nodeSerial_.available() > 0) {
        acceptByte(static_cast<uint8_t>(nodeSerial_.read()), currentTime);
    }
    return freshPacket_;
}

void MeasurementNodeClient::acceptByte(uint8_t value, uint32_t currentTime)
{
    if (receiveLength_ == 0 && value != STATUS_MAGIC_LOW) return;

    if (receiveLength_ == 1 && value != STATUS_MAGIC_HIGH) {
        receiveLength_ = value == STATUS_MAGIC_LOW ? 1 : 0;
        if (receiveLength_ == 1) receiveBuffer_[0] = value;
        return;
    }

    receiveBuffer_[receiveLength_++] = value;
    if (receiveLength_ < sizeof(receiveBuffer_)) return;

    ArrowLabProtocol::StatusPacket incoming;
    std::memcpy(&incoming, receiveBuffer_, sizeof(incoming));
    receiveLength_ = 0;

    if (!ArrowLabProtocol::valid(incoming)) return;

    freshPacket_ = !hasPacket_ || incoming.sequence != status_.sequence;
    status_ = incoming;
    hasPacket_ = true;
    lastValidPacketTime_ = currentTime;
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

    return nodeSerial_.write(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(packet)) == sizeof(packet);
}
