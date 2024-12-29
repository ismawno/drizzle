#include "flu/app/layer.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D>
Layer<D>::Layer(Onyx::Application *p_Application) noexcept : Onyx::Layer("Flu Layer"), m_Application(p_Application)
{
}

template <Dimension D> void Layer<D>::OnStart() noexcept
{
    m_Window = m_Application->GetMainWindow();
    m_Context = m_Window->GetRenderContext<D>();
}

template <Dimension D> void Layer<D>::OnUpdate() noexcept
{
    if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::Space))
        addParticle();
    m_Solver.Step(m_Timestep);
}

template <Dimension D> void Layer<D>::OnRender(const VkCommandBuffer) noexcept
{
    m_Context->Flush(Onyx::Color::BLACK);
    m_Context->ScaleAxes(0.05f);

    m_Context->ApplyCameraMovementControls(1.5f * m_Application->GetDeltaTime());

    m_Solver.DrawParticles(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    ImGui::Begin("Editor");
    EditPresentMode(*m_Window);
    ImGui::Text("Frame time: %.2f ms", m_Application->GetDeltaTime().AsMilliseconds());
    if constexpr (D == D2)
    {
        const fvec2 mpos = m_Context->GetMouseCoordinates();
        ImGui::Text("Mouse: (%.2f, %.2f)", mpos.x, mpos.y);
    }
    static bool syncTimestep = false;
    ImGui::Checkbox("Sync Timestep", &syncTimestep);
    if (syncTimestep)
    {
        m_Timestep = m_Application->GetDeltaTime().AsSeconds();
        ImGui::Text("Timestep: %.2f", m_Timestep);
    }
    else
        ImGui::SliderFloat("Timestep", &m_Timestep, 0.001f, 0.1f, "%.3f", ImGuiSliderFlags_Logarithmic);

    ImGui::Text("Particles: %zu", m_Solver.GetParticleCount());
    if (ImGui::TreeNode("Bounding box"))
    {
        if (ImGui::DragFloat("Width", &m_Solver.BoundingBox.Max.x, 0.05f))
            m_Solver.BoundingBox.Min.x = -m_Solver.BoundingBox.Max.x;
        if (ImGui::DragFloat("Height", &m_Solver.BoundingBox.Max.y, 0.05f))
            m_Solver.BoundingBox.Min.y = -m_Solver.BoundingBox.Max.y;
        ImGui::TreePop();
    }

    if constexpr (D == D2)
    {
        const fvec2 mpos = m_Context->GetMouseCoordinates();
        const f32 density = m_Solver.ComputeDensityAtPoint(mpos);
        const f32 pressure = m_Solver.GetPressureFromDensity(density);

        ImGui::Text("Density: %.2f", density);
        ImGui::Text("Pressure: %.2f", pressure);
    }

    const f32 speed = 0.2f;
    ImGui::DragFloat("Particle Radius", &m_Solver.ParticleRadius, speed);
    ImGui::DragFloat("Particle Mass", &m_Solver.ParticleMass, speed);
    ImGui::DragFloat("Target Density", &m_Solver.TargetDensity, 0.1f * speed);
    ImGui::DragFloat("Pressure Stiffness", &m_Solver.PressureStiffness, speed);
    ImGui::DragFloat("Smoothing Radius", &m_Solver.SmoothingRadius, speed);
    ImGui::DragFloat("Fast Speed", &m_Solver.FastSpeed, speed);
    ImGui::DragFloat("Gravity", &m_Solver.Gravity, speed);
    ImGui::DragFloat("Encase Friction", &m_Solver.EncaseFriction, speed);

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

    return false;
}

template <Dimension D> void Layer<D>::addParticle() noexcept
{
    if constexpr (D == D2)
        m_Solver.AddParticle(m_Context->GetMouseCoordinates());
    else
        m_Solver.AddParticle(fvec3{0.f});
}

template class Layer<D2>;
template class Layer<D3>;

} // namespace Flu