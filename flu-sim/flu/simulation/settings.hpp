#pragma once

#include "flu/simulation/kernel.hpp"
#include "flu/core/glm.hpp"
#include "flu/core/core.hpp"
#include "onyx/draw/color.hpp"
#include "onyx/serialization/color.hpp"
#include "tkit/container/array.hpp"
#include "tkit/reflection/reflect.hpp"
#include "tkit/serialization/yaml/container.hpp"

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

    TKit::Array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};

    ParticleLookupMode LookupMode = ParticleLookupMode::GridMultiThread;
    ParticleIterationMode IterationMode = ParticleIterationMode::PairWise;

    KernelType KType = KernelType::Spiky3;
    KernelType NearKType = KernelType::Spiky5;

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

template <typename T>
void Serialize(const std::string &p_DirectoryName, const std::string &p_FileName, const T &p_Instance) noexcept
{
    const std::string directory = FLU_ROOT_PATH "/saves/" + p_DirectoryName;
    std::filesystem::create_directories(directory);
    const std::string path = directory + "/" + p_FileName;

    TKit::Yaml::Serialize(path, p_Instance);
}
template <typename T> bool Deserialize(const std::string &p_RelativePath, T &p_Instance) noexcept
{
    const std::string path = FLU_ROOT_PATH "/saves/" + p_RelativePath;
    if (!std::filesystem::exists(path))
        return false;

    p_Instance = TKit::Yaml::Deserialize<T>(path);
    return true;
}

} // namespace Flu