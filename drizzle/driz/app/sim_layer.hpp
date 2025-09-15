#pragma once

#include "onyx/app/user_layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "driz/simulation/solver.hpp"
#include "driz/app/inspector.hpp"

namespace Driz
{
template <Dimension D> class SimLayer final : public Onyx::UserLayer
{
  public:
    SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
             const SimulationState<D> &p_State);

  private:
    void OnUpdate() override;
    void OnEvent(const Onyx::Event &p_Event) override;

    void step(bool p_Dummy = false);
    void renderVisualizationSettings();

    Onyx::Application *m_Application;
    Onyx::Window *m_Window;

    Solver<D> m_Solver;
#ifdef DRIZ_ENABLE_INSPECTOR
    Inspector<D> m_Inspector{&m_Solver};
#endif
    Onyx::RenderContext<D> *m_Context;
    Onyx::Camera<D> *m_Camera;

    f32 m_Timestep = 1.f / 60.f;
    bool m_DummyStep = false;
    bool m_Pause = false;
};
} // namespace Driz
