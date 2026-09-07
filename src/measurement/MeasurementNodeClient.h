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
    bool startSpineTest(uint8_t positionCount);
    bool cancelSpineTest();
    bool confirmSpineClear();
    bool confirmSpineZero();
    bool restartSpineAttempt();
    bool testWifiCredentials(const char *ssid, const char *password);
    bool commitWifiCredentials();
    bool revertWifiCredentials();
    bool forgetWifiProfile(const char *ssid);

    bool connected(uint32_t currentTime) const;
    bool freshPacket() const;
    const ArrowLabProtocol::StatusPacket &status() const;
    const ArrowLabProtocol::ChannelStatus &channel(
        ArrowLabProtocol::Side side) const;

private:
    struct QueuedCommand
    {
        ArrowLabProtocol::CommandType command =
            ArrowLabProtocol::CommandType::None;
        ArrowLabProtocol::Side side = ArrowLabProtocol::Side::Left;
        int32_t referenceMilliGrams = 0;
        char wifiSsid[33] = {};
        char wifiPassword[65] = {};
    };

    static constexpr size_t COMMAND_QUEUE_CAPACITY = 8;
    static constexpr uint32_t COMMAND_ACK_TIMEOUT_MS = 250;
    static constexpr uint8_t COMMAND_MAX_RETRIES = 4;

    void acceptByte(uint8_t value, uint32_t currentTime);
    void serviceTransport(uint32_t currentTime);
    bool transmit(const ArrowLabProtocol::CommandPacket &packet);
    bool send(
        ArrowLabProtocol::CommandType command,
        ArrowLabProtocol::Side side,
        int32_t referenceMilliGrams,
        const char *wifiSsid = nullptr,
        const char *wifiPassword = nullptr);

    HardwareSerial nodeSerial_{1};
    ArrowLabProtocol::StatusPacket status_;
    uint8_t receiveBuffer_[sizeof(ArrowLabProtocol::StatusPacket)] = {};
    size_t receiveLength_ = 0;
    uint32_t lastValidPacketTime_ = 0;
    uint32_t lastPollTime_ = 0;
    uint16_t commandSequence_ = 0;
    QueuedCommand commandQueue_[COMMAND_QUEUE_CAPACITY] = {};
    size_t commandQueueHead_ = 0;
    size_t commandQueueTail_ = 0;
    size_t commandQueueCount_ = 0;
    portMUX_TYPE commandQueueMux_ = portMUX_INITIALIZER_UNLOCKED;
    ArrowLabProtocol::CommandPacket inFlightPacket_ = {};
    uint32_t inFlightSentAt_ = 0;
    uint8_t inFlightRetryCount_ = 0;
    bool commandInFlight_ = false;
    bool hasPacket_ = false;
    bool freshPacket_ = false;
    bool firstPollLogged_ = false;
};
