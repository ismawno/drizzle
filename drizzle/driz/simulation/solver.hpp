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
    void ComputeDensitiesAndDistances(f32 p_DeltaTime);
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
    fvec2 getPressureFromDensity(const Density &p_Density) const;

    void encase(u32 p_Index);

    void mergeDensityAndDistanceArrays();
    void mergeAccelerationArrays();

    f32 getInfluence(f32 p_Distance) const;
    f32 getInfluenceSlope(f32 p_Distance) const;

    f32 getNearInfluence(f32 p_Distance) const;
    f32 getNearInfluenceSlope(f32 p_Distance) const;

    f32 getViscosityInfluence(f32 p_Distance) const;

    void resizeState(u32 p_Size);

    TKit::Array<SimArray<fvec<D>>, DRIZ_MAX_THREADS> m_Accelerations;
    TKit::Array<SimArray<Density>, DRIZ_MAX_THREADS> m_Densities;
    TKit::Array<SimArray<f32>, DRIZ_MAX_THREADS> m_NeighborDistances;
    TKit::Array<SimArray<u32>, DRIZ_MAX_THREADS> m_NeighborCounts;
};
} // namespace Driz
