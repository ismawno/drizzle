#pragma once

#include "driz/simulation/settings.hpp"
#include "driz/simulation/lookup.hpp"
#include "onyx/rendering/render_context.hpp"

namespace Driz
{
template <Dimension D> class Solver
{
  public:
    Solver(const SimulationSettings &p_Settings, const SimulationState<D> &p_State) noexcept;

    void BeginStep(f32 p_DeltaTime) noexcept;
    void EndStep() noexcept;

    void AddMouseForce(const fvec<D> &p_MousePos) noexcept;
    void AddPressureAndViscosity() noexcept;
    void ComputeDensities() noexcept;
    void ApplyComputedForces(f32 p_DeltaTime) noexcept;

    u32 GetParticleCount() const noexcept;

    void UpdateLookup() noexcept;
    void UpdateAllLookups() noexcept;

    void AddParticle(const fvec<D> &p_Position) noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    LookupMethod<D> Lookup;
    SimulationData<D> Data;
    SimulationSettings Settings;

  private:
    // This is hilarious
    template <typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8>
    void forEachWithinSmoothingRadius(F1 &&p_BruteForcePairWiseST, F2 &&p_BruteForcePairWiseMT, F3 &&p_GridPairWiseST,
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

    fvec2 getPressureFromDensity(const Density &p_Density) const noexcept;

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

    TKit::Array<SimArray<fvec<D>>, TKIT_THREAD_POOL_MAX_THREADS> m_ThreadAccelerations;
    TKit::Array<SimArray<Density>, TKIT_THREAD_POOL_MAX_THREADS> m_ThreadDensities;
};
} // namespace Driz