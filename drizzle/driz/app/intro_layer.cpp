#include "driz/app/intro_layer.hpp"
#include "driz/app/sim_layer.hpp"
#include "driz/app/visualization.hpp"
#include "tkit/serialization/yaml/glm.hpp"
#include <imgui.h>

namespace Driz
{
IntroLayer::IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                       const Dimension p_Dim) noexcept
    : m_Application(p_Application), m_Dim(p_Dim == D2 ? 0 : 1), m_Settings(p_Settings)
{
    m_Window = m_Application->GetMainWindow();
    m_Context2 = m_Window->GetRenderContext<D2>();
    m_Context3 = m_Window->GetRenderContext<D3>();
    m_Context3->SetPerspectiveProjection();

    updateStateAsLattice<D2>(m_State2, m_Dimensions2);
    updateStateAsLattice<D3>(m_State3, m_Dimensions3);
}

template <Dimension D>
IntroLayer::IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                       const SimulationState<D> &p_State) noexcept
    : m_Application(p_Application), m_Settings(p_Settings)
{
    if constexpr (D == D2)
    {
        m_Dim = 0;
        m_State2 = p_State;
        updateStateAsLattice<D3>(m_State3, m_Dimensions3);
    }
    else
    {
        m_Dim = 1;
        m_State3 = p_State;
        updateStateAsLattice<D2>(m_State2, m_Dimensions2);
    }
    m_Window = m_Application->GetMainWindow();

    m_Context2 = m_Window->GetRenderContext<D2>();
    m_Context3 = m_Window->GetRenderContext<D3>();
    m_Context3->SetPerspectiveProjection();
}

void IntroLayer::OnRender(const VkCommandBuffer) noexcept
{
    m_Context2->Flush();
    m_Context3->Flush();
    if (m_Dim == 0)
        onRender(m_Context2, m_State2);
    else
        onRender(m_Context3, m_State3);
    renderIntroSettings();
}

template <Dimension D>
void IntroLayer::onRender(Onyx::RenderContext<D> *p_Context, const SimulationState<D> &p_State) noexcept
{
    Visualization<D>::AdjustRenderingContext(p_Context, m_Application->GetDeltaTime());
    Visualization<D>::DrawParticles(p_Context, m_Settings, p_State);
    Visualization<D>::DrawBoundingBox(p_Context, p_State.Min, p_State.Max, Onyx::Color::FromHexadecimal("A6B1E1"));
}

void IntroLayer::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if (m_Dim == 0 && p_Event.Type == Onyx::Event::Scrolled && !ImGui::GetIO().WantCaptureMouse)
    {
        f32 step = 0.005f * p_Event.ScrollOffset.y;
        if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
            step *= 10.f;
        m_Context2->ApplyCameraScalingControls(step);
        return;
    }

    if (p_Event.Type == Onyx::Event::KeyPressed && !ImGui::GetIO().WantCaptureKeyboard)
        switch (p_Event.Key)
        {
        case Onyx::Input::Key::Escape:
            m_Application->Quit();
            break;
        default:
            break;
        }
}

