#pragma once

#include "onyx/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/solver/solver.hpp"

namespace Flu
{
template <Dimension D> class Layer final : public Onyx::Layer
{
  public:
    Layer(Onyx::Application *p_Application) noexcept;

    void OnStart() noexcept override;
    void OnUpdate() noexcept override;
    void OnRender(VkCommandBuffer) noexcept override;
    bool OnEvent(const Onyx::Event &p_Event) noexcept override;

  private:
    void addParticle() noexcept;

    Onyx::Application *m_Application;
    Onyx::Window *m_Window;

    Solver<D> m_Solver;
    Onyx::RenderContext<D> *m_Context;
};
} // namespace Flu