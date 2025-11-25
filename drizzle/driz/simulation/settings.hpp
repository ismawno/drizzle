#pragma once

#include "driz/simulation/kernel.hpp"
#include "driz/core/math.hpp"
#include "driz/core/core.hpp"
#include "onyx/property/color.hpp"
#include "tkit/container/array.hpp"
#include "tkit/reflection/reflect.hpp"

namespace Driz
{
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

    f32 ElasticityStrength = 1.f;

    f32 PlasticAlpha = 2.0f;
    f32 PlasticYield = 0.05f;
    f32 PlasticMaxStep = 0.05f;

    f32 MouseRadius = 6.f;
    f32 MouseForce = -30.f;

    u32 Partitions = 1;

    KernelType KType = KernelType::Spiky3;
    KernelType NearKType = KernelType::Spiky5;
    TKIT_REFLECT_GROUP_END()

    TKit::Array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};
};

template <Dimension D> struct SimulationState
{
    TKIT_REFLECT_DECLARE(SimulationState)
    TKIT_YAML_SERIALIZE_DECLARE(SimulationState)

    SimArray<f32v<D>> Positions;
    SimArray<f32v<D>> Velocities;

    f32v<D> Min{-30.f + 25.f * (D - 2)};
    f32v<D> Max{30.f - 25.f * (D - 2)};
};

using Density = f32v2;

template <Dimension D> struct ISimulationData
{
    SimulationState<D> State;
    SimArray<f32v<D>> Accelerations;
    SimArray<f32v<D>> StagedPositions;

    SimArray<Density> Densities; // Density and Near Density

    SimArray<f32> RestDistances;
    SimArray<f32> NeighborDistances;
    SimArray<u32> NeighborCounts;
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
