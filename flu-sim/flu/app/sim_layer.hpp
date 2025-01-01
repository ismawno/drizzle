#pragma once

#include "onyx/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/solver/solver.hpp"

namespace Flu
{
template <Dimension D> class SimLayer final : public Onyx::Layer
{
  public:
    SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
             const ivec<D> &p_StartingLayout) noexcept;

  private:
    void OnUpdate() noexcept override;
    void OnRender(VkCommandBuffer) noexcept override;
    bool OnEvent(const Onyx::Event &p_Event) noexcept override;

    void renderSimulationSettings() noexcept;
    void addParticle() noexcept;

    Onyx::Application *m_Application;
    Onyx::Window *m_Window;

    Solver<D> m_Solver;
    Onyx::RenderContext<D> *m_Context;

    f32 m_Timestep = 1.f / 60.f;
};
} // namespace Flu