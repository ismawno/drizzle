#pragma once

#include "flu/core/glm.hpp"
#include "onyx/rendering/render_context.hpp"

namespace Flu
{
template <Dimension D> class Solver
{
  public:
    void Step(f32 p_DeltaTime) noexcept;
    void UpdateGrid() noexcept;

    f32 ComputeDensityAtPoint(const fvec<D> &p_Point) const noexcept;
    fvec<D> ComputePressureGradient(u32 p_Index) const noexcept;

    f32 GetPressureFromDensity(f32 p_Density) const noexcept;

    template <typename F> void ForEachPointWithinSmoothingRadius(const fvec<D> &p_Point, F &&p_Function) const noexcept
    {
        // for (u32 i = 0; i < m_Particles.size(); ++i)
        // {
        //     const f32 distance = glm::distance(p_Point, m_Particles[i].Position);
        //     if (distance < SmoothingRadius)
        //         std::forward<F>(p_Function)(i, distance);
        // }
        if (m_Positions.empty())
            return;
        if constexpr (D == D2)
            for (i32 offsetX = -1; offsetX <= 1; ++offsetX)
                for (i32 offsetY = -1; offsetY <= 1; ++offsetY)
                {
                    const ivec<D> offset = {offsetX, offsetY};
                    const ivec<D> cellPosition = getCellPosition(p_Point) + offset;
                    const u32 cellIndex = getCellIndex(cellPosition);
                    const u32 startIndex = m_StartIndices[cellIndex];
                    for (u32 i = startIndex; i < m_SpatialLookup.size() && m_SpatialLookup[i].CellIndex == cellIndex;
                         ++i)
                    {
                        const u32 particleIndex = m_SpatialLookup[i].ParticleIndex;
                        const f32 distance = glm::distance(p_Point, m_Positions[particleIndex]);
                        if (distance < SmoothingRadius)
                            std::forward<F>(p_Function)(particleIndex, distance);
                    }
                }
    }

    void AddParticle(const fvec<D> &p_Position) noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    usize GetParticleCount() const noexcept;

    f32 ParticleRadius = 0.1f;
    f32 ParticleMass = 1.f;
    f32 TargetDensity = 1.f;
    f32 PressureStiffness = 100.f;
    f32 SmoothingRadius = 2.f;
    f32 FastSpeed = 35.f;
    f32 Gravity = 0.f;
    f32 EncaseFriction = 0.2f;

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
    void encase(usize p_Index) noexcept;
    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    ivec<D> getCellPosition(const fvec<D> &p_Position) const noexcept;
    u32 getCellIndex(const ivec<D> &p_CellPosition) const noexcept;

    DynamicArray<fvec<D>> m_PredictedPositions;
    DynamicArray<fvec<D>> m_Positions;
    DynamicArray<fvec<D>> m_Velocities;

    DynamicArray<f32> m_Densities;
    DynamicArray<IndexPair> m_SpatialLookup;
    DynamicArray<u32> m_StartIndices;
};
} // namespace Flu