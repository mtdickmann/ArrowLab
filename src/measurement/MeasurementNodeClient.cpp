#include "MeasurementNodeClient.h"

#include <cmath>
#include <cstring>

#include "ArrowLabConfig.h"
#include "OneWireUart.h"

#include <driver/gpio.h>

namespace
{
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
    firstPollLogged_ = false;
    nodeSerial_.begin(
        ArrowLabConfig::MEASUREMENT_LINK_BAUD,
        SERIAL_8N1,
        ArrowLabConfig::vieweMeasurementRxPin(),
        ArrowLabConfig::vieweMeasurementTxPin());

    if (ArrowLabConfig::measurementLinkIsOneWire()) {
        const gpio_num_t signalPin = static_cast<gpio_num_t>(
            ArrowLabConfig::vieweMeasurementRxPin());
        const bool matrixAttachOk = OneWireUart::attach(
            UART_NUM_1,
            signalPin);
        Serial.printf(
            "AL_HMI,CONFIG,ONE_WIRE_MATRIX_ATTACH=%s,RX=%d,TX=%d\n",
            matrixAttachOk ? "OK" : "FAILED",
            ArrowLabConfig::vieweMeasurementRxPin(),
            ArrowLabConfig::vieweMeasurementTxPin());
        if (!matrixAttachOk) {
            return false;
        }
        while (nodeSerial_.available() > 0) nodeSerial_.read();
    }

    lastPollTime_ = millis()
        - ArrowLabConfig::MEASUREMENT_STATUS_INTERVAL_MS;
    return true;
}

bool MeasurementNodeClient::poll(uint32_t currentTime)
{
    freshPacket_ = false;
    while (nodeSerial_.available() > 0) {
        acceptByte(static_cast<uint8_t>(nodeSerial_.read()), currentTime);
    }

    if (
        ArrowLabConfig::measurementLinkIsOneWire()
        && currentTime - lastPollTime_
            >= ArrowLabConfig::MEASUREMENT_STATUS_INTERVAL_MS
    ) {
        send(
            ArrowLabProtocol::CommandType::PollStatus,
            ArrowLabProtocol::Side::Left,
            0);
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

bool MeasurementNodeClient::startSpineTest(uint8_t positionCount)
{
    if (positionCount != 1 && positionCount != 4) return false;
    return send(
        ArrowLabProtocol::CommandType::StartSpineTest,
        ArrowLabProtocol::Side::Left,
        positionCount);
}

bool MeasurementNodeClient::cancelSpineTest()
{
    return send(
        ArrowLabProtocol::CommandType::CancelSpineTest,
        ArrowLabProtocol::Side::Left,
        0);
}

bool MeasurementNodeClient::confirmSpineZero()
{
    return send(
        ArrowLabProtocol::CommandType::ConfirmSpineZero,
        ArrowLabProtocol::Side::Left,
        0);
}

bool MeasurementNodeClient::restartSpineAttempt()
{
    return send(
        ArrowLabProtocol::CommandType::RestartSpineAttempt,
        ArrowLabProtocol::Side::Left,
        0);
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

    const bool sent = nodeSerial_.write(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(packet)) == sizeof(packet);
    if (ArrowLabConfig::measurementLinkIsOneWire()) {
        nodeSerial_.flush();
        lastPollTime_ = millis();
        if (
            command == ArrowLabProtocol::CommandType::PollStatus
            && !firstPollLogged_
        ) {
            Serial.printf(
                "AL_HMI,LINK,POLL_QUEUED,SEQ=%u,WRITE=%s\n",
                packet.sequence,
                sent ? "OK" : "FAILED");
            firstPollLogged_ = true;
        }
    }
    return sent;
}
