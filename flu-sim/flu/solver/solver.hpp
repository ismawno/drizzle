#pragma once

#include "flu/core/dimension.hpp"
#include "flu/core/glm.hpp"
#include "flu/core/alias.hpp"
#include "onyx/rendering/render_context.hpp"

namespace Flu
{

template <Dimension D> struct Particle
{
    vec<D> Position{0.f};
    vec<D> Velocity{0.f};
};

template <Dimension D> struct Solver
{
    void Step(f32 p_DeltaTime) noexcept;

    f32 ComputeDensityAtPoint(const vec<D> &p_Point) const noexcept;
    vec<D> ComputePressureGradient(usize p_Index) const noexcept;

    f32 GetPressureFromDensity(f32 p_Density) const noexcept;

    void AddParticle(const vec<D> &p_Position) noexcept;

    void Encase() noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    DynamicArray<Particle<D>> Particles;
    DynamicArray<f32> Densities;
    f32 ParticleRadius = 0.3f;
    f32 ParticleMass = 1.f;
    f32 TargetDensity = 0.01f;
    f32 PressureStiffness = 1.f;
    f32 SmoothingRadius = 6.f;
    f32 FastSpeed = 35.f;
    f32 Gravity = -10.f;
    f32 EncaseFriction = 0.2f;

    struct
    {
        vec<D> Min{-15.f};
        vec<D> Max{15.f};
    } BoundingBox;
};
} // namespace Flu