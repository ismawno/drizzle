#pragma once

#include "flu/simulation/settings.hpp"
#include <optional>

namespace Flu
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

ParseResult ParseArgs(int argc, char **argv) noexcept;
} // namespace Flu