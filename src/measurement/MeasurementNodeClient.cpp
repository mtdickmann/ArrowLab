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
    commandQueueHead_ = 0;
    commandQueueTail_ = 0;
    commandQueueCount_ = 0;
    commandInFlight_ = false;
    inFlightRetryCount_ = 0;
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

    if (ArrowLabConfig::measurementLinkIsOneWire()) {
        serviceTransport(currentTime);
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

    if (
        commandInFlight_
        && incoming.lastCommandSequence == inFlightPacket_.sequence
    ) {
        Serial.printf(
            "AL_HMI,LINK,COMMAND_ACK,SEQ=%u,CMD=%u,RETRIES=%u\n",
            inFlightPacket_.sequence,
            inFlightPacket_.command,
            inFlightRetryCount_);
        commandInFlight_ = false;
        inFlightRetryCount_ = 0;
    }
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

bool MeasurementNodeClient::confirmSpineClear()
{
    return send(
        ArrowLabProtocol::CommandType::ConfirmSpineClear,
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

bool MeasurementNodeClient::testWifiCredentials(
    const char *ssid,
    const char *password)
{
    return send(
        ArrowLabProtocol::CommandType::TestWifiCredentials,
        ArrowLabProtocol::Side::Left,
        0,
        ssid,
        password);
}

bool MeasurementNodeClient::commitWifiCredentials()
{
    return send(
        ArrowLabProtocol::CommandType::CommitWifiCredentials,
        ArrowLabProtocol::Side::Left,
        0);
}

bool MeasurementNodeClient::revertWifiCredentials()
{
    return send(
        ArrowLabProtocol::CommandType::RevertWifiCredentials,
        ArrowLabProtocol::Side::Left,
        0);
}

bool MeasurementNodeClient::forgetWifiProfile(const char *ssid)
{
    return send(
        ArrowLabProtocol::CommandType::ForgetWifiProfile,
        ArrowLabProtocol::Side::Left,
        0,
        ssid,
        nullptr);
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

void MeasurementNodeClient::serviceTransport(uint32_t currentTime)
{
    if (commandInFlight_) {
        if (currentTime - inFlightSentAt_ < COMMAND_ACK_TIMEOUT_MS) return;

        if (inFlightRetryCount_ >= COMMAND_MAX_RETRIES) {
            Serial.printf(
                "AL_HMI,LINK,COMMAND_FAILED,SEQ=%u,CMD=%u\n",
                inFlightPacket_.sequence,
                inFlightPacket_.command);
            commandInFlight_ = false;
            inFlightRetryCount_ = 0;
        } else {
            ++inFlightRetryCount_;
            const bool sent = transmit(inFlightPacket_);
            inFlightSentAt_ = millis();
            Serial.printf(
                "AL_HMI,LINK,COMMAND_RETRY,SEQ=%u,CMD=%u,TRY=%u,WRITE=%s\n",
                inFlightPacket_.sequence,
                inFlightPacket_.command,
                inFlightRetryCount_,
                sent ? "OK" : "FAILED");
            return;
        }
    }

    QueuedCommand queued;
    bool haveCommand = false;
    portENTER_CRITICAL(&commandQueueMux_);
    if (commandQueueCount_ > 0) {
        queued = commandQueue_[commandQueueHead_];
        commandQueueHead_ =
            (commandQueueHead_ + 1) % COMMAND_QUEUE_CAPACITY;
        --commandQueueCount_;
        haveCommand = true;
    }
    portEXIT_CRITICAL(&commandQueueMux_);

    if (haveCommand) {
        inFlightPacket_ = {};
        inFlightPacket_.sequence = ++commandSequence_;
        inFlightPacket_.command = static_cast<uint8_t>(queued.command);
        inFlightPacket_.side = static_cast<uint8_t>(queued.side);
        inFlightPacket_.referenceMilliGrams = queued.referenceMilliGrams;
        snprintf(
            inFlightPacket_.wifiSsid,
            sizeof(inFlightPacket_.wifiSsid),
            "%s",
            queued.wifiSsid);
        snprintf(
            inFlightPacket_.wifiPassword,
            sizeof(inFlightPacket_.wifiPassword),
            "%s",
            queued.wifiPassword);
        ArrowLabProtocol::seal(inFlightPacket_);

        commandInFlight_ = true;
        inFlightRetryCount_ = 0;
        const bool sent = transmit(inFlightPacket_);
        inFlightSentAt_ = millis();
        Serial.printf(
            "AL_HMI,LINK,COMMAND_SENT,SEQ=%u,CMD=%u,WRITE=%s\n",
            inFlightPacket_.sequence,
            inFlightPacket_.command,
            sent ? "OK" : "FAILED");
        return;
    }

    if (
        currentTime - lastPollTime_
        < ArrowLabConfig::MEASUREMENT_STATUS_INTERVAL_MS
    ) {
        return;
    }

    ArrowLabProtocol::CommandPacket pollPacket;
    pollPacket.sequence = ++commandSequence_;
    pollPacket.command = static_cast<uint8_t>(
        ArrowLabProtocol::CommandType::PollStatus);
    pollPacket.side = static_cast<uint8_t>(ArrowLabProtocol::Side::Left);
    ArrowLabProtocol::seal(pollPacket);
    const bool sent = transmit(pollPacket);
    if (!firstPollLogged_) {
        Serial.printf(
            "AL_HMI,LINK,POLL_QUEUED,SEQ=%u,WRITE=%s\n",
            pollPacket.sequence,
            sent ? "OK" : "FAILED");
        firstPollLogged_ = true;
    }
}

bool MeasurementNodeClient::transmit(
    const ArrowLabProtocol::CommandPacket &packet)
{
    const bool sent = nodeSerial_.write(
        reinterpret_cast<const uint8_t *>(&packet),
        sizeof(packet)) == sizeof(packet);
    if (ArrowLabConfig::measurementLinkIsOneWire()) {
        nodeSerial_.flush();
        lastPollTime_ = millis();
    }
    return sent;
}

bool MeasurementNodeClient::send(
    ArrowLabProtocol::CommandType command,
    ArrowLabProtocol::Side side,
    int32_t referenceMilliGrams,
    const char *wifiSsid,
    const char *wifiPassword)
{
    QueuedCommand queued;
    queued.command = command;
    queued.side = side;
    queued.referenceMilliGrams = referenceMilliGrams;
    if (wifiSsid != nullptr) {
        snprintf(queued.wifiSsid, sizeof(queued.wifiSsid), "%s", wifiSsid);
    }
    if (wifiPassword != nullptr) {
        snprintf(
            queued.wifiPassword,
            sizeof(queued.wifiPassword),
            "%s",
            wifiPassword);
    }

    bool accepted = false;
    portENTER_CRITICAL(&commandQueueMux_);
    if (commandQueueCount_ < COMMAND_QUEUE_CAPACITY) {
        commandQueue_[commandQueueTail_] = queued;
        commandQueueTail_ =
            (commandQueueTail_ + 1) % COMMAND_QUEUE_CAPACITY;
        ++commandQueueCount_;
        accepted = true;
    }
    portEXIT_CRITICAL(&commandQueueMux_);

    if (!accepted) {
        Serial.printf(
            "AL_HMI,LINK,COMMAND_QUEUE_FULL,CMD=%u\n",
            static_cast<unsigned>(command));
    }
    return accepted;
}
