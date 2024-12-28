#pragma once

#include "flu/core/dimension.hpp"
#include "flu/core/glm.hpp"
#include "flu/core/alias.hpp"
#include "onyx/rendering/render_context.hpp"

namespace FLU
{
template <Dimension D> struct Particle
{
    vec<D> Position{0.f};
    vec<D> Velocity{0.f};
};

template <Dimension D> struct Solver
{
    void Step(f32 p_DeltaTime) noexcept;
    void Encase() noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;
    void DrawParticles(Onyx::RenderContext<D> *p_Context) const noexcept;

    DynamicArray<Particle<D>> Particles;
    f32 Radius = 0.3f;
    f32 FastSpeed = 35.f;
    f32 Gravity = -10.f;
    f32 EncaseFriction = 0.2f;

    struct
    {
        vec<D> Min{-15.f};
        vec<D> Max{15.f};
    } BoundingBox;
};
} // namespace FLU