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

    updateStateAsLattice(m_State2);
    updateStateAsLattice(m_State3);
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
        updateStateAsLattice(m_State3);
    }
    else
    {
        m_Dim = 1;
        m_State3 = p_State;
        updateStateAsLattice(m_State2);
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
    Visualization<D>::AdjustAndControlCamera(p_Context, m_Application->GetDeltaTime());
    Visualization<D>::DrawParticles(p_Context, m_Settings, p_State);
    Visualization<D>::DrawBoundingBox(p_Context, p_State.Min, p_State.Max, Onyx::Color::FromHexadecimal("A6B1E1"));
}

bool IntroLayer::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if (m_Dim == 0 && p_Event.Type == Onyx::Event::Scrolled && !ImGui::GetIO().WantCaptureMouse)
    {
        f32 step = 0.005f * p_Event.ScrollOffset.y;
        if (Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
            step *= 10.f;
        m_Context2->ApplyCameraScalingControls(step);
        return true;
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

    return false;
}

void IntroLayer::renderIntroSettings() noexcept
{
    ImGui::Begin("Welcome to Drizzle, my fluid simulator!");
    const f32 deltaTime = m_Application->GetDeltaTime().AsMilliseconds();
    PresentModeEditor(m_Window);
    ImGui::Text("Frame time: %.2f ms", deltaTime);

    ImGui::Spacing();
    ImGui::TextWrapped(
        "Drizzle is a small project I have made inspired by Sebastian Lague's fluid simulation video. It "
        "features a 2D and 3D fluid simulation using the Smoothed Particle Hydrodynamics method. The "
        "simulation itself is simple, performance oriented and can be simulated both in 2D and 3D.");
    ImGui::TextLinkOpenURL("Sebastian Lague's video", "https://www.youtube.com/watch?v=rSKMYc1CQHE");
    ImGui::TextLinkOpenURL("My GitHub", "https://github.com/ismawno");

    ImGui::Spacing();

    ImGui::Combo("Dimension", &m_Dim,
                 "2D\0"
                 "3D\0\0");

    ImGui::Spacing();

    ImGui::Text("The camera controls are the following:");
    ImGui::BulletText("W-A-S-D: Move the camera");
    ImGui::BulletText("Q-E: Rotate the camera");
    ImGui::BulletText("Mouse wheel: Zoom in/out (Left Shift for faster zoom, 2D only)");
    ImGui::BulletText("Left mouse button: Apply a force to the fluid");

    ImGui::Spacing();

    ImGui::TextWrapped(
        "You can choose how many starting particles you want to have by tweaking the settings below. Note "
        "that you may add more particles during the simulation by pressing the space bar. The layout of the starting "
        "particles is conditioned by the selected dimension, either 2D or 3D.");
    ImGui::TextWrapped("Note that you may also choose the option to import a custom or past simulation state.");

    ImGui::Spacing();

    if (m_Dim == 0)
    {
        ImGui::Text("Current amount: %u", m_State2.Positions.size());
        if (ImGui::DragInt2("Particles", glm::value_ptr(m_Dimensions), 1, 1, INT32_MAX))
            updateStateAsLattice(m_State2);
        ExportWidget("Export particle state", Core::GetStatePath<D2>(), m_State2);
        ImportWidget("Import particle state", Core::GetStatePath<D2>(), m_State2);
    }
    else
    {
        ImGui::Text("Current amount: %u", m_State3.Positions.size());
        if (ImGui::DragInt3("Particles", glm::value_ptr(m_Dimensions), 1, 1, INT32_MAX))
            updateStateAsLattice(m_State3);
        ExportWidget("Export particle state", Core::GetStatePath<D3>(), m_State3);
        ImportWidget("Import particle state", Core::GetStatePath<D3>(), m_State3);
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
    ImGui::End();
}

template <Dimension D> void IntroLayer::updateStateAsLattice(SimulationState<D> &p_State) noexcept
{
    p_State.Positions.clear();
    p_State.Velocities.clear();
    const f32 separation = 0.4f * m_Settings.SmoothingRadius;
    const fvec<D> midPoint = 0.5f * separation * fvec<D>{m_Dimensions};
    for (i32 i = 0; i < m_Dimensions.x; ++i)
        for (i32 j = 0; j < m_Dimensions.y; ++j)
            if constexpr (D == D2)
            {
                const f32 x = static_cast<f32>(i) * separation;
                const f32 y = static_cast<f32>(j) * separation;
                p_State.Positions.push_back(fvec2{x, y} - midPoint);
                p_State.Velocities.push_back(fvec2{0.f});
            }
            else
                for (i32 k = 0; k < m_Dimensions.z; ++k)
                {
                    const f32 x = static_cast<f32>(i) * separation;
                    const f32 y = static_cast<f32>(j) * separation;
                    const f32 z = static_cast<f32>(k) * separation;
                    p_State.Positions.push_back(fvec3{x, y, z} - midPoint);
                    p_State.Velocities.push_back(fvec3{0.f});
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