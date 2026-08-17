#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ArrowLabProtocol
{
    constexpr uint8_t VERSION = 6;
    constexpr uint16_t STATUS_MAGIC = 0x5341;  // "AS"
    constexpr uint16_t COMMAND_MAGIC = 0x4341; // "AC"

    enum class Side : uint8_t
    {
        Left = 0,
        Right = 1
    };

    enum class CommandType : uint8_t
    {
        None = 0,
        Tare = 1,
        PrepareCalibration = 2,
        StartCalibration = 3,
        PollStatus = 4,
        StartSpineTest = 5,
        CancelSpineTest = 6,
        ConfirmSpineZero = 7,
        RestartSpineAttempt = 8,
        ConfirmSpineClear = 9,
        TestWifiCredentials = 10,
        CommitWifiCredentials = 11,
        RevertWifiCredentials = 12
    };

    enum class CalibrationStage : uint8_t
    {
        NeedsTare = 0,
        Taring = 1,
        Ready = 2,
        AwaitingLoad = 3,
        ReadyToCalibrate = 4,
        Settling = 5,
        Sampling = 6
    };

    enum ChannelFlags : uint8_t
    {
        ChannelLive = 1U << 0,
        TareComplete = 1U << 1,
        UserTareConfirmed = 1U << 2,
        Calibrated = 1U << 3
    };

    enum class SpineStage : uint8_t
    {
        Idle = 0,
        AwaitingClear = 1,
        TaringLeft = 2,
        TaringRight = 3,
        AwaitingArrow = 4,
        StabilizingArrow = 5,
        AwaitingPlungerZero = 6,
        ReadyToPress = 7,
        Holding = 8,
        AwaitingRetryRelease = 9,
        AwaitingRelease = 10,
        Complete = 11,
        Fault = 12
    };

#pragma pack(push, 1)
    struct ChannelStatus
    {
        int32_t rawCount = 0;
        int32_t heldRawCounts = 0;
        int32_t heldMilliGrams = 0;
        int32_t referenceMilliGrams = 0;
        uint16_t settleRemainingSeconds = 0;
        uint8_t stage = 0;
        uint8_t settlePercent = 0;
        uint8_t flags = 0;
    };

    enum NetworkFlags : uint8_t
    {
        NetworkConfigured = 1U << 0,
        NetworkConnected = 1U << 1,
        NetworkCredentialsSaved = 1U << 2,
        NetworkCredentialTesting = 1U << 3,
        NetworkCredentialTestSucceeded = 1U << 4,
        NetworkCredentialTestFailed = 1U << 5
    };

    struct NetworkStatus
    {
        char hostname[24] = {};
        char ssid[33] = {};
        char ip[16] = {};
        char mac[18] = {};
        int16_t rssiDbm = -127;
        uint8_t flags = 0;
    };

    struct StatusPacket
    {
        uint16_t magic = STATUS_MAGIC;
        uint8_t version = VERSION;
        uint8_t packetSize = sizeof(StatusPacket);
        uint16_t sequence = 0;
        uint16_t lastCommandSequence = 0;
        ChannelStatus left;
        ChannelStatus right;
        int32_t arrowMilliGrams = 0;
        int32_t appliedMilliGrams = 0;
        int32_t positionMilliGrams[4] = {};
        uint8_t spineStage = 0;
        uint8_t spinePositionCount = 0;
        uint8_t spineCurrentPosition = 0;
        uint8_t spineHoldPercent = 0;
        NetworkStatus network;
        uint8_t checksum = 0;
    };

    struct CommandPacket
    {
        uint16_t magic = COMMAND_MAGIC;
        uint8_t version = VERSION;
        uint8_t packetSize = sizeof(CommandPacket);
        uint16_t sequence = 0;
        uint8_t command = 0;
        uint8_t side = 0;
        int32_t referenceMilliGrams = 0;
        char wifiSsid[33] = {};
        char wifiPassword[65] = {};
        uint8_t checksum = 0;
    };
#pragma pack(pop)

    static_assert(sizeof(ChannelStatus) == 21, "Unexpected channel packet padding");
    static_assert(sizeof(NetworkStatus) == 94, "Unexpected network packet padding");
    static_assert(sizeof(StatusPacket) == 173, "Unexpected status packet size");
    static_assert(sizeof(CommandPacket) == 111, "Unexpected command packet size");

    inline uint8_t checksum(const void *data, size_t length)
    {
        const auto *bytes = static_cast<const uint8_t *>(data);
        uint8_t value = 0xA5;
        for (size_t index = 0; index < length; ++index) {
            value ^= bytes[index];
        }
        return value;
    }

    template <typename Packet>
    inline void seal(Packet &packet)
    {
        packet.checksum = 0;
        packet.checksum = checksum(&packet, sizeof(Packet));
    }

    template <typename Packet>
    inline bool checksumValid(const Packet &packet)
    {
        Packet copy;
        std::memcpy(&copy, &packet, sizeof(Packet));
        const uint8_t received = copy.checksum;
        copy.checksum = 0;
        return received == checksum(&copy, sizeof(Packet));
    }

    inline bool valid(const StatusPacket &packet)
    {
        return packet.magic == STATUS_MAGIC
            && packet.version == VERSION
            && packet.packetSize == sizeof(StatusPacket)
            && checksumValid(packet);
    }

    inline bool valid(const CommandPacket &packet)
    {
        return packet.magic == COMMAND_MAGIC
            && packet.version == VERSION
            && packet.packetSize == sizeof(CommandPacket)
            && packet.command
                <= static_cast<uint8_t>(CommandType::RevertWifiCredentials)
            && packet.side <= static_cast<uint8_t>(Side::Right)
            && checksumValid(packet);
    }
}
