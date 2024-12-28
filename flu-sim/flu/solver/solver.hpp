#pragma once

#include "flu/solver/particle.hpp"

namespace FLU
{
template <Dimension D> struct Solver
{
    void Step(f32 p_DeltaTime) noexcept;
    void Encase() noexcept;

    void DrawBoundingBox(Onyx::RenderContext<D> *p_Context) const noexcept;

    DynamicArray<Particle<D>> Particles;
    struct
    {
        vec<D> Min{-15.f};
        vec<D> Max{15.f};
    } BoundingBox;

    static inline f32 Gravity = -10.f;
    static inline f32 EncaseFriction = 0.2f;
};
} // namespace FLU