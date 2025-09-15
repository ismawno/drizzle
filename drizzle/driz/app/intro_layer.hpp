#pragma once

#include "onyx/app/user_layer.hpp"
#include "onyx/app/app.hpp"
#include "onyx/rendering/render_context.hpp"
#include "driz/simulation/settings.hpp"

namespace Driz
{
class IntroLayer final : public Onyx::UserLayer
{
  public:
    IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings, Dimension p_Dim);

    template <Dimension D>
    IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
               const SimulationState<D> &p_State);

  private:
    void OnUpdate() override;
    void OnEvent(const Onyx::Event &p_Event) override;

    template <Dimension D>
    void onUpdate(Onyx::Camera<D> *p_Camera, Onyx::RenderContext<D> *p_Context,
                  const SimulationState<D> &p_State);
    template <Dimension D> void updateStateAsLattice(SimulationState<D> &p_State, const uvec<D> &p_Dimensions);
    template <Dimension D> void renderBoundingBox(SimulationState<D> &p_State);

    void renderIntroSettings();

    Onyx::Application *m_Application;
    i32 m_Dim = 0;
#ifndef TKIT_DEBUG
    uvec2 m_Dimensions2{64, 64};
    uvec3 m_Dimensions3{16, 16, 16};
#else
    uvec2 m_Dimensions2{16, 16};
    uvec3 m_Dimensions3{8, 8, 8};
#endif
    Onyx::Window *m_Window;

    Onyx::RenderContext<D2> *m_Context2;
    Onyx::RenderContext<D3> *m_Context3;

    Onyx::Camera<D2> *m_Camera2;
    Onyx::Camera<D3> *m_Camera3;

    SimulationSettings m_Settings;

    SimulationState<D2> m_State2;
    SimulationState<D3> m_State3;
};
} // namespace Driz
