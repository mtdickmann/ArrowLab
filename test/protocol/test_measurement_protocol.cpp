#include "protocol/MeasurementProtocol.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    ArrowLabProtocol::StatusPacket status;
    status.sequence = 42;
    status.left.rawCount = -123456;
    status.left.heldMilliGrams = 999800;
    status.left.flags = ArrowLabProtocol::ChannelLive
        | ArrowLabProtocol::Calibrated;
    ArrowLabProtocol::seal(status);
    assert(ArrowLabProtocol::valid(status));

    ArrowLabProtocol::StatusPacket corrupted = status;
    corrupted.left.rawCount += 1;
    assert(!ArrowLabProtocol::valid(corrupted));

    ArrowLabProtocol::CommandPacket command;
    command.sequence = 7;
    command.command = static_cast<uint8_t>(
        ArrowLabProtocol::CommandType::PrepareCalibration);
    command.side = static_cast<uint8_t>(ArrowLabProtocol::Side::Right);
    command.referenceMilliGrams = 999800;
    ArrowLabProtocol::seal(command);
    assert(ArrowLabProtocol::valid(command));

    command.side = 3;
    ArrowLabProtocol::seal(command);
    assert(!ArrowLabProtocol::valid(command));

    std::cout << "Measurement protocol tests passed\n";
    return 0;
}
