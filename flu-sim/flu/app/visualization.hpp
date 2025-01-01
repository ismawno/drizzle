#pragma once

#include "flu/core/glm.hpp"
#include "onyx/rendering/render_context.hpp"
#include "tkit/profiling/timespan.hpp"

namespace Flu
{
struct SimulationSettings;
template <Dimension D> struct Visualization
{
  public:
    static void AdjustAndControlCamera(Onyx::RenderContext<D> *p_Context, TKit::Timespan p_DeltaTime) noexcept;

    static void DrawParticle(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Position, const Onyx::Color &p_Color,
                             f32 p_Size) noexcept;
    static void DrawMouseInfluence(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color, f32 p_Size) noexcept;
    static void DrawBoundingBox(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color, const fvec<D> &p_Min,
                                const fvec<D> &p_Max) noexcept;

    static void DrawParticleLattice(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color,
                                    const ivec<D> &p_Dimensions, f32 p_Size) noexcept;

    static void RenderSettings(SimulationSettings &p_Settings) noexcept;
};
} // namespace Flu