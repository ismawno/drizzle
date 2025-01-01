#pragma once

#include "flu/solver/kernel.hpp"
#include "flu/core/glm.hpp"
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

    f32 TargetDensity = 1.f;
    f32 PressureStiffness = 100.f;
    f32 SmoothingRadius = 2.f;

    f32 FastSpeed = 35.f;
    f32 Gravity = 0.f;
    f32 EncaseFriction = 0.2f;

    f32 MouseRadius = 3.f;
    f32 MouseForce = 100.f;

    std::array<Onyx::Color, 3> Gradient = {Onyx::Color::CYAN, Onyx::Color::YELLOW, Onyx::Color::RED};

    NeighborSearch SearchMethod = NeighborSearch::Grid;
    KernelType KernelType = KernelType::Spiky;
};

template <Dimension D> class Solver
{
  public:
    void BeginStep(f32 p_DeltaTime) noexcept;
    void EndStep(f32 p_DeltaTime) noexcept;
    void ApplyMouseForce(const fvec<D> &p_MousePos, f32 p_Timestep) noexcept;

    void UpdateGrid() noexcept;

    f32 ComputeDensityAtPoint(const fvec<D> &p_Point) const noexcept;
    fvec<D> ComputePressureGradient(u32 p_Index) const noexcept;

    f32 GetPressureFromDensity(f32 p_Density) const noexcept;

    template <typename F>
    void ForEachParticleWithinSmoothingRadius(const fvec<D> &p_Point, F &&p_Function) const noexcept
    {
        if (Settings.SearchMethod == NeighborSearch::BruteForce)
            forEachBruteForce(p_Point, std::forward<F>(p_Function));
        else
            forEachGrid(p_Point, std::forward<F>(p_Function));
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
    struct IndexPair
    {
        u32 ParticleIndex;
        u32 CellIndex;
    };
    template <typename F> void forEachBruteForce(const fvec<D> &p_Point, F &&p_Function) const noexcept
    {
        for (u32 i = 0; i < Positions.size(); ++i)
        {
            const f32 distance = glm::distance(p_Point, Positions[i]);
            if (distance < Settings.SmoothingRadius)
                std::forward<F>(p_Function)(i, distance);
        }
    }
    template <typename F> void forEachGrid(const fvec<D> &p_Point, F &&p_Function) const noexcept
    {
        if (Positions.empty())
            return;
        const auto processParticle = [this, &p_Point](const ivec<D> &p_Offset, F &&p_Function) {
            const ivec<D> cellPosition = getCellPosition(p_Point) + p_Offset;
            const u32 cellIndex = getCellIndex(cellPosition);
            const u32 startIndex = m_StartIndices[cellIndex];
            for (u32 i = startIndex; i < m_SpatialLookup.size() && m_SpatialLookup[i].CellIndex == cellIndex; ++i)
            {
                const u32 particleIndex = m_SpatialLookup[i].ParticleIndex;
                const f32 distance = glm::distance(p_Point, Positions[particleIndex]);
                if (distance < Settings.SmoothingRadius)
                    std::forward<F>(p_Function)(particleIndex, distance);
            }
        };

        for (i32 offsetX = -1; offsetX <= 1; ++offsetX)
            for (i32 offsetY = -1; offsetY <= 1; ++offsetY)
                if constexpr (D == D2)
                {
                    const ivec<D> offset = {offsetX, offsetY};
                    processParticle(offset, std::forward<F>(p_Function));
                }
                else
                    for (i32 offsetZ = -1; offsetZ <= 1; ++offsetZ)
                    {
                        const ivec<D> offset = {offsetX, offsetY, offsetZ};
                        processParticle(offset, std::forward<F>(p_Function));
                    }
    }

    void encase(usize p_Index) noexcept;
    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    ivec<D> getCellPosition(const fvec<D> &p_Position) const noexcept;
    u32 getCellIndex(const ivec<D> &p_CellPosition) const noexcept;

    DynamicArray<fvec<D>> m_PredictedPositions;

    DynamicArray<f32> m_Densities;
    DynamicArray<IndexPair> m_SpatialLookup;
    DynamicArray<u32> m_StartIndices;
};
} // namespace Flu