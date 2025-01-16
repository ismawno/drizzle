#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D>
SimLayer<D>::SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                      const uvec<D> &p_StartingLayout) noexcept
    : Onyx::Layer("Flu Layer"), m_Application(p_Application)
{
    m_Window = m_Application->GetMainWindow();
    m_Context = m_Window->GetRenderContext<D>();
    m_Solver.Settings = p_Settings;

    const f32 size = 4.f * m_Solver.Settings.ParticleRadius;
    const fvec<D> midPoint = 0.5f * size * fvec<D>{p_StartingLayout};
    for (u32 i = 0; i < p_StartingLayout.x; ++i)
        for (u32 j = 0; j < p_StartingLayout.y; ++j)
            if constexpr (D == D2)
            {
                const f32 x = static_cast<f32>(i) * size;
                const f32 y = static_cast<f32>(j) * size;
                m_Solver.AddParticle(fvec2{x, y} - midPoint);
            }
            else
                for (u32 k = 0; k < p_StartingLayout.z; ++k)
                {
                    const f32 x = static_cast<f32>(i) * size;
                    const f32 y = static_cast<f32>(j) * size;
                    const f32 z = static_cast<f32>(k) * size;
                    m_Solver.AddParticle(fvec3{x, y, z} - midPoint);
                }
}

template <Dimension D> void SimLayer<D>::OnUpdate() noexcept
{
    if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::Space))
        m_Solver.AddParticle(m_Context->GetMouseCoordinates());
    if (!m_Pause)
        step(m_DummyStep);
}

template <Dimension D> void SimLayer<D>::OnRender(const VkCommandBuffer) noexcept
{
    Visualization<D>::AdjustAndControlCamera(m_Context, m_Application->GetDeltaTime());
    m_Solver.DrawParticles(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
        Visualization<D>::DrawMouseInfluence(m_Context, 2.f * m_Solver.Settings.MouseRadius, Onyx::Color::ORANGE);

    if (ImGui::Begin("Simulation settings"))
        Visualization<D>::RenderSettings(m_Solver.Settings);
    ImGui::End();

    if (ImGui::Begin("Visualization settings"))
        renderVisualizationSettings();
    ImGui::End();
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

template <Dimension D> void SimLayer<D>::step(const bool p_Dummy) noexcept
{
    m_Solver.BeginStep(m_Timestep);
    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
    {
        const fvec<D> p_MousePos = m_Context->GetMouseCoordinates();
        m_Solver.ApplyMouseForce(p_MousePos);
    }
    m_Solver.EndStep(m_Timestep, !p_Dummy);
}

template <Dimension D> void SimLayer<D>::renderVisualizationSettings() noexcept
{
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

    static bool drawGrid = false;
    ImGui::Checkbox("Draw grid", &drawGrid);
    if (m_Solver.Settings.SearchMethod == NeighborSearch::Grid && drawGrid)
    {
        const u32 cellClashes = m_Solver.GetLookup().DrawCells(m_Context);
        ImGui::Text("Cell clashes: %u", cellClashes);
    }

    ImGui::Checkbox("Pause simulation", &m_Pause);
    ImGui::Checkbox("Dummy step", &m_DummyStep);
    if ((m_Pause || m_DummyStep) && ImGui::Button("Step"))
        step();

    const u32 pcount = m_Solver.GetParticleCount();
    if (ImGui::TreeNode(this, "Particles: %u", pcount))
    {
        ImGui::BeginChild("Particles", {0, 150}, true);
        for (u32 i = 0; i < pcount; ++i)
        {
            const auto data = m_Solver.GetParticleData(i);
            ImGui::Text("Particle %u", i);
            ImGui::Indent(15.f);
            if constexpr (D == D2)
            {
                ImGui::Text("Position: (%.2f, %.2f)", data.Position.x, data.Position.y);
                ImGui::Text("Velocity: (%.2f, %.2f)", data.Velocity.x, data.Velocity.y);
                ImGui::Text("Acceleration: (%.2f, %.2f)", data.Acceleration.x, data.Acceleration.y);
                ImGui::Text("Cell position: (%d, %d)", data.CellPosition.x, data.CellPosition.y);
            }
            else
            {
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", data.Position.x, data.Position.y, data.Position.z);
                ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", data.Velocity.x, data.Velocity.y, data.Velocity.z);
                ImGui::Text("Acceleration: (%.2f, %.2f, %.2f)", data.Acceleration.x, data.Acceleration.y,
                            data.Acceleration.z);
                ImGui::Text("Cell position: (%d, %d, %d)", data.CellPosition.x, data.CellPosition.y,
                            data.CellPosition.z);
            }
            ImGui::Text("Cell key: %u", data.CellKey);
            ImGui::Unindent(15.f);
        }
        ImGui::EndChild();
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Bounding box"))
    {
        if (ImGui::DragFloat("Width", &m_Solver.BoundingBox.Max.x, 0.05f))
            m_Solver.BoundingBox.Min.x = -m_Solver.BoundingBox.Max.x;
        if (ImGui::DragFloat("Height", &m_Solver.BoundingBox.Max.y, 0.05f))
            m_Solver.BoundingBox.Min.y = -m_Solver.BoundingBox.Max.y;
        ImGui::TreePop();
    }
}

template class SimLayer<D2>;
template class SimLayer<D3>;

} // namespace Flu