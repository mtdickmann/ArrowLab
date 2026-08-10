#include <Arduino.h>
#include <Wire.h>

#include <cmath>
#include <cstring>

#include "Version.h"
#include "calibration/CalibrationController.h"
#include "measurement/LoadCellChannel.h"
#include "measurement/MeasurementChannel.h"
#include "protocol/MeasurementProtocol.h"
#include "storage/InstrumentStorage.h"

namespace
{
    constexpr uint8_t LEFT_DT_PIN = 4;
    constexpr uint8_t LEFT_SCK_PIN = 5;
    constexpr uint8_t RIGHT_DT_PIN = 6;
    constexpr uint8_t RIGHT_SCK_PIN = 7;
    constexpr uint8_t I2C_SDA_PIN = 8;
    constexpr uint8_t I2C_SCL_PIN = 9;
    constexpr uint32_t I2C_FREQUENCY_HZ = 400000;
    constexpr uint32_t SENSOR_INTERVAL_MS = 10;
    constexpr uint32_t SENSOR_TIMEOUT_MS = 1500;

    LoadCellChannel leftSensor("Left", LEFT_DT_PIN, LEFT_SCK_PIN);
    LoadCellChannel rightSensor("Right", RIGHT_DT_PIN, RIGHT_SCK_PIN);
    MeasurementChannel leftMeasurement;
    MeasurementChannel rightMeasurement;
    InstrumentStorage instrumentStorage;
    CalibrationController calibrationController(
        leftMeasurement,
        rightMeasurement,
        instrumentStorage);

    ArrowLabProtocol::StatusPacket statusPacket;
    ArrowLabProtocol::CommandPacket pendingCommand;
    volatile bool commandPending = false;
    uint16_t statusSequence = 0;
    uint16_t lastCommandSequence = 0;
    uint32_t lastSensorUpdate = 0;
    bool streamRawUsb = false;
    char serialCommand[24] = {};
    size_t serialCommandLength = 0;

    CalibrationSide calibrationSide(ArrowLabProtocol::Side side)
    {
        return side == ArrowLabProtocol::Side::Left
            ? CalibrationSide::Left
            : CalibrationSide::Right;
    }

    uint8_t protocolStage(CalibrationController::Stage stage)
    {
        using ProtocolStage = ArrowLabProtocol::CalibrationStage;
        switch (stage) {
        case CalibrationController::Stage::NeedsTare:
            return static_cast<uint8_t>(ProtocolStage::NeedsTare);
        case CalibrationController::Stage::Taring:
            return static_cast<uint8_t>(ProtocolStage::Taring);
        case CalibrationController::Stage::Ready:
            return static_cast<uint8_t>(ProtocolStage::Ready);
        case CalibrationController::Stage::AwaitingLoad:
            return static_cast<uint8_t>(ProtocolStage::AwaitingLoad);
        case CalibrationController::Stage::ReadyToCalibrate:
            return static_cast<uint8_t>(ProtocolStage::ReadyToCalibrate);
        case CalibrationController::Stage::Settling:
            return static_cast<uint8_t>(ProtocolStage::Settling);
        case CalibrationController::Stage::Sampling:
            return static_cast<uint8_t>(ProtocolStage::Sampling);
        }
        return static_cast<uint8_t>(ProtocolStage::NeedsTare);
    }

    void fillChannelStatus(
        ArrowLabProtocol::ChannelStatus &destination,
        CalibrationSide side,
        const LoadCellChannel &sensor,
        const MeasurementChannel &measurement,
        uint32_t now)
    {
        const CalibrationController::ChannelStatus calibration =
            calibrationController.status(side, now);

        destination.rawCount = sensor.rawValue();
        destination.heldRawCounts = measurement.heldRawCounts();
        destination.heldMilliGrams = static_cast<int32_t>(
            std::lround(measurement.heldGrams() * 1000.0f));
        destination.referenceMilliGrams = static_cast<int32_t>(
            std::lround(calibration.referenceGrams * 1000.0f));
        destination.settleRemainingSeconds =
            static_cast<uint16_t>(calibration.settleRemainingSeconds);
        destination.stage = protocolStage(calibration.stage);
        destination.settlePercent = calibration.settlePercent;
        destination.flags = 0;

        if (sensor.isLive(now, SENSOR_TIMEOUT_MS)) {
            destination.flags |= ArrowLabProtocol::ChannelLive;
        }
        if (measurement.tareComplete()) {
            destination.flags |= ArrowLabProtocol::TareComplete;
        }
        if (measurement.userTareConfirmed()) {
            destination.flags |= ArrowLabProtocol::UserTareConfirmed;
        }
        if (measurement.calibrated()) {
            destination.flags |= ArrowLabProtocol::Calibrated;
        }
    }

    void refreshStatus(uint32_t now)
    {
        statusPacket.sequence = ++statusSequence;
        statusPacket.lastCommandSequence = lastCommandSequence;
        fillChannelStatus(
            statusPacket.left,
            CalibrationSide::Left,
            leftSensor,
            leftMeasurement,
            now);
        fillChannelStatus(
            statusPacket.right,
            CalibrationSide::Right,
            rightSensor,
            rightMeasurement,
            now);
        ArrowLabProtocol::seal(statusPacket);
    }

