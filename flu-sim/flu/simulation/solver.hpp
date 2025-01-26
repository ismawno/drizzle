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
    bool IterateOverPairs = true;
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
    template <typename F> void ForEachPairWithinSmoothingRadius(F &&p_Function) const noexcept
    {
        switch (Settings.SearchMethod)
        {
        case NeighborSearch::BruteForce:
            m_Lookup.ForEachPairBruteForce(std::forward<F>(p_Function));
            break;
        case NeighborSearch::Grid:
            m_Lookup.ForEachPairGrid(std::forward<F>(p_Function));
            break;
        }
    }
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

    auto getPairwisePressureGradientComputation() const noexcept
    {
        return [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
            const fvec<D> dir = (m_Data.Positions[p_Index1] - m_Data.Positions[p_Index2]) / p_Distance;

            const f32 kernelGradient = getInfluenceSlope(p_Distance);
            const f32 nearKernelGradient = getNearInfluenceSlope(p_Distance);
            const auto [p1, np1] = GetPressureFromDensity(m_Data.Densities[p_Index1], m_Data.NearDensities[p_Index1]);
            const auto [p2, np2] = GetPressureFromDensity(m_Data.Densities[p_Index2], m_Data.NearDensities[p_Index2]);

            const f32 density = 0.5f * (m_Data.Densities[p_Index1] + m_Data.Densities[p_Index2]);
            const f32 ndensity = 0.5f * (m_Data.NearDensities[p_Index1] + m_Data.NearDensities[p_Index2]);

            const f32 dg1 = 0.5f * (p1 + p2) * kernelGradient / density;
            const f32 dg2 = 0.5f * (np1 + np2) * nearKernelGradient / ndensity;
            return (Settings.ParticleMass * (dg1 + dg2)) * dir;
        };
    }
    auto getPairwiseViscosityTermComputation() const noexcept
    {
        return [this](const u32 p_Index1, const u32 p_Index2, const f32 p_Distance) {
            const fvec<D> diff = m_Data.Velocities[p_Index2] - m_Data.Velocities[p_Index1];
            const f32 kernel = getViscosityInfluence(p_Distance);

            const f32 u = glm::length(diff);
            return ((Settings.ViscLinearTerm + Settings.ViscQuadraticTerm * u) * kernel) * diff;
        };
    }

    Lookup<D> m_Lookup;
    SimulationData<D> m_Data;
};
} // namespace Flu