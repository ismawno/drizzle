#include "flu/app/layer.hpp"
#include <imgui.h>

namespace FLU
{
template <Dimension D>
Layer<D>::Layer(ONYX::Application *p_Application) noexcept : ONYX::Layer("FLU Layer"), m_Application(p_Application)
{
}

template <Dimension D> void Layer<D>::OnStart() noexcept
{
    m_Window = m_Application->GetMainWindow();
    m_Context = m_Window->GetRenderContext<D>();
}

template <Dimension D> void Layer<D>::OnUpdate() noexcept
{
    m_Solver.Step(m_Application->GetDeltaTime().AsSeconds());
}

template <Dimension D> void Layer<D>::OnRender(const VkCommandBuffer) noexcept
{
    m_Context->Flush(ONYX::Color::BLACK);
    m_Context->ScaleAxes(0.05f);

    const f32 step = 1.5f * m_Application->GetDeltaTime().AsSeconds();
    m_Context->ApplyCameraLikeMovementControls(step, step);

    for (const Particle<D> &particle : m_Solver.Particles)
        particle.Draw(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    ImGui::Begin("Editor");
    if constexpr (D == D2)
    {
        const vec2 mpos = m_Context->GetMouseCoordinates();
        ImGui::Text("Mouse: (%.2f, %.2f)", mpos.x, mpos.y);
    }

    ImGui::Text("Particles: %zu", m_Solver.Particles.size());
    ImGui::End();
}

template <Dimension D> bool Layer<D>::OnEvent(const ONYX::Event &p_Event) noexcept
{
    if constexpr (D == D2)
    {
        if (p_Event.Type == ONYX::Event::Scrolled && ONYX::Input::IsKeyPressed(m_Window, ONYX::Input::Key::LeftShift))
        {
            m_Context->ApplyCameraLikeScalingControls(0.005f * p_Event.ScrollOffset.y);
            return true;
        }
    }
    if (p_Event.Type == ONYX::Event::MousePressed && ONYX::Input::IsKeyPressed(m_Window, ONYX::Input::Key::Space))
    {
        Particle<D> particle{};
        if constexpr (D == D2)
            particle.Position = m_Context->GetMouseCoordinates();
        m_Solver.Particles.push_back(particle);
        return true;
    }

    return false;
}

template class Layer<D2>;
template class Layer<D3>;

} // namespace FLU