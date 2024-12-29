#pragma once

#include "flu/core/glm.hpp"
#include "onyx/rendering/render_context.hpp"

namespace Flu
{
using ivec2 = glm::ivec2;
template <Dimension D> struct Particle
{
    fvec<D> Position{0.f};
    fvec<D> Velocity{0.f};
};

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
        for (u32 i = 0; i < m_Particles.size(); ++i)
        {
            const f32 distance = glm::distance(p_Point, m_Particles[i].Position);
            if (distance < SmoothingRadius)
                std::forward<F>(p_Function)(i, distance);
        }
    }

    void AddParticle(const fvec<D> &p_Position) noexcept;

    void Encase() noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    const DynamicArray<Particle<D>> &GetParticles() const noexcept;

    f32 ParticleRadius = 0.3f;
    f32 ParticleMass = 1.f;
    f32 TargetDensity = 0.01f;
    f32 PressureStiffness = 1.f;
    f32 SmoothingRadius = 6.f;
    f32 FastSpeed = 35.f;
    f32 Gravity = -10.f;
    f32 EncaseFriction = 0.2f;

  private:
    struct IndexTuple
    {
        u32 ParticleIndex;
        u32 CellIndex;
        u32 StartIndex;
    };
    f32 getInfluence(f32 p_Distance) const noexcept;
    f32 getInfluenceSlope(f32 p_Distance) const noexcept;

    ivec<D> getCellPosition(const fvec<D> &p_Position) const noexcept;

    DynamicArray<Particle<D>> m_Particles;
    DynamicArray<f32> m_Densities;
    DynamicArray<IndexTuple> m_SpatialLookup;

    struct
    {
        fvec<D> Min{-15.f};
        fvec<D> Max{15.f};
    } m_BoundingBox;
};
} // namespace Flu