void IntroLayer::renderIntroSettings() noexcept
{
    if (ImGui::Begin("Welcome to Drizzle, my fluid simulator!"))
    {
        PresentModeEditor(m_Window, Flag_DisplayHelp);
        ImGui::Spacing();
        DisplayFrameTime(m_Application->GetDeltaTime(), Flag_DisplayHelp);
        ImGui::Spacing();

        ImGui::Text("Version: " DRIZ_VERSION);
        ImGui::TextWrapped(
            "Drizzle is a small project I have made inspired by Sebastian Lague's fluid simulation video. It "
            "features a 2D and 3D fluid simulation using the Smoothed Particle Hydrodynamics method. The "
            "simulation itself is simple and performance oriented.");

        ImGui::Text("Missing features I would like to implement shortly:");
        ImGui::BulletText("Additional fluid behaviours: Viscoealsticity, plasticity, stickiness, etc.");
        ImGui::BulletText("SIMD optimizations.");
        ImGui::BulletText("Compute shaders support.");

        ImGui::TextLinkOpenURL("Sebastian Lague's video", "https://www.youtube.com/watch?v=rSKMYc1CQHE");
        ImGui::TextLinkOpenURL("My GitHub", "https://github.com/ismawno");

        ImGui::Spacing();
        ImGui::Combo("Dimension", &m_Dim,
                     "2D\0"
                     "3D\0\0");
        HelpMarkerSameLine("You can choose between a 2D and 3D simulation. 3D is more computationally expensive.");
        ImGui::Spacing();

        ImGui::Text("The camera controls are the following:");
        if (m_Dim == 0)
            DisplayCameraMovementControls<D2>();
        else
            DisplayCameraMovementControls<D3>();
        ImGui::BulletText("R: Spawn particles");
        ImGui::BulletText("Mouse click: Interact with the fluid!");

        ImGui::Spacing();
        ImGui::TextWrapped("You can choose how many starting particles you want to have by tweaking the settings "
                           "below. The layout of the starting particles is conditioned by the selected dimension.");
        ImGui::TextWrapped("Note that you may also choose the option to import a custom or past simulation state.");
        ImGui::Spacing();

        if (m_Dim == 0)
        {
            ImGui::Text("Current amount: %u", m_State2.Positions.size());
            if (ImGui::DragInt2("Particles", reinterpret_cast<i32 *>(glm::value_ptr(m_Dimensions2)), 1.f, 1, INT32_MAX))
                updateStateAsLattice<D2>(m_State2, m_Dimensions2);
            ExportWidget("Export simulation state", Core::GetStatePath<D2>(), m_State2);
            ImportWidget("Import simulation state", Core::GetStatePath<D2>(), m_State2);
        }
        else
        {
            ResolutionEditor("Shape resolution", Core::Resolution, Flag_DisplayHelp);
            ImGui::Text("Current amount: %u", m_State3.Positions.size());
            if (ImGui::DragInt3("Particles", reinterpret_cast<i32 *>(glm::value_ptr(m_Dimensions3)), 1.f, 1, INT32_MAX))
                updateStateAsLattice<D3>(m_State3, m_Dimensions3);
            ExportWidget("Export simulation state", Core::GetStatePath<D3>(), m_State3);
            ImportWidget("Import simulation state", Core::GetStatePath<D3>(), m_State3);
        }

        ImGui::Spacing();

        if (ImGui::Button("Start simulation"))
        {
            if (m_Dim == 0)
                m_Application->SetUserLayer<SimLayer<D2>>(m_Application, m_Settings, m_State2);
            else
                m_Application->SetUserLayer<SimLayer<D3>>(m_Application, m_Settings, m_State3);
        }

        if (m_Dim == 0)
            renderBoundingBox(m_State2);
        else
            renderBoundingBox(m_State3);
    }
    ImGui::End();
}

template <Dimension D>
void IntroLayer::updateStateAsLattice(SimulationState<D> &p_State, const uvec<D> &p_Dimensions) noexcept
{
    p_State.Positions.clear();
    p_State.Velocities.clear();
    const f32 separation = 0.4f * m_Settings.SmoothingRadius;
    const fvec<D> midPoint = 0.5f * separation * fvec<D>{p_Dimensions - 1u};
    for (u32 i = 0; i < p_Dimensions.x; ++i)
    {
        const f32 x = static_cast<f32>(i) * separation;
        for (u32 j = 0; j < p_Dimensions.y; ++j)
        {
            const f32 y = static_cast<f32>(j) * separation;
            if constexpr (D == D2)
            {
                p_State.Positions.push_back(fvec2{x, y} - midPoint);
                p_State.Velocities.push_back(fvec2{0.f});
            }
            else
                for (u32 k = 0; k < p_Dimensions.z; ++k)
                {
                    const f32 z = static_cast<f32>(k) * separation;
                    p_State.Positions.push_back(fvec3{x, y, z} - midPoint);
                    p_State.Velocities.push_back(fvec3{0.f});
                }
        }
    }
}

template <Dimension D> void IntroLayer::renderBoundingBox(SimulationState<D> &p_State) noexcept
{
    if (ImGui::TreeNode("Bounding box"))
    {
        if (ImGui::DragFloat("Width", &p_State.Max.x, 0.05f))
            p_State.Min.x = -p_State.Max.x;
        if (ImGui::DragFloat("Height", &p_State.Max.y, 0.05f))
            p_State.Min.y = -p_State.Max.y;
        if constexpr (D == D3)
        {
            if (ImGui::DragFloat("Depth", &p_State.Max.z, 0.05f))
                p_State.Min.z = -p_State.Max.z;
        }
        ImGui::TreePop();
    }
    Visualization<D>::RenderSettings(m_Settings);
}

template IntroLayer::IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                                const SimulationState<D2> &p_State) noexcept;
template IntroLayer::IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                                const SimulationState<D3> &p_State) noexcept;

} // namespace Driz