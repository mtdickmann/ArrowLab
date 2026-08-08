#include "diagnostics/CreepDiagnostic.h"

#include <cassert>
#include <cstdint>
#include <iostream>

extern std::uint32_t stubMillis;

int main()
{
    LoadCellChannel left;
    LoadCellChannel right;
    CreepDiagnostic diagnostic;

    // A raw load run is valid without operational tare, calibration or a
    // previously captured zero-baseline flag.
    assert(diagnostic.start(
        DiagnosticSide::Left,
        1000.0f,
        false,
        stubMillis));

    diagnostic.handleHostCommand("AL_HOST,HELLO,3", stubMillis);
    assert(
        diagnostic.state()
        == CreepDiagnostic::State::CapturingReference);

    left.setRaw(100000);
    for (int index = 0; index < 20; ++index) {
        diagnostic.update(stubMillis, true, false, left, right);
        stubMillis += 100;
    }
    assert(diagnostic.state() == CreepDiagnostic::State::AwaitingLoad);

    left.setRaw(1100000);
    for (int index = 0; index < 5; ++index) {
        diagnostic.update(stubMillis, true, false, left, right);
        stubMillis += 100;
    }
    assert(diagnostic.state() == CreepDiagnostic::State::Running);
    assert(diagnostic.bufferedSampleCount() == 1);

    diagnostic.cancel();
    assert(diagnostic.state() == CreepDiagnostic::State::Idle);

    std::cout << "CreepDiagnostic tests passed\n";
    return 0;
}
