#pragma once

#include "onyx/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/simulation/solver.hpp"

namespace Flu
{
class IntroLayer final : public Onyx::Layer
{
  public:
    IntroLayer(Onyx::Application *p_Application) noexcept;

  private:
    void OnRender(VkCommandBuffer) noexcept override;
    void OnRemoval() noexcept override;

    void renderIntroSettings() noexcept;

    Onyx::Application *m_Application;
    i32 m_Dim = 0;
    ivec3 m_Dimensions{60, 60, 60};

    Onyx::RenderContext<D2> *m_Context2;
    Onyx::RenderContext<D3> *m_Context3;

    SimulationSettings m_Settings;
};
} // namespace Flu