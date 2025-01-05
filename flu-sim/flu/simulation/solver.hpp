#pragma once

#include "flu/simulation/kernel.hpp"
#include "flu/simulation/lookup.hpp"
#include "onyx/rendering/render_context.hpp"

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

    std::array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};

    NeighborSearch SearchMethod = NeighborSearch::Grid;
    KernelType KType = KernelType::Spiky3;
    KernelType NearKType = KernelType::Spiky5;
};

template <Dimension D> class Solver
{
    TKIT_NON_COPYABLE(Solver)
  public:
    Solver() noexcept = default;

    void BeginStep(f32 p_DeltaTime) noexcept;
    void EndStep(f32 p_DeltaTime) noexcept;

    void ApplyMouseForce(const fvec<D> &p_MousePos, f32 p_Timestep) noexcept;

    std::pair<f32, f32> ComputeParticleDensity(const u32 p_Index) const noexcept;

    fvec<D> ComputeViscosityTerm(u32 p_Index) const noexcept;
    fvec<D> ComputePressureGradient(u32 p_Index) const noexcept;

    std::pair<f32, f32> GetPressureFromDensity(f32 p_Density, f32 p_NearDensity) const noexcept;

    void UpdateLookup() noexcept;
    template <typename F> void ForEachParticleWithinSmoothingRadius(const u32 p_Index, F &&p_Function) const noexcept
    {
        switch (Settings.SearchMethod)
        {
        case NeighborSearch::BruteForce:
            m_Lookup.ForEachParticleBruteForce(p_Index, std::forward<F>(p_Function));
            break;
        case NeighborSearch::Grid:
            m_Lookup.ForEachParticleGrid(p_Index, std::forward<F>(p_Function));
            break;
        }
    }

    void AddParticle(const fvec<D> &p_Position) noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    SimulationSettings Settings{};

    DynamicArray<fvec<D>> Positions;
    DynamicArray<fvec<D>> Velocities;

    struct
    {
        fvec<D> Min{-15.f};
        fvec<D> Max{15.f};
    } BoundingBox;

  private:
    void encase(usize p_Index) noexcept;

    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getNearInfluence(f32 p_Distance) const noexcept;
    f32 getNearInfluenceSlope(f32 p_Distance) const noexcept;

    f32 getViscosityInfluence(f32 p_Distance) const noexcept;

    Lookup<D> m_Lookup{&Positions};
    DynamicArray<fvec<D>> m_PredictedPositions;

    DynamicArray<f32> m_Densities;
    DynamicArray<f32> m_NearDensities;
};
} // namespace Flu