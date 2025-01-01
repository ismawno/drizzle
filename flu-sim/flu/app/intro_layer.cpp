#include "flu/app/intro_layer.hpp"
#include "flu/app/sim_layer.hpp"
#include "flu/app/visualization.hpp"
#include <imgui.h>

namespace Flu
{
IntroLayer::IntroLayer(Onyx::Application *p_Application) noexcept
    : Onyx::Layer("Intro Layer"), m_Application(p_Application)
{
    m_Context2 = m_Application->GetMainWindow()->GetRenderContext<D2>();
    m_Context3 = m_Application->GetMainWindow()->GetRenderContext<D3>();
}

void IntroLayer::OnRender(const VkCommandBuffer) noexcept
{
    if (m_Dim == 0)
    {
        Visualization<D2>::AdjustAndControlCamera(m_Context2, m_Application->GetDeltaTime());
        Visualization<D2>::DrawParticleLattice(m_Context2, m_Settings.Gradient[0], m_Dimensions,
                                               2.f * m_Settings.ParticleRadius);
    }
    else
    {
        Visualization<D3>::AdjustAndControlCamera(m_Context3, m_Application->GetDeltaTime());
        Visualization<D3>::DrawParticleLattice(m_Context3, m_Settings.Gradient[0], m_Dimensions,
                                               2.f * m_Settings.ParticleRadius);
    }
    renderIntroSettings();
}

void IntroLayer::OnRemoval() noexcept
{
    if (m_Dim == 0)
        m_Application->Layers.Push<SimLayer<TKit::D2>>(m_Application, m_Settings, fvec2{m_Dimensions});
    else
        m_Application->Layers.Push<SimLayer<TKit::D3>>(m_Application, m_Settings, m_Dimensions);
}

void IntroLayer::renderIntroSettings() noexcept
{
    ImGui::SetWindowSize({400, 400});
    ImGui::Begin("Welcome to my fluid simulator!");
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
        ImGui::DragInt2("Particles", glm::value_ptr(m_Dimensions), 1, 1, 100);
    }
    else
    {
        ImGui::Text("Current amount: %d", m_Dimensions.x * m_Dimensions.y * m_Dimensions.z);
        ImGui::DragInt3("Particles", glm::value_ptr(m_Dimensions), 1, 1, 100);
    }

    if (m_Dim == 0)
        Visualization<D2>::RenderSettings(m_Settings);
    else
        Visualization<D3>::RenderSettings(m_Settings);

    ImGui::Text("The camera controls are the following:");
    ImGui::Text("W-A-S-D: Move the camera");
    ImGui::Text("Q-E: Rotate the camera");
    ImGui::Text("Left shift + Mouse wheel: Zoom in/out");
    ImGui::Text("Left mouse button: Apply a force to the fluid");

    if (ImGui::Button("Start simulation"))
        m_Application->Layers.FlagRemove(this);
    ImGui::End();
}

} // namespace Flu