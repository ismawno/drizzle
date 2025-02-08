#include "flu/app/visualization.hpp"
#include "flu/simulation/solver.hpp"
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
void Visualization<D>::DrawParticle(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Position, const f32 p_Size,
                                    const Onyx::Color &p_Color) noexcept
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
void Visualization<D>::DrawMouseInfluence(Onyx::RenderContext<D> *p_Context, const f32 p_Size,
                                          const Onyx::Color &p_Color) noexcept
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
void Visualization<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Min, const fvec<D> &p_Max,
                                       const Onyx::Color &p_Color) noexcept
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
void Visualization<D>::DrawParticleLattice(Onyx::RenderContext<D> *p_Context, const ivec<D> &p_Dimensions,
                                           const f32 p_Separation, const f32 p_ParticleSize,
                                           const Onyx::Color &p_Color) noexcept
{
    const fvec<D> midPoint = 0.5f * p_Separation * fvec<D>{p_Dimensions};
    for (i32 i = 0; i < p_Dimensions.x; ++i)
        for (i32 j = 0; j < p_Dimensions.y; ++j)
            if constexpr (D == D2)
            {
                const f32 x = static_cast<f32>(i) * p_Separation;
                const f32 y = static_cast<f32>(j) * p_Separation;
                Visualization<D2>::DrawParticle(p_Context, fvec2{x, y} - midPoint, p_ParticleSize, p_Color);
            }
            else
                for (i32 k = 0; k < p_Dimensions.z; ++k)
                {
                    const f32 x = static_cast<f32>(i) * p_Separation;
                    const f32 y = static_cast<f32>(j) * p_Separation;
                    const f32 z = static_cast<f32>(k) * p_Separation;
                    Visualization<D3>::DrawParticle(p_Context, fvec3{x, y, z} - midPoint, p_ParticleSize, p_Color);
                }
}

template <Dimension D>
void Visualization<D>::DrawCell(Onyx::RenderContext<D> *p_Context, const ivec<D> &p_Position, const f32 p_Size,
                                const Onyx::Color &p_Color, const f32 p_Thickness) noexcept
{
    p_Context->Fill(p_Color);
    if constexpr (D == D2)
    {
        const ivec2 right = ivec2{p_Size, 0};
        const ivec2 up = ivec2{0, p_Size};
        const ivec2 wopa = ivec2{p_Size, p_Size};

        p_Context->Line(p_Position, p_Position + right, p_Thickness);
        p_Context->Line(p_Position, p_Position + up, p_Thickness);

        p_Context->Line(p_Position + right, p_Position + wopa, p_Thickness);
        p_Context->Line(p_Position + up, p_Position + wopa, p_Thickness);
    }
    else
    {
        const ivec3 right = ivec3{p_Size, 0, 0};
        const ivec3 up = ivec3{0, p_Size, 0};
        const ivec3 front = ivec3{0, 0, p_Size};
        const ivec3 wopa = ivec3{p_Size, p_Size, 0};

        p_Context->Line(p_Position, p_Position + right, p_Thickness);
        p_Context->Line(p_Position, p_Position + up, p_Thickness);

        p_Context->Line(p_Position + right, p_Position + wopa, p_Thickness);
        p_Context->Line(p_Position + up, p_Position + wopa, p_Thickness);

        p_Context->Line(p_Position + front, p_Position + front + right, p_Thickness);
        p_Context->Line(p_Position + front, p_Position + front + up, p_Thickness);

        p_Context->Line(p_Position + front + right, p_Position + front + wopa, p_Thickness);
        p_Context->Line(p_Position + front + up, p_Position + front + wopa, p_Thickness);

        p_Context->Line(p_Position, p_Position + front, p_Thickness);
        p_Context->Line(p_Position + right, p_Position + right + front, p_Thickness);
        p_Context->Line(p_Position + up, p_Position + up + front, p_Thickness);
        p_Context->Line(p_Position + wopa, p_Position + wopa + front, p_Thickness);
    }
}

template <Dimension D> static void comboKenel(const char *name, KernelType &p_Type) noexcept
{
    ImGui::Combo(name, reinterpret_cast<i32 *>(&p_Type),
                 "Spiky2\0Spiky3\0Spiky5\0Cubic Spline\0WendlandC2\0WendlandC4\0\0");
}

template <Dimension D> void Visualization<D>::RenderSettings(SimulationSettings &p_Settings) noexcept
{
    const f32 speed = 0.2f;

    ImGui::Text("Mouse controls:");
    ImGui::DragFloat("Mouse Radius", &p_Settings.MouseRadius, speed);
    ImGui::DragFloat("Mouse Force", &p_Settings.MouseForce, speed);
    ImGui::Spacing();

    ImGui::Text("Particle settings:");
    ImGui::DragFloat("Particle Radius", &p_Settings.ParticleRadius, speed * 0.1f);
    ImGui::DragFloat("Particle Mass", &p_Settings.ParticleMass, speed);
    ImGui::DragFloat("Particle Fast Speed", &p_Settings.FastSpeed, speed);
    ImGui::DragFloat("Smoothing Radius", &p_Settings.SmoothingRadius, speed);
    ImGui::Spacing();

    ImGui::Text("Fluid settings:");
    ImGui::DragFloat("Target Density", &p_Settings.TargetDensity, speed * 0.1f);
    ImGui::DragFloat("Pressure Stiffness", &p_Settings.PressureStiffness, speed);
    ImGui::DragFloat("Near Pressure Stiffness", &p_Settings.NearPressureStiffness, speed);
    ImGui::Spacing();

    ImGui::Text("Viscosity settings:");
    ImGui::DragFloat("Linear Term", &p_Settings.ViscLinearTerm, speed * 0.01f, 0.f, FLT_MAX);
    ImGui::DragFloat("Quadratic Term", &p_Settings.ViscQuadraticTerm, speed * 0.01f, 0.f, FLT_MAX);
    comboKenel<D>("Viscosity kernel", p_Settings.ViscosityKType);

    ImGui::Text("Environment settings:");
    ImGui::DragFloat("Gravity", &p_Settings.Gravity, speed);
    ImGui::DragFloat("Encase Friction", &p_Settings.EncaseFriction, speed);

    ImGui::Text("Kernel settings:");
    comboKenel<D>("Smooth radius kernel", p_Settings.KType);
    comboKenel<D>("Near pressure/density kernel", p_Settings.NearKType);

    ImGui::Text("Optimizations:");
    ImGui::Combo("Lookup mode", reinterpret_cast<i32 *>(&p_Settings.LookupMode),
                 "Brute Force SingleThread\0Brute Force MultiTread\0Grid SingleTread\0Grid MultiTread\0\0");
    ImGui::Combo("Iteration mode", reinterpret_cast<i32 *>(&p_Settings.IterationMode), "Pairwise\0Particlewise\0\0");
    if (p_Settings.UsesMultiThread())
    {
        i32 threads = static_cast<i32>(Core::GetThreadPool().GetThreadCount());
        if (ImGui::SliderInt("Worker thread count", &threads, 0, 15))
            Core::SetWorkerThreadCount(static_cast<u32>(threads));
    }
}

template struct Visualization<D2>;
template struct Visualization<D3>;

} // namespace Flu