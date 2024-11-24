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

    static inline f32 Radius = 0.3f;
    static inline f32 FastSpeed = 35.f;

    void Draw(ONYX::RenderContext<D> *p_Context) const noexcept;
};
} // namespace FLU