#pragma once

#include "flu/simulation/kernel.hpp"
#include "flu/simulation/lookup.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/utilities/math.hpp"

namespace Flu
{
enum class NeighborSearch
{
    BruteForce = 0,
    Grid
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

    NeighborSearch SearchMethod = NeighborSearch::Grid;
    KernelType KType = KernelType::Spiky3;
    KernelType NearKType = KernelType::Spiky5;
};

template <Dimension D> struct SimulationData
{
    SimArray<fvec<D>> Positions;
    SimArray<fvec<D>> Velocities;
    SimArray<fvec<D>> Accelerations;
    SimArray<fvec<D>> StagedPositions;

    SimArray<f32> Densities;
    SimArray<f32> NearDensities;
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

    std::pair<f32, f32> GetPressureFromDensity(f32 p_Density, f32 p_NearDensity) const noexcept;

    void UpdateLookup() noexcept;
    template <typename F> void ForEachPairWithinSmoothingRadiusST(F &&p_Function) const noexcept
    {
        switch (Settings.SearchMethod)
        {
        case NeighborSearch::BruteForce:
            m_Lookup.ForEachPairBruteForceST(std::forward<F>(p_Function));
            break;
        case NeighborSearch::Grid:
            m_Lookup.ForEachPairGridST(std::forward<F>(p_Function));
            break;
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

    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getNearInfluence(f32 p_Distance) const noexcept;
    f32 getNearInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getViscosityInfluence(f32 p_Distance) const noexcept;

    fvec<D> computePairwisePressureGradient(u32 p_Index1, u32 p_Index2, f32 p_Distance) const noexcept;
    fvec<D> computePairwiseViscosityTerm(u32 p_Index1, u32 p_Index2, f32 p_Distance) const noexcept;

    Lookup<D> m_Lookup;
    SimulationData<D> m_Data;
};
} // namespace Flu