#pragma once

#include "driz/simulation/settings.hpp"
#include "driz/simulation/lookup.hpp"
#include "onyx/rendering/render_context.hpp"

namespace Driz
{
template <Dimension D> class Solver
{
  public:
    Solver(const SimulationSettings &p_Settings, const SimulationState<D> &p_State);

    void BeginStep(f32 p_DeltaTime);
    void EndStep();

    void AddMouseForce(const fvec<D> &p_MousePos);
    void AddPressureAndViscosity();
    void ComputeDensities();
    void ApplyComputedForces(f32 p_DeltaTime);

    u32 GetParticleCount() const;

    void UpdateLookup();
    void UpdateAllLookups();

    void AddParticle(const fvec<D> &p_Position);

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const;

    LookupMethod<D> Lookup;
    SimulationData<D> Data;
    SimulationSettings Settings;

  private:
    // This is hilarious
    template <typename F1, typename F2, typename F3, typename F4, typename F5, typename F6, typename F7, typename F8>
    void forEachWithinSmoothingRadius(F1 &&p_BruteForcePairWiseST, F2 &&p_BruteForcePairWiseMT, F3 &&p_GridPairWiseST,
                                      F4 &&p_GridPairWiseMT, F5 &&p_BruteForceParticleWiseST,
                                      F6 &&p_BruteForceParticleWiseMT, F7 &&p_GridParticleWiseST,
                                      F8 &&p_GridParticleWiseMT) const
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

    fvec2 getPressureFromDensity(const Density &p_Density) const;

    void encase(u32 p_Index);

    void mergeDensityArrays();
    void mergeAccelerationArrays();

    f32 getInfluence(f32 p_Distance) const;
    f32 getInfluenceSlope(f32 p_Distance) const;

    f32 getNearInfluence(f32 p_Distance) const;
    f32 getNearInfluenceSlope(f32 p_Distance) const;

    f32 getViscosityInfluence(f32 p_Distance) const;

    fvec<D> computePairwisePressureGradient(u32 p_Index1, u32 p_Index2, f32 p_Distance) const;
    fvec<D> computePairwiseViscosityTerm(u32 p_Index1, u32 p_Index2, f32 p_Distance) const;

    TKit::Array<SimArray<fvec<D>>, DRIZ_MAX_THREADS> m_ThreadAccelerations;
    TKit::Array<SimArray<Density>, DRIZ_MAX_THREADS> m_ThreadDensities;
};
} // namespace Driz
