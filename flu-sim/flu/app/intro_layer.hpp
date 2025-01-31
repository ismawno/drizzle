#pragma once

#include "onyx/app/user_layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/simulation/solver.hpp"

namespace Flu
{
class IntroLayer final : public Onyx::UserLayer
{
  public:
    IntroLayer(Onyx::Application *p_Application) noexcept;

  private:
    void OnRender(VkCommandBuffer) noexcept override;
    bool OnEvent(const Onyx::Event &p_Event) noexcept override;

    void renderIntroSettings() noexcept;

    Onyx::Application *m_Application;
    i32 m_Dim = 0;
#ifndef TKIT_DEBUG
    ivec3 m_Dimensions{60, 60, 60};
#else
    ivec3 m_Dimensions{20, 20, 20};
#endif
    BoundingBox<D2> m_Bounds2{fvec2{-30.f}, fvec2{30.f}};
    BoundingBox<D3> m_Bounds3{fvec3{-30.f}, fvec3{30.f}};

    Onyx::Window *m_Window;
    Onyx::RenderContext<D2> *m_Context2;
    Onyx::RenderContext<D3> *m_Context3;

    SimulationSettings m_Settings;
};
} // namespace Flu