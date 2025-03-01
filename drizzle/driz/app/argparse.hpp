#pragma once

#include "driz/simulation/settings.hpp"
#include <optional>

namespace Driz
{
struct ParseResult
{
    SimulationSettings Settings;
    std::optional<SimulationState<D2>> State2;
    std::optional<SimulationState<D3>> State3;

    Dimension Dim;
    f32 RunTime;
    bool Intro;
    bool HasRunTime;
};

const ParseResult *ParseArgs(int argc, char **argv);
} // namespace Driz