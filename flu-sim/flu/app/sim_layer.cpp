#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D>
SimLayer<D>::SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                      const ivec<D> &p_StartingLayout) noexcept
    : Onyx::Layer("Flu Layer"), m_Application(p_Application)
{
    m_Window = m_Application->GetMainWindow();
    m_Context = m_Window->GetRenderContext<D>();
    m_Solver.Settings = p_Settings;

    const f32 size = 2.f * m_Solver.Settings.ParticleRadius;
    for (i32 i = -p_StartingLayout.x / 2; i < p_StartingLayout.x / 2; ++i)
        for (i32 j = -p_StartingLayout.y / 2; j < p_StartingLayout.y / 2; ++j)
            if constexpr (D == D2)
            {
                const f32 x = static_cast<f32>(i) * size * 2.f;
                const f32 y = static_cast<f32>(j) * size * 2.f;
                m_Solver.AddParticle(fvec<D>{x, y});
            }
            else
                for (i32 k = -p_StartingLayout.z / 2; k < p_StartingLayout.z / 2; ++k)
                {
                    const f32 x = static_cast<f32>(i) * size * 2.f;
                    const f32 y = static_cast<f32>(j) * size * 2.f;
                    const f32 z = static_cast<f32>(k) * size * 2.f;
                    m_Solver.AddParticle(fvec<D>{x, y, z});
                }
}

template <Dimension D> void SimLayer<D>::OnUpdate() noexcept
{
    if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::Space))
        addParticle();
    m_Solver.BeginStep(m_Timestep);
    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
    {
        const fvec<D> p_MousePos = m_Context->GetMouseCoordinates();
        m_Solver.ApplyMouseForce(p_MousePos, m_Timestep);
    }
    m_Solver.EndStep(m_Timestep);
}

template <Dimension D> void SimLayer<D>::OnRender(const VkCommandBuffer) noexcept
{
    Visualization<D>::AdjustAndControlCamera(m_Context, m_Application->GetDeltaTime());
    m_Solver.DrawParticles(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
        Visualization<D>::DrawMouseInfluence(m_Context, Onyx::Color::ORANGE, 2.f * m_Solver.Settings.MouseRadius);

    renderSimulationSettings();
}

template <Dimension D> bool SimLayer<D>::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if constexpr (D == D2)
        if (p_Event.Type == Onyx::Event::Scrolled && Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
        {
            m_Context->ApplyCameraScalingControls(0.005f * p_Event.ScrollOffset.y);
            return true;
        }

    return false;
}

template <Dimension D> void SimLayer<D>::renderSimulationSettings() noexcept
{
    ImGui::Begin("Simulation settings");

    EditPresentMode(m_Window);
    ImGui::Text("Frame time: %.2f ms", m_Application->GetDeltaTime().AsMilliseconds());

    static bool syncTimestep = false;
    ImGui::Checkbox("Sync Timestep", &syncTimestep);

    if (syncTimestep)
    {
        m_Timestep = m_Application->GetDeltaTime().AsSeconds();
        const u32 hertz = static_cast<u32>(1.f / m_Timestep);
        ImGui::Text("Hertz: %u (%.4f)", hertz, m_Timestep);
    }
    else
    {
        i32 hertz = static_cast<i32>(1.f / m_Timestep);
        if (ImGui::SliderInt("Hertz", &hertz, 30, 180))
            m_Timestep = 1.f / static_cast<f32>(hertz);
        ImGui::SameLine();
        ImGui::Text("(%.4f)", m_Timestep);
    }

    ImGui::Text("Particles: %zu", m_Solver.Positions.size());
    if (ImGui::TreeNode("Bounding box"))
    {
        if (ImGui::DragFloat("Width", &m_Solver.BoundingBox.Max.x, 0.05f))
            m_Solver.BoundingBox.Min.x = -m_Solver.BoundingBox.Max.x;
        if (ImGui::DragFloat("Height", &m_Solver.BoundingBox.Max.y, 0.05f))
            m_Solver.BoundingBox.Min.y = -m_Solver.BoundingBox.Max.y;
        ImGui::TreePop();
    }

    Visualization<D>::RenderSettings(m_Solver.Settings);

    ImGui::End();
}

template <Dimension D> void SimLayer<D>::addParticle() noexcept
{
    if constexpr (D == D2)
        m_Solver.AddParticle(m_Context->GetMouseCoordinates());
    else
        m_Solver.AddParticle(fvec3{0.f});
}

template class SimLayer<D2>;
template class SimLayer<D3>;

} // namespace Flu