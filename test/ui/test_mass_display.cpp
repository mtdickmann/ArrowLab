#include "ui/MassDisplay.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    char primary[24];
    char secondary[64];

    MassDisplay::formatPrimary(
        primary,
        sizeof(primary),
        45.3f,
        ArrowLabConfig::MassUnit::Grams);
    assert(std::strcmp(primary, "45.30") == 0);

    MassDisplay::formatPrimary(
        primary,
        sizeof(primary),
        45.3f,
        ArrowLabConfig::MassUnit::Grains);
    assert(std::strcmp(primary, "699") == 0);

    MassDisplay::formatSecondary(
        secondary,
        sizeof(secondary),
        45.3f,
        ArrowLabConfig::MassUnit::Grams);
    assert(std::strcmp(secondary, "699 gr   1.598 oz") == 0);

    assert(
        MassDisplay::nextUnit(ArrowLabConfig::MassUnit::Grams)
        == ArrowLabConfig::MassUnit::Grains);
    assert(
        MassDisplay::nextUnit(ArrowLabConfig::MassUnit::Grains)
        == ArrowLabConfig::MassUnit::Ounces);
    assert(
        MassDisplay::nextUnit(ArrowLabConfig::MassUnit::Ounces)
        == ArrowLabConfig::MassUnit::Grams);

    std::cout << "Mass display tests passed\n";
    return 0;
}
