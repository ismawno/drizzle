#pragma once

#include "driz/simulation/kernel.hpp"
#include "driz/core/glm.hpp"
#include "driz/core/core.hpp"
#include "onyx/property/color.hpp"
#include "tkit/container/array.hpp"
#include "tkit/reflection/reflect.hpp"

namespace Driz
{
TKIT_YAML_SERIALIZE_DECLARE_ENUM(ParticleIterationMode)
TKIT_YAML_SERIALIZE_DECLARE_ENUM(ParticleLookupMode)
TKIT_REFLECT_DECLARE_ENUM(ParticleIterationMode)
TKIT_REFLECT_DECLARE_ENUM(ParticleLookupMode)
enum class ParticleLookupMode
{
    BruteForceSingleThread = 0,
    BruteForceMultiThread,

    GridSingleThread,
    GridMultiThread
};

enum class ParticleIterationMode
{
    PairWise = 0,
    ParticleWise
};

struct SimulationSettings
{
    TKIT_REFLECT_DECLARE(SimulationSettings)
    TKIT_YAML_SERIALIZE_DECLARE(SimulationSettings)

    TKIT_REFLECT_GROUP_BEGIN("CommandLine")
    f32 ParticleRadius = 0.3f;
    f32 ParticleMass = 1.f;

    f32 TargetDensity = 10.f;
    f32 PressureStiffness = 100.f;
    f32 NearPressureStiffness = 25.f;
    f32 SmoothingRadius = 1.f;

    f32 FastSpeed = 15.f;
    f32 Gravity = -4.f;
    f32 EncaseFriction = 0.8f;

    f32 ViscLinearTerm = 0.06f;
    f32 ViscQuadraticTerm = 0.0f;
    KernelType ViscosityKType = KernelType::Poly6;

    f32 MouseRadius = 6.f;
    f32 MouseForce = -30.f;

    u32 Partitions = 1;

    ParticleLookupMode LookupMode = ParticleLookupMode::GridMultiThread;
    ParticleIterationMode IterationMode = ParticleIterationMode::PairWise;

    KernelType KType = KernelType::Spiky3;
    KernelType NearKType = KernelType::Spiky5;
    TKIT_REFLECT_GROUP_END()

    TKit::Array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};

    bool UsesGrid() const noexcept;
    bool UsesMultiThread() const noexcept;
};

template <Dimension D> struct SimulationState
{
    TKIT_REFLECT_DECLARE(SimulationState)
    TKIT_YAML_SERIALIZE_DECLARE(SimulationState)

    SimArray<fvec<D>> Positions;
    SimArray<fvec<D>> Velocities;

    fvec<D> Min{-30.f + 25.f * (D - 2)};
    fvec<D> Max{30.f - 25.f * (D - 2)};
};

using Density = fvec2;

template <Dimension D> struct ISimulationData
{
    SimulationState<D> State;
    SimArray<fvec<D>> Accelerations;
    SimArray<fvec<D>> StagedPositions;

    SimArray<Density> Densities; // Density and Near Density
};
template <Dimension D> struct SimulationData;

template <> struct SimulationData<D2> : ISimulationData<D2>
{
};

template <> struct SimulationData<D3> : ISimulationData<D3>
{
    SimArray<u8> UnderMouseInfluence;
};

} // namespace Driz
