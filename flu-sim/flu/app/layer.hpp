#pragma once

#include "onyx/app/layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "flu/solver/solver.hpp"

namespace FLU
{
template <Dimension D> class Layer final : public ONYX::Layer
{
  public:
    Layer(ONYX::Application *p_Application) noexcept;

    void OnStart() noexcept override;
    void OnUpdate() noexcept override;
    void OnRender(VkCommandBuffer) noexcept override;
    bool OnEvent(const ONYX::Event &p_Event) noexcept override;

  private:
    ONYX::Application *m_Application;
    ONYX::Window *m_Window;

    Solver<D> m_Solver;
    ONYX::RenderContext<D> *m_Context;
};
} // namespace FLU