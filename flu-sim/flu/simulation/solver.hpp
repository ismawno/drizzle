#pragma once

#include "flu/simulation/kernel.hpp"
#include "flu/simulation/lookup.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/utilities/math.hpp"

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
    f32 ParticleRadius = 0.1f;
    f32 ParticleMass = 1.f;

    f32 TargetDensity = 10.f;
    f32 PressureStiffness = 100.f;
    f32 NearPressureStiffness = 25.f;
    f32 SmoothingRadius = 1.f;

    f32 FastSpeed = 35.f;
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

using Density = fvec2;

template <Dimension D> struct SimulationData
{
    SimArray<fvec<D>> Positions;
    SimArray<fvec<D>> Velocities;
    SimArray<fvec<D>> Accelerations;
    SimArray<fvec<D>> StagedPositions;

    SimArray<Density> Densities; // Density and Near Density
};

template <Dimension D> class Solver
{
  public:
    void BeginStep(f32 p_DeltaTime) noexcept;
    void EndStep() noexcept;

    void AddMouseForce(const fvec<D> &p_MousePos) noexcept;
    void AddPressureAndViscosity() noexcept;
    void ComputeDensities() noexcept;
    void ApplyComputedForces(f32 p_DeltaTime) noexcept;

    const SimulationData<D> &GetData() const noexcept;

    u32 GetParticleCount() const noexcept;
    const Lookup<D> &GetLookup() const noexcept;

    fvec2 GetPressureFromDensity(const Density &p_Density) const noexcept;

    void UpdateLookup() noexcept;
    void UpdateAllLookups() noexcept;

    // This is hilarious
    template <typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8>
    void ForEachWithinSmoothingRadius(F1 &&p_BruteForcePairWiseST, F2 &&p_BruteForcePairWiseMT, F3 &&p_GridPairWiseST,
                                      F4 &&p_GridPairWiseMT, F5 &&p_BruteForceParticleWiseST,
                                      F6 &&p_BruteForceParticleWiseMT, F7 &&p_GridParticleWiseST,
                                      F8 &&p_GridParticleWiseMT) const noexcept
    {
        switch (Settings.IterationMode)
        {
        case ParticleIterationMode::PairWise:
            switch (Settings.LookupMode)
            {
            case ParticleLookupMode::BruteForceSingleThread:
                std::forward<F1>(p_BruteForcePairWiseST)();
                return;
            case ParticleLookupMode::BruteForceMultiThread:
                std::forward<F2>(p_BruteForcePairWiseMT)();
                return;
            case ParticleLookupMode::GridSingleThread:
                std::forward<F3>(p_GridPairWiseST)();
                return;
            case ParticleLookupMode::GridMultiThread:
                std::forward<F4>(p_GridPairWiseMT)();
                return;
            }
            return;
        case ParticleIterationMode::ParticleWise:
            switch (Settings.LookupMode)
            {
            case ParticleLookupMode::BruteForceSingleThread:
                std::forward<F5>(p_BruteForceParticleWiseST)();
                return;
            case ParticleLookupMode::BruteForceMultiThread:
                std::forward<F6>(p_BruteForceParticleWiseMT)();
                return;
            case ParticleLookupMode::GridSingleThread:
                std::forward<F7>(p_GridParticleWiseST)();
                return;
            case ParticleLookupMode::GridMultiThread:
                std::forward<F8>(p_GridParticleWiseMT)();
                return;
            }
            return;
        }
    }

    template <typename F> void ForEachPairWithinSmoothingRadius(F &&p_Function) const noexcept
    {
        switch (Settings.LookupMode)
        {
        case ParticleLookupMode::BruteForceSingleThread:
            m_Lookup.ForEachPairBruteForceST(std::forward<F>(p_Function));
            return;
        case ParticleLookupMode::BruteForceMultiThread:
            m_Lookup.ForEachPairBruteForceMT(std::forward<F>(p_Function));
            return;
        case ParticleLookupMode::GridSingleThread:
            m_Lookup.ForEachPairGridST(std::forward<F>(p_Function));
            return;
        case ParticleLookupMode::GridMultiThread:
            m_Lookup.ForEachPairGridMT(std::forward<F>(p_Function));
            return;
        }
    }

    template <typename F> void ForEachParticleWithinSmoothingRadius(const u32 p_Index, F &&p_Function) const noexcept
    {
        switch (Settings.LookupMode)
        {
        case ParticleLookupMode::BruteForceSingleThread:
        case ParticleLookupMode::BruteForceMultiThread:
            m_Lookup.ForEachParticleBruteForce(p_Index, std::forward<F>(p_Function));
            return;
        case ParticleLookupMode::GridSingleThread:
        case ParticleLookupMode::GridMultiThread:
            m_Lookup.ForEachParticleGrid(p_Index, std::forward<F>(p_Function));
            return;
        }
    }

    void AddParticle(const fvec<D> &p_Position) noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    SimulationSettings Settings{};

    struct
    {
        fvec<D> Min{-20.f};
        fvec<D> Max{20.f};
    } BoundingBox;

  private:
    void encase(u32 p_Index) noexcept;

    void mergeDensityArrays() noexcept;
    void mergeAccelerationArrays() noexcept;

    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getNearInfluence(f32 p_Distance) const noexcept;
    f32 getNearInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getViscosityInfluence(f32 p_Distance) const noexcept;

    fvec<D> computePairwisePressureGradient(u32 p_Index1, u32 p_Index2, f32 p_Distance) const noexcept;
    fvec<D> computePairwiseViscosityTerm(u32 p_Index1, u32 p_Index2, f32 p_Distance) const noexcept;

    Lookup<D> m_Lookup;
    SimulationData<D> m_Data;

    TKit::Array<SimArray<fvec<D>>, FLU_THREAD_COUNT> m_ThreadAccelerations;
    TKit::Array<SimArray<Density>, FLU_THREAD_COUNT> m_ThreadDensities;
};
} // namespace Flu