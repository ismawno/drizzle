#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include "flu/app/intro_layer.hpp"
#include "tkit/profiling/macros.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D>
SimLayer<D>::SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                      const SimulationState<D> &p_State) noexcept
    : m_Application(p_Application), m_Solver(p_Settings, p_State)
{
    m_Window = m_Application->GetMainWindow();
    m_Context = m_Window->GetRenderContext<D>();
}

template <Dimension D> void SimLayer<D>::OnUpdate() noexcept
{
    TKIT_PROFILE_NSCOPE("SimLayer::Onupdate");
    if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::Space) && !ImGui::GetIO().WantCaptureKeyboard)
        m_Solver.AddParticle(m_Context->GetMouseCoordinates());
    if (!m_Pause)
        step(m_DummyStep);
}

template <Dimension D> void SimLayer<D>::OnRender(const VkCommandBuffer) noexcept
{
    TKIT_PROFILE_NSCOPE("SimLayer::OnRender");
    Visualization<D>::AdjustAndControlCamera(m_Context, m_Application->GetDeltaTime());
    m_Solver.DrawParticles(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft) && !ImGui::GetIO().WantCaptureMouse)
        Visualization<D>::DrawMouseInfluence(m_Context, 2.f * m_Solver.Settings.MouseRadius, Onyx::Color::ORANGE);

    if (ImGui::Begin("Simulation settings"))
    {
        if (ImGui::Button("Back to menu"))
            m_Application->SetUserLayer<IntroLayer>(m_Application);
        Visualization<D>::RenderSettings(m_Solver.Settings);
    }
    ImGui::End();

    if (ImGui::Begin("Visualization settings"))
        renderVisualizationSettings();
    ImGui::End();

#ifdef FLU_ENABLE_INSPECTOR
    if (ImGui::Begin("Simulation inspector"))
        m_Inspector.Render();
    ImGui::End();
#endif
}

template <Dimension D> bool SimLayer<D>::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if constexpr (D == D2)
        if (p_Event.Type == Onyx::Event::Scrolled && !ImGui::GetIO().WantCaptureMouse)
        {
            f32 step = 0.005f * p_Event.ScrollOffset.y;
            if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
                step *= 10.f;
            m_Context->ApplyCameraScalingControls(step);
            return true;
        }

    if (p_Event.Type == Onyx::Event::KeyPressed && !ImGui::GetIO().WantCaptureKeyboard)
        switch (p_Event.Key)
        {
        case Onyx::Input::Key::P:
            m_Pause = !m_Pause;
            break;
        case Onyx::Input::Key::O:
            m_DummyStep = !m_DummyStep;
            break;
        default:
            break;
        }

    return false;
}

template <Dimension D> void SimLayer<D>::step(const bool p_Dummy) noexcept
{
    m_Solver.BeginStep(m_Timestep);
    m_Solver.UpdateLookup();
    m_Solver.ComputeDensities();
    m_Solver.AddPressureAndViscosity();
    if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
    {
        const fvec<D> p_MousePos = m_Context->GetMouseCoordinates();
        m_Solver.AddMouseForce(p_MousePos);
    }

#ifdef FLU_ENABLE_INSPECTOR
    if (m_Inspector.WantsToInspect())
    {
        m_Solver.UpdateAllLookups();
        m_Inspector.Inspect();
    }
#endif

    if (!p_Dummy)
        m_Solver.ApplyComputedForces(m_Timestep);
    m_Solver.EndStep();
}

template <Dimension D> void SimLayer<D>::renderVisualizationSettings() noexcept
{
    PresentModeEditor(m_Window);
    ImGui::Text("Frame time: %.2f ms", m_Application->GetDeltaTime().AsMilliseconds());
    const u32 fps = static_cast<u32>(1.f / m_Application->GetDeltaTime().AsSeconds());
    ImGui::Text("FPS: %u", fps);

    const u32 pcount = m_Solver.GetParticleCount();
    ImGui::Text("Particles: %u", pcount);

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
    if (m_Solver.Settings.UsesGrid() && drawGrid)
    {
        m_Solver.UpdateLookup();
        const u32 cellClashes = m_Solver.Lookup.DrawCells(m_Context);
        ImGui::Text("Cell clashes: %u", cellClashes);
    }

    ImGui::Checkbox("Pause simulation", &m_Pause);
    ImGui::Checkbox("Dummy step", &m_DummyStep);
    if ((m_Pause || m_DummyStep) && ImGui::Button("Step"))
        step();

    if (ImGui::TreeNode("Bounding box"))
    {
        if (ImGui::DragFloat("Width", &m_Solver.Data.State.Max.x, 0.05f))
            m_Solver.Data.State.Min.x = -m_Solver.Data.State.Max.x;
        if (ImGui::DragFloat("Height", &m_Solver.Data.State.Max.y, 0.05f))
            m_Solver.Data.State.Min.y = -m_Solver.Data.State.Max.y;
        if constexpr (D == D3)
        {
            if (ImGui::DragFloat("Depth", &m_Solver.Data.State.Max.z, 0.05f))
                m_Solver.Data.State.Min.z = -m_Solver.Data.State.Max.z;
        }

        ImGui::TreePop();
    }
}

template class SimLayer<D2>;
template class SimLayer<D3>;

} // namespace Flu