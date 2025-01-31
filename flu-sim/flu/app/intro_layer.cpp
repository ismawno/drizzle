#include "flu/app/intro_layer.hpp"
#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include <imgui.h>

namespace Flu
{
IntroLayer::IntroLayer(Onyx::Application *p_Application) noexcept : m_Application(p_Application)
{
    m_Window = m_Application->GetMainWindow();

    m_Context2 = m_Window->GetRenderContext<D2>();
    m_Context3 = m_Window->GetRenderContext<D3>();
}

void IntroLayer::OnRender(const VkCommandBuffer) noexcept
{
    if (m_Dim == 0)
    {
        Visualization<D2>::AdjustAndControlCamera(m_Context2, m_Application->GetDeltaTime());
        Visualization<D2>::DrawParticleLattice(m_Context2, m_Dimensions, 2.f * m_Settings.ParticleRadius,
                                               m_Settings.Gradient[0]);
        Visualization<D2>::DrawBoundingBox(m_Context2, m_Bounds2.Min, m_Bounds2.Max,
                                           Onyx::Color::FromHexadecimal("A6B1E1", false));
    }
    else
    {
        Visualization<D3>::AdjustAndControlCamera(m_Context3, m_Application->GetDeltaTime());
        Visualization<D3>::DrawParticleLattice(m_Context3, m_Dimensions, 2.f * m_Settings.ParticleRadius,
                                               m_Settings.Gradient[0]);
        Visualization<D3>::DrawBoundingBox(m_Context3, m_Bounds3.Min, m_Bounds3.Max,
                                           Onyx::Color::FromHexadecimal("A6B1E1", false));
    }
    renderIntroSettings();
}

bool IntroLayer::OnEvent(const Onyx::Event &p_Event) noexcept
{
    if (m_Dim == 0 && p_Event.Type == Onyx::Event::Scrolled &&
        Onyx::Input::IsKeyPressed(m_Window, Onyx::Input::Key::LeftShift))
    {
        m_Context2->ApplyCameraScalingControls(0.005f * p_Event.ScrollOffset.y);
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
            m_Application->SetUserLayer<SimLayer<TKit::D2>>(m_Application, m_Settings, fvec2{m_Dimensions}, m_Bounds2);
        else
            m_Application->SetUserLayer<SimLayer<TKit::D3>>(m_Application, m_Settings, m_Dimensions, m_Bounds3);
    }

    if (m_Dim == 0)
    {
        if (ImGui::TreeNode("Bounding box"))
        {
            if (ImGui::DragFloat("Width", &m_Bounds2.Max.x, 0.05f))
                m_Bounds2.Min.x = -m_Bounds2.Max.x;
            if (ImGui::DragFloat("Height", &m_Bounds2.Max.y, 0.05f))
                m_Bounds2.Min.y = -m_Bounds2.Max.y;
            ImGui::TreePop();
        }
        Visualization<D2>::RenderSettings(m_Settings);
    }
    else
    {
        if (ImGui::TreeNode("Bounding box"))
        {
            if (ImGui::DragFloat("Width", &m_Bounds3.Max.x, 0.05f))
                m_Bounds3.Min.x = -m_Bounds3.Max.x;
            if (ImGui::DragFloat("Height", &m_Bounds3.Max.y, 0.05f))
                m_Bounds3.Min.y = -m_Bounds3.Max.y;
            if (ImGui::DragFloat("Depth", &m_Bounds3.Max.z, 0.05f))
                m_Bounds3.Min.z = -m_Bounds3.Max.z;
            ImGui::TreePop();
        }
        Visualization<D3>::RenderSettings(m_Settings);
    }
    ImGui::End();
}

} // namespace Flu