    void receiveCommand(int byteCount)
    {
        ArrowLabProtocol::CommandPacket incoming;
        auto *bytes = reinterpret_cast<uint8_t *>(&incoming);
        size_t received = 0;

        while (Wire.available() > 0 && received < sizeof(incoming)) {
            bytes[received++] = static_cast<uint8_t>(Wire.read());
        }
        while (Wire.available() > 0) Wire.read();

        if (
            byteCount != static_cast<int>(sizeof(incoming))
            || received != sizeof(incoming)
            || !ArrowLabProtocol::valid(incoming)
        ) {
            return;
        }

        pendingCommand = incoming;
        commandPending = true;
    }

    void sendStatus()
    {
        Wire.write(
            reinterpret_cast<const uint8_t *>(&statusPacket),
            sizeof(statusPacket));
    }

    void processPendingCommand()
    {
        if (!commandPending) return;

        noInterrupts();
        const ArrowLabProtocol::CommandPacket command = pendingCommand;
        commandPending = false;
        interrupts();

        if (
            command.side > static_cast<uint8_t>(ArrowLabProtocol::Side::Right)
        ) {
            return;
        }

        const auto side = static_cast<ArrowLabProtocol::Side>(command.side);
        const CalibrationSide localSide = calibrationSide(side);
        const auto type = static_cast<ArrowLabProtocol::CommandType>(
            command.command);

        switch (type) {
        case ArrowLabProtocol::CommandType::Tare:
            calibrationController.requestTare(localSide);
            break;

        case ArrowLabProtocol::CommandType::PrepareCalibration:
            calibrationController.requestCalibration(
                localSide,
                static_cast<float>(command.referenceMilliGrams) / 1000.0f);
            break;

        case ArrowLabProtocol::CommandType::StartCalibration:
            calibrationController.requestCalibration(localSide, 0.0f);
            break;

        case ArrowLabProtocol::CommandType::None:
        default:
            return;
        }

        lastCommandSequence = command.sequence;
        Serial.printf(
            "AL_NODE,EVENT,COMMAND,%u,%s,%u\n",
            command.sequence,
            side == ArrowLabProtocol::Side::Left ? "LEFT" : "RIGHT",
            command.command);
    }

    void processSerialCommands()
    {
        while (Serial.available() > 0) {
            const char value = static_cast<char>(Serial.read());
            if (value == '\r') continue;

            if (value == '\n') {
                serialCommand[serialCommandLength] = '\0';
                if (strcmp(serialCommand, "STREAM ON") == 0) {
                    streamRawUsb = true;
                    Serial.println("AL_NODE,EVENT,STREAM,ON");
                } else if (strcmp(serialCommand, "STREAM OFF") == 0) {
                    streamRawUsb = false;
                    Serial.println("AL_NODE,EVENT,STREAM,OFF");
                }
                serialCommandLength = 0;
                continue;
            }

            if (serialCommandLength < sizeof(serialCommand) - 1) {
                serialCommand[serialCommandLength++] = value;
            } else {
                serialCommandLength = 0;
            }
        }
    }

    void sampleChannel(
        LoadCellChannel &sensor,
        MeasurementChannel &measurement,
        CalibrationSide side,
        const char *name,
        uint32_t now)
    {
        if (!sensor.read(now)) return;

        measurement.onRawSample(sensor.rawValue(), now);
        calibrationController.onFreshReading(side);

        if (streamRawUsb) {
            Serial.printf(
                "AL_NODE,DATA,%lu,%s,%ld,%ld\n",
                static_cast<unsigned long>(now),
                name,
                sensor.rawValue(),
                measurement.zeroedRaw());
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.printf(
        "\n%s v%u.%u.%u %s - Measurement Node\n",
        Version::PROJECT_NAME,
        Version::MAJOR,
        Version::MINOR,
        Version::PATCH,
        Version::STATUS);

    if (!instrumentStorage.begin()) {
        Serial.println("WARNING: measurement-node storage unavailable");
    }

    leftSensor.begin();
    rightSensor.begin();
    calibrationController.begin();

    Wire.onReceive(receiveCommand);
    Wire.onRequest(sendStatus);
    if (!Wire.begin(
            ArrowLabProtocol::I2C_ADDRESS,
            I2C_SDA_PIN,
            I2C_SCL_PIN,
            I2C_FREQUENCY_HZ
        )) {
        Serial.println("ERROR: measurement-node I2C slave failed to start");
    }

    refreshStatus(millis());
    Serial.printf(
        "AL_NODE,CONFIG,I2C,ADDR=0x%02X,SDA=%u,SCL=%u\n",
        ArrowLabProtocol::I2C_ADDRESS,
        I2C_SDA_PIN,
        I2C_SCL_PIN);
    Serial.printf(
        "AL_NODE,CONFIG,LEFT,DT=%u,SCK=%u\n",
        LEFT_DT_PIN,
        LEFT_SCK_PIN);
    Serial.printf(
        "AL_NODE,CONFIG,RIGHT,DT=%u,SCK=%u\n",
        RIGHT_DT_PIN,
        RIGHT_SCK_PIN);
}

void loop()
{
    const uint32_t now = millis();
    processSerialCommands();
    processPendingCommand();

    if (now - lastSensorUpdate >= SENSOR_INTERVAL_MS) {
        lastSensorUpdate = now;
        sampleChannel(
            leftSensor,
            leftMeasurement,
            CalibrationSide::Left,
            "LEFT",
            now);
        sampleChannel(
            rightSensor,
            rightMeasurement,
            CalibrationSide::Right,
            "RIGHT",
            now);
        calibrationController.update(now);
        refreshStatus(now);
    }

    delay(1);
}
