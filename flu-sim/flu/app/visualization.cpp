#include "flu/app/visualization.hpp"
#include "flu/solver/solver.hpp"
#include <imgui.h>

namespace Flu
{
template <Dimension D>
void Visualization<D>::AdjustAndControlCamera(Onyx::RenderContext<D> *p_Context,
                                              const TKit::Timespan p_DeltaTime) noexcept
{
    p_Context->Flush(0.15f, 0.15f, 0.15f);
    p_Context->ScaleAxes(0.05f);

    p_Context->ApplyCameraMovementControls(1.5f * p_DeltaTime);
}

template <Dimension D>
void Visualization<D>::DrawParticle(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Position,
                                    const Onyx::Color &p_Color, const f32 p_Size) noexcept
{
    p_Context->Push();
    p_Context->Fill(p_Color);

    p_Context->Scale(p_Size);
    p_Context->Translate(p_Position);
    p_Context->Circle();

    // p_Context->Pop();
    // p_Context->Push();

    // p_Context->Scale(SmoothingRadius);
    // p_Context->Translate(pos);
    // p_Context->Alpha(0.4f);
    // p_Context->Circle(0.f, 1.f);

    p_Context->Pop();
}

template <Dimension D>
void Visualization<D>::DrawMouseInfluence(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color,
                                          const f32 p_Size) noexcept
{
    const fvec<D> mpos = p_Context->GetMouseCoordinates();
    p_Context->Push();
    p_Context->Fill(p_Color);
    p_Context->Scale(p_Size);
    p_Context->Translate(mpos);
    p_Context->Circle(0.f, 0.f, 0.99f);
    p_Context->Pop();
}

template <Dimension D>
void Visualization<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color,
                                       const fvec<D> &p_Min, const fvec<D> &p_Max) noexcept
{
    p_Context->Push();

    p_Context->Fill(false);
    p_Context->Outline(p_Color);
    p_Context->OutlineWidth(0.02f);

    const fvec<D> center = 0.5f * (p_Min + p_Max);
    const fvec<D> size = p_Max - p_Min;

    p_Context->Scale(size);
    p_Context->Translate(center);
    p_Context->Square();

    p_Context->Pop();
}

template <Dimension D>
void Visualization<D>::DrawParticleLattice(Onyx::RenderContext<D> *p_Context, const Onyx::Color &p_Color,
                                           const ivec<D> &p_Dimensions, const f32 p_Size) noexcept
{
    const f32 size = p_Size * 2.f;
    for (i32 i = -p_Dimensions.x / 2; i < p_Dimensions.x / 2; ++i)
        for (i32 j = -p_Dimensions.y / 2; j < p_Dimensions.y / 2; ++j)
            if constexpr (D == D2)
            {
                const f32 x = static_cast<f32>(i) * size;
                const f32 y = static_cast<f32>(j) * size;
                Visualization<D2>::DrawParticle(p_Context, fvec2{x, y}, p_Color, p_Size);
            }
            else
                for (i32 k = -p_Dimensions.z / 2; k < p_Dimensions.z / 2; ++k)
                {
                    const f32 x = static_cast<f32>(i) * size;
                    const f32 y = static_cast<f32>(j) * size;
                    const f32 z = static_cast<f32>(k) * size;
                    Visualization<D3>::DrawParticle(p_Context, fvec3{x, y, z}, p_Color, p_Size);
                }
}

template <Dimension D> void Visualization<D>::RenderSettings(SimulationSettings &p_Settings) noexcept
{
    const f32 speed = 0.2f;
    if (ImGui::CollapsingHeader("Simulation parameters"))
    {

        ImGui::Text("Mouse controls:");
        ImGui::DragFloat("Mouse Radius", &p_Settings.MouseRadius, speed);
        ImGui::DragFloat("Mouse Force", &p_Settings.MouseForce, speed);
        ImGui::Spacing();

        ImGui::Text("Particle settings:");
        ImGui::DragFloat("Particle Radius", &p_Settings.ParticleRadius, speed);
        ImGui::DragFloat("Particle Mass", &p_Settings.ParticleMass, speed);
        ImGui::DragFloat("Particle Fast Speed", &p_Settings.FastSpeed, speed);
        ImGui::DragFloat("Smoothing Radius", &p_Settings.SmoothingRadius, speed);
        ImGui::Spacing();

        ImGui::Text("Fluid settings:");
        ImGui::DragFloat("Target Density", &p_Settings.TargetDensity, 0.1f * speed);
        ImGui::DragFloat("Pressure Stiffness", &p_Settings.PressureStiffness, speed);
        ImGui::DragFloat("Near Pressure Stiffness", &p_Settings.NearPressureStiffness, speed);
        ImGui::Spacing();

        ImGui::Text("Environment settings:");
        ImGui::DragFloat("Gravity", &p_Settings.Gravity, speed);
        ImGui::DragFloat("Encase Friction", &p_Settings.EncaseFriction, speed);

        ImGui::Text("Simulation settings:");
        ImGui::Combo("Neighbor Search", reinterpret_cast<i32 *>(&p_Settings.SearchMethod), "Brute Force\0Grid\0\0");
        ImGui::Combo("Kernel Type", reinterpret_cast<i32 *>(&p_Settings.KType),
                     "Spiky2\0Spiky3\0Spiky5\0Cubic Spline\0WendlandC2\0WendlandC4\0\0");
        ImGui::Combo("Near Kernel Type", reinterpret_cast<i32 *>(&p_Settings.NearKType),
                     "Spiky2\0Spiky3\0Spiky5\0Cubic Spline\0WendlandC2\0WendlandC4\0\0");
    }
}

template struct Visualization<D2>;
template struct Visualization<D3>;

} // namespace Flu