#include "flu/app/layer.hpp"
#include <imgui.h>

namespace FLU
{
template <Dimension D>
Layer<D>::Layer(Onyx::Application *p_Application) noexcept : Onyx::Layer("FLU Layer"), m_Application(p_Application)
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
    m_Context->Flush(Onyx::Color::BLACK);
    m_Context->ScaleAxes(0.05f);

    m_Context->ApplyCameraMovementControls(1.5f * m_Application->GetDeltaTime());

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

template <Dimension D> bool Layer<D>::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if constexpr (D == D2)
    {
        if (p_Event.Type == Onyx::Event::Scrolled && Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
        {
            m_Context->ApplyCameraScalingControls(0.005f * p_Event.ScrollOffset.y);
            return true;
        }
    }
    if (p_Event.Type == Onyx::Event::MousePressed && Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::Space))
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