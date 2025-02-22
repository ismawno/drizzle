#include "flu/app/intro_layer.hpp"
#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include <imgui.h>

namespace Flu
{
IntroLayer::IntroLayer(Onyx::Application *p_Application, const SimulationSettings &p_Settings,
                       const SimulationState<D2> &p_State2, const SimulationState<D3> &p_State3) noexcept
    : m_Application(p_Application), m_Settings(p_Settings), m_State2(p_State2), m_State3(p_State3)
{
    m_Window = m_Application->GetMainWindow();

    m_Context2 = m_Window->GetRenderContext<D2>();
    m_Context3 = m_Window->GetRenderContext<D3>();
}

void IntroLayer::OnRender(const VkCommandBuffer) noexcept
{
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
    Visualization<D>::DrawParticleLattice(p_Context, m_Dimensions, 0.4f * m_Settings.SmoothingRadius,
                                          2.f * m_Settings.ParticleRadius, m_Settings.Gradient[0]);
    Visualization<D>::DrawBoundingBox(p_Context, p_State.Min, p_State.Max,
                                      Onyx::Color::FromHexadecimal("A6B1E1", false));
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

    return false;
}

void IntroLayer::renderIntroSettings() noexcept
{
    ImGui::SetWindowSize({400, 400});
    ImGui::Begin("Welcome to my fluid simulator!");
    const f32 deltaTime = m_Application->GetDeltaTime().AsMilliseconds();
    ImGui::Text("Frame time: %.2f ms", deltaTime);
    ImGui::Text("This is a small project I wanted to make because\nI have always been fascinated by fluid dynamics");
    ImGui::Spacing();

    ImGui::Text("The fluid can be simulated both in 2D and 3D");

    ImGui::Combo("Dimension", &m_Dim,
                 "2D\0"
                 "3D\0\0");

    ImGui::Spacing();

    ImGui::Text(
        "You can choose how many starting particles you want to have by tweaking the settings below.\nNote that "
        "you may add more particles during the simulation by pressing the space bar");

    if (m_Dim == 0)
    {
        ImGui::Text("Current amount: %d", m_Dimensions.x * m_Dimensions.y);
        ImGui::DragInt2("Particles", glm::value_ptr(m_Dimensions), 1, 1, INT32_MAX);
    }
    else
    {
        ImGui::Text("Current amount: %d", m_Dimensions.x * m_Dimensions.y * m_Dimensions.z);
        ImGui::DragInt3("Particles", glm::value_ptr(m_Dimensions), 1, 1, INT32_MAX);
    }

    ImGui::Text("The camera controls are the following:");
    ImGui::Text("W-A-S-D: Move the camera");
    ImGui::Text("Q-E: Rotate the camera");
    ImGui::Text("Left shift + Mouse wheel: Zoom in/out");
    ImGui::Text("Left mouse button: Apply a force to the fluid");

    if (ImGui::Button("Start simulation"))
    {
        if (m_Dim == 0)
            startSimulation(m_State2);
        else
            startSimulation(m_State3);
    }

    if (m_Dim == 0)
        renderBoundingBox(m_State2);
    else
        renderBoundingBox(m_State3);
    ImGui::End();
}

template <Dimension D> void IntroLayer::startSimulation(SimulationState<D> &p_State) noexcept
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
    m_Application->SetUserLayer<SimLayer<D>>(m_Application, m_Settings, p_State);
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

} // namespace Flu