#pragma once

#include "flu/simulation/kernel.hpp"
#include "flu/core/glm.hpp"
#include "flu/core/core.hpp"
#include "onyx/draw/color.hpp"
#include "tkit/container/array.hpp"
#include "tkit/reflection/reflect.hpp"

namespace Flu
{
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
    SimArray<fvec<D>> Positions;
    SimArray<fvec<D>> Velocities;

    fvec<D> Min{-30.f};
    fvec<D> Max{30.f};
};

using Density = fvec2;

template <Dimension D> struct SimulationData
{
    SimulationState<D> State;
    SimArray<fvec<D>> Accelerations;
    SimArray<fvec<D>> StagedPositions;

    SimArray<Density> Densities; // Density and Near Density
};
} // namespace Flu