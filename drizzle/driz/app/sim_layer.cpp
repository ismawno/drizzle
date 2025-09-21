#include "driz/app/sim_layer.hpp"
#include "driz/app/visualization.hpp"
#include "driz/app/intro_layer.hpp"
#include "tkit/profiling/macros.hpp"
#include "tkit/serialization/yaml/glm.hpp"
#include <imgui.h>

namespace Driz
{
static f32 s_RayDistance = 0.f;

template <Dimension D>
SimLayer<D>::SimLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                      const SimulationState<D> &p_State)
    : m_Application(p_Application), m_Solver(p_Settings, p_State)
{
    m_Window = m_Application->GetMainWindow();
    m_Camera = m_Window->CreateCamera<D>();
    m_Camera->BackgroundColor = Onyx::Color{0.15f};
    if constexpr (D == D3)
        m_Camera->SetPerspectiveProjection();

    m_Context = m_Window->CreateRenderContext<D>();
}

static f32 rayCast(const Onyx::Camera<D3> *p_Camera, const Onyx::RenderContext<D3> *p_Context,
                   const SimulationState<D3> &p_State, const f32 p_Radius)
{
    const fvec3 origin = p_Camera->GetWorldMousePosition(&p_Context->GetCurrentAxes(), 0.f);
    const fvec3 direction = p_Camera->GetMouseRayCastDirection();

    f32 rayDistance = FLT_MAX;
    f32 maxParticleDistance = 0.f;
    for (const fvec3 &p : p_State.Positions)
    {
        const fvec3 op = p - origin;
        const f32 b = glm::dot(op, direction);
        const f32 particleDistance = glm::length2(op);
        if (particleDistance > maxParticleDistance)
            maxParticleDistance = particleDistance;

        const f32 c = particleDistance - p_Radius * p_Radius;
        const f32 disc = b * b - c;
        if (disc < 0.f)
            continue;
        const f32 dist = b - glm::sqrt(disc);
        if (dist < rayDistance)
            rayDistance = dist;
    }

    return rayDistance != FLT_MAX ? rayDistance : glm::sqrt(maxParticleDistance);
}

template <Dimension D> void SimLayer<D>::OnUpdate()
{
    TKIT_PROFILE_NSCOPE("SimLayer::Onupdate");
    if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::R) && !ImGui::GetIO().WantCaptureKeyboard)
        m_Solver.AddParticle(m_Camera->GetWorldMousePosition(&m_Context->GetCurrentAxes()));
    if (!m_Pause)
        step(m_DummyStep);

    Visualization<D>::AdjustRenderContext(m_Context);
    m_Camera->ControlMovementWithUserInput(0.75f * m_Application->GetDeltaTime());
    m_Solver.DrawParticles(m_Context);
    m_Solver.DrawBoundingBox(m_Context);

    if constexpr (D == D2)
        if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft) &&
            !ImGui::GetIO().WantCaptureMouse)
            Visualization<D2>::DrawMouseInfluence(m_Camera, m_Context, 2.f * m_Solver.Settings.MouseRadius,
                                                  Onyx::Color::ORANGE);

    if (ImGui::Begin("Simulation settings"))
    {
        ExportWidget("Export simulation state", Core::GetStatePath<D>(), m_Solver.Data.State);
        ImportWidget("Import simulation state", Core::GetStatePath<D>(), m_Solver.Data.State);

        if (ImGui::Button("Back to menu"))
        {
            m_Window->DestroyCamera(m_Camera);
            m_Window->DestroyRenderContext(m_Context);
            m_Application->SetUserLayer<IntroLayer>(m_Application, m_Solver.Settings, m_Solver.Data.State);
        }
        Visualization<D>::RenderSettings(m_Solver.Settings);
    }
    ImGui::End();

    if (ImGui::Begin("Visualization settings"))
        renderVisualizationSettings();
    ImGui::End();
}

template <Dimension D> void SimLayer<D>::OnEvent(const Onyx::Event &p_Event)
{
    if constexpr (D == D2)
        if (p_Event.Type == Onyx::Event::Scrolled && !ImGui::GetIO().WantCaptureMouse)
        {
            f32 step = 0.005f * p_Event.ScrollOffset.y;
            if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
                step *= 10.f;

            m_Camera->ControlScrollWithUserInput(step);
            return;
        }

    if (p_Event.Type == Onyx::Event::KeyPressed && !ImGui::GetIO().WantCaptureKeyboard)
        switch (p_Event.Key)
        {
        case Onyx::Input::Key::Escape:
            m_Application->Quit();
            break;
        case Onyx::Input::Key::P:
            m_Pause = !m_Pause;
            break;
        case Onyx::Input::Key::O:
            m_DummyStep = !m_DummyStep;
            break;
        default:
            break;
        }
}

template <Dimension D> void SimLayer<D>::step(const bool p_Dummy)
{
    m_Solver.BeginStep(m_Timestep);
    m_Solver.UpdateLookup();
    m_Solver.ComputeDensities();
    m_Solver.AddPressureAndViscosity();
    if constexpr (D == D2)
    {
        if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft) &&
            !ImGui::GetIO().WantCaptureMouse)
        {
            const fvec2 mpos = m_Camera->GetWorldMousePosition(&m_Context->GetCurrentAxes());
            m_Solver.AddMouseForce(mpos);
        }
    }
    else
    {
        const fvec3 origin = m_Camera->GetWorldMousePosition(&m_Context->GetCurrentAxes(), 0.f);
        const fvec3 direction = m_Camera->GetMouseRayCastDirection();
        if (Onyx::Input::IsMouseButtonPressed(m_Window, Onyx::Input::Mouse::ButtonLeft))
            m_Solver.AddMouseForce(origin + s_RayDistance * direction);
        else
        {
            s_RayDistance = rayCast(m_Camera, m_Context, m_Solver.Data.State, m_Solver.Settings.ParticleRadius);
            const fvec3 pos = origin + s_RayDistance * direction;
            for (u32 i = 0; i < m_Solver.Data.State.Positions.GetSize(); ++i)
            {
                const f32 distance2 = glm::distance2(m_Solver.Data.State.Positions[i], pos);
                if (distance2 < m_Solver.Settings.MouseRadius * m_Solver.Settings.MouseRadius)
                    m_Solver.Data.UnderMouseInfluence[i] = 2;
            }
        }
    }

    if (!p_Dummy)
        m_Solver.ApplyComputedForces(m_Timestep);
    m_Solver.EndStep();
}

template <Dimension D> void SimLayer<D>::renderVisualizationSettings()
{
    PresentModeEditor(m_Window, Flag_DisplayHelp);
    ImGui::Spacing();
    DisplayFrameTime(m_Application->GetDeltaTime(), Flag_DisplayHelp);
    ImGui::Spacing();

    if constexpr (D == D3)
        ResolutionEditor("Shape resolution", Core::Resolution, Flag_DisplayHelp);

    const u32 pcount = m_Solver.GetParticleCount();
    ImGui::Text("Particles: %u", pcount);

    static bool syncTimestep = false;
    ImGui::Checkbox("Sync timestep", &syncTimestep);
    HelpMarkerSameLine("If enabled, the timestep will be synchronized with the application's delta time. This is "
                       "actually discouraged, as it can lead to unstable simulations.");

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
    HelpMarkerSameLine(
        "This is the time step/frequency of the simulation, which determines how big time jumps are between steps. A "
        "larger time step will make the simulation run faster (as in, time will pass faster), but it can lead to "
        "unstabilities. Smaller time steps however will make the simulation run slower, but it will be more stable. "
        "Usually, 60 hertz is a good enough value.");

    static bool drawGrid = false;
    ImGui::Checkbox("Draw grid", &drawGrid);
    HelpMarkerSameLine(
        "If the grid spatial lookup optimization is enabled, this setting will let you visualize the grid cells as "
        "well as if there are clashes between them.");

    if (drawGrid)
    {
        m_Solver.UpdateLookup();
        const u32 cellClashes = m_Solver.Lookup.DrawCells(m_Context);
        ImGui::Text("Cell clashes: %u", cellClashes);
        HelpMarkerSameLine(
            "The grid spatial lookup optimization divides the simulation space into cells, which are "
            "used to quickly find neighboring particles. To efficiently access and relate particles "
            "with their corresponding cells, the latter are hashed to the number of particles. Because of this, cell "
            "hashes can clash, which will render the grid lookup slightly less efficient. This metric displays the "
            "number of clashes found.");
    }

    ImGui::Checkbox("Pause simulation", &m_Pause);
    ImGui::Checkbox("Dummy step", &m_DummyStep);
    HelpMarkerSameLine(
        "A dummy step is very similar to pausing the simulation. The only difference is that the whole step is "
        "actually computed, but the forces are not applied to the particles. This is useful for debugging purposes.");

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

} // namespace Driz
