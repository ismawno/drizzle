#include "driz/app/visualization.hpp"
#include "tkit/profiling/macros.hpp"

namespace Driz
{
template <Dimension D> void IVisualization<D>::AdjustRenderContext(Onyx::RenderContext<D> *p_Context)
{
    p_Context->Flush();
    p_Context->ScaleAxes(0.025f);

    if constexpr (D == D3)
    {
        p_Context->TranslateZAxis(-20.f);
        p_Context->DirectionalLight(fvec3{0.f, 1.f, 1.f}, 0.4f);
    }
}

template <Dimension D>
void IVisualization<D>::DrawParticles(Onyx::RenderContext<D> *p_Context, const SimulationSettings &p_Settings,
                                      const SimulationState<D> &p_State)
{
    p_Context->ShareCurrentState();
    Core::ForEach(0, p_State.Positions.GetSize(), p_Settings.Partitions,
                  [&, p_Context](const u32 p_Start, const u32 p_End) {
                      TKIT_PROFILE_NSCOPE("Driz::IVisualization::DrawParticles");
                      const f32 psize = 2.f * p_Settings.ParticleRadius;

                      const Onyx::Gradient gradient{p_Settings.Gradient};
                      for (u32 i = p_Start; i < p_End; ++i)
                      {
                          const fvec<D> &pos = p_State.Positions[i];
                          const fvec<D> &vel = p_State.Velocities[i];

                          const f32 speed = glm::min(p_Settings.FastSpeed, glm::length(vel));
                          const Onyx::Color color = gradient.Evaluate(speed / p_Settings.FastSpeed);

                          p_Context->Push();
                          p_Context->Fill(color);

                          p_Context->Translate(pos);
                          if constexpr (D == D2)
                              p_Context->Circle(psize);
                          else
                              p_Context->Sphere(psize, Core::Resolution);

                          p_Context->Pop();
                      }
                  });
}

template <Dimension D>
void IVisualization<D>::DrawBoundingBox(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Min, const fvec<D> &p_Max,
                                        const Onyx::Color &p_Color)
{
    p_Context->Push();

    if constexpr (D == D2)
    {
        p_Context->Fill(false);
        p_Context->Outline(p_Color);
        p_Context->OutlineWidth(0.5f);

        const fvec2 center = 0.5f * (p_Min + p_Max);
        const fvec2 GetSize = p_Max - p_Min;

        p_Context->Translate(center);
        p_Context->Square(GetSize);
    }
    else
    {
        p_Context->Fill(p_Color);
        const fvec3 dims = p_Max - p_Min;
        const fvec3 right = fvec3{dims.x, 0, 0};
        const fvec3 up = fvec3{0, dims.y, 0};
        const fvec3 front = fvec3{0, 0, dims.z};
        const fvec3 wopa = fvec3{dims.x, dims.y, 0};

        const Onyx::LineOptions options{.Thickness = 0.2f, .Resolution = Core::Resolution};
        p_Context->Line(p_Min, p_Min + right, options);
        p_Context->Line(p_Min, p_Min + up, options);

        p_Context->Line(p_Min + right, p_Min + wopa, options);
        p_Context->Line(p_Min + up, p_Min + wopa, options);

        p_Context->Line(p_Min + front, p_Min + front + right, options);
        p_Context->Line(p_Min + front, p_Min + front + up, options);

        p_Context->Line(p_Min + front + right, p_Min + front + wopa, options);
        p_Context->Line(p_Min + front + up, p_Min + front + wopa, options);

        p_Context->Line(p_Min, p_Min + front, options);
        p_Context->Line(p_Min + right, p_Min + right + front, options);
        p_Context->Line(p_Min + up, p_Min + up + front, options);
        p_Context->Line(p_Min + wopa, p_Min + wopa + front, options);

        // p_Context->ScaleX(dims.x);
        // p_Context->ScaleY(dims.z);
        // p_Context->RotateX(glm::half_pi<f32>());
        // p_Context->TranslateY(-0.5f * dims.y);
        // p_Context->Square();
    }

    p_Context->Pop();
}

template <Dimension D>
void IVisualization<D>::DrawCell(Onyx::RenderContext<D> *p_Context, const ivec<D> &p_Position, const f32 p_Size,
                                 const Onyx::Color &p_Color, const f32 p_Thickness)
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

        const Onyx::LineOptions options{.Thickness = p_Thickness, .Resolution = Core::Resolution};
        p_Context->Line(p_Position, p_Position + right, options);
        p_Context->Line(p_Position, p_Position + up, options);

        p_Context->Line(p_Position + right, p_Position + wopa, options);
        p_Context->Line(p_Position + up, p_Position + wopa, options);

        p_Context->Line(p_Position + front, p_Position + front + right, options);
        p_Context->Line(p_Position + front, p_Position + front + up, options);

        p_Context->Line(p_Position + front + right, p_Position + front + wopa, options);
        p_Context->Line(p_Position + front + up, p_Position + front + wopa, options);

        p_Context->Line(p_Position, p_Position + front, options);
        p_Context->Line(p_Position + right, p_Position + right + front, options);
        p_Context->Line(p_Position + up, p_Position + up + front, options);
        p_Context->Line(p_Position + wopa, p_Position + wopa + front, options);
    }
}

static void comboKenel(const char *name, KernelType &p_Type)
{
    ImGui::Combo(name, reinterpret_cast<i32 *>(&p_Type),
                 "Spiky2\0Spiky3\0Spiky5\0Cubic Spline\0WendlandC2\0WendlandC4\0\0");
    Onyx::UserLayer::HelpMarkerSameLine(
        "The kernel is a function that takes a smoothing radius and a distance an returns a value from 0 to 1 that "
        "symbolizes the influence of a particle on another at such given distance. How the kernel and its derivative "
        "behave is crucial for the behavior of the fluid.");
}

template <Dimension D> void IVisualization<D>::RenderSettings(SimulationSettings &p_Settings)
{
    const f32 speed = 0.2f;
    ImGui::TextWrapped(
        "The simulation settings control general parameters for the fluid simulation. Hover over the little (?) icon "
        "to get a brief description of its function.");
    ImGui::TextWrapped("The settings can be exported and imported to and from .yaml files located in the "
                       "'saves/settings' folder, relative to the root of the project.");

    if (ImGui::Button("Load default settings"))
        p_Settings = SimulationSettings{};

    ExportWidget("Export settings", Core::GetSettingsPath(), p_Settings);
    ImportWidget("Import settings", Core::GetSettingsPath(), p_Settings);

    ImGui::Text("Mouse controls");
    Onyx::UserLayer::HelpMarkerSameLine(
        "These settings determine the influence and strength of the mouse on the fluid when you click on the screen.");
    ImGui::DragFloat("Mouse Radius", &p_Settings.MouseRadius, speed);
    ImGui::DragFloat("Mouse Force", &p_Settings.MouseForce, speed);
    ImGui::Spacing();

    ImGui::Text("Particle settings");
    ImGui::DragFloat("Particle Radius", &p_Settings.ParticleRadius, speed * 0.1f);
    Onyx::UserLayer::HelpMarkerSameLine("The visual radius of the particles. Although it is almost purely visual, it "
                                        "does have an impact on wall collisions.");

    ImGui::DragFloat("Particle Mass", &p_Settings.ParticleMass, speed);
    Onyx::UserLayer::HelpMarkerSameLine("The mass of the particles. This value is used to calculate the density of the "
                                        "particles and the forces acting on them. The default of 1.0 is recommended.");

    ImGui::DragFloat("Particle Fast Speed", &p_Settings.FastSpeed, speed);
    Onyx::UserLayer::HelpMarkerSameLine(
        "Particles, when moving, will change color based on their speed. This bigger this value, "
        "the faster a particle needs to move to reach the maximum color.");

    ImGui::DragFloat("Smoothing Radius", &p_Settings.SmoothingRadius, speed);
    Onyx::UserLayer::HelpMarkerSameLine(
        "The radius of the smoothing kernel is likely one of the most important "
        "parameters in the simulation. It determines the range of influence a particle has upon its neighbors.");

    ImGui::Spacing();

    ImGui::Text("Fluid settings");
    ImGui::DragFloat("Target Density", &p_Settings.TargetDensity, speed * 0.1f);
    Onyx::UserLayer::HelpMarkerSameLine("This is the density that the fluid will try to reach. The higher this value, "
                                        "the more compressed the fluid will be.");

    ImGui::DragFloat("Pressure Stiffness", &p_Settings.PressureStiffness, speed);
    Onyx::UserLayer::HelpMarkerSameLine("The stiffness of the pressure force. Lower values will make the fluid more "
                                        "compressible, while higher values will make it more incompressible. Keep in "
                                        "mind that too high values may introduce instabilities.");

    ImGui::DragFloat("Near Pressure Stiffness", &p_Settings.NearPressureStiffness, speed);
    Onyx::UserLayer::HelpMarkerSameLine("An additional 'near' stiffness, used as a small workaround to prevent "
                                        "particles from clustering together. It should be a fraction of the pressure "
                                        "stiffness.");

    comboKenel("Pressure kernel", p_Settings.KType);
    comboKenel("Near pressure/density kernel", p_Settings.NearKType);

    ImGui::Spacing();

    ImGui::Text("Viscosity settings");
    Onyx::UserLayer::HelpMarkerSameLine(
        "The viscosity is an interactive force that tries to equalize the velocities of neighboring "
        "particles. It is useful to prevent particles too fast moving particles from passing through each other.");

    ImGui::DragFloat("Linear Term", &p_Settings.ViscLinearTerm, speed * 0.01f, 0.f, FLT_MAX);
    Onyx::UserLayer::HelpMarkerSameLine("The linear viscotity term operates proportionally to the relative velocity "
                                        "between two particles.");

    ImGui::DragFloat("Quadratic Term", &p_Settings.ViscQuadraticTerm, speed * 0.01f, 0.f, FLT_MAX);
    Onyx::UserLayer::HelpMarkerSameLine(
        "The quadratic viscosity term operates proportionally to the square of the relative velocity "
        "between two particles.");

    comboKenel("Viscosity kernel", p_Settings.ViscosityKType);

    ImGui::Spacing();

    ImGui::Text("Environment settings");
    ImGui::DragFloat("Gravity", &p_Settings.Gravity, speed);
    ImGui::DragFloat("Encase Friction", &p_Settings.EncaseFriction, speed);
    Onyx::UserLayer::HelpMarkerSameLine("How much are the particles slowed down when they collide with the walls.");

    ImGui::Spacing();

    ImGui::Text("Optimizations");
    Onyx::UserLayer::HelpMarkerSameLine(
        "The sole purpose of optimizations is to make the simulation do the same thing, but faster. This requires "
        "writing more efficient code, or explicitly using available hardware (such as multi-threading).");

    ImGui::Combo("Lookup mode", reinterpret_cast<i32 *>(&p_Settings.LookupMode),
                 "Brute Force SingleThread\0Brute Force MultiTread\0Grid SingleTread\0Grid MultiTread\0\0");
    Onyx::UserLayer::HelpMarkerSameLine(
        "The lookup mode is one of the most important optimizations, as it affects the most expensive operation in the "
        "simulation by far: finding neighboring particles. The brute force method mindlessly checks every particle "
        "against every other particle, while the grid method divides the simulation space into cells and only checks "
        "particles within the same cell or neighboring cells. You may also choose the single-threaded or "
        "multi-threaded variants of both.");

    ImGui::Combo("Iteration mode", reinterpret_cast<i32 *>(&p_Settings.IterationMode), "Pairwise\0Particlewise\0\0");
    Onyx::UserLayer::HelpMarkerSameLine(
        "The iteration mode determines how the simulation traverses the lookup data "
        "structure. The pairwise mode iterates over every pair of particles. Its main advantage is that it avoids "
        "redundancy calculations. The particlewise mode iterates over every particle and calculates the forces acting "
        "on it. This mode is more cache-friendly and can be parallelized more easily, specially in GPU-land, but it "
        "introduces a lot of redundant operations.");

    if (p_Settings.UsesMultiThread())
    {
        const u32 mn = 1;
        const u32 mx = DRIZ_MAX_TASKS + 1;
        ImGui::SliderScalar("Worker task count", ImGuiDataType_U32, &p_Settings.Partitions, &mn, &mx);
        Onyx::UserLayer::HelpMarkerSameLine(
            "The number of additional threads that will be used to compute the simulation. Try to match the number of "
            "threads with the number of cores in your CPU.");
    }
}

void Visualization<D2>::DrawMouseInfluence(const Onyx::Camera<D2> *p_Camera, Onyx::RenderContext<D2> *p_Context,
                                           const f32 p_Size, const Onyx::Color &p_Color)
{
    const fvec2 mpos = p_Camera->GetWorldMousePosition(&p_Context->GetCurrentAxes());
    p_Context->Push();
    p_Context->Fill(p_Color);
    p_Context->Translate(mpos);
    p_Context->Circle(p_Size, {.Hollowness = 0.99f});
    p_Context->Pop();
}

void Visualization<D3>::DrawParticles(Onyx::RenderContext<D3> *p_Context, const SimulationSettings &p_Settings,
                                      const SimulationData<D3> &p_Data, const Onyx::Color &p_OutlineHighlight,
                                      const Onyx::Color &p_OutlinePressed)
{
    p_Context->ShareCurrentState();
    Core::ForEach(0, p_Data.State.Positions.GetSize(), p_Settings.Partitions,
                  [&, p_Context](const u32 p_Start, const u32 p_End) {
                      TKIT_PROFILE_NSCOPE("Driz::Visualization<D3>::DrawParticles");
                      const f32 psize = 2.f * p_Settings.ParticleRadius;

                      const Onyx::Gradient gradient{p_Settings.Gradient};
                      for (u32 i = p_Start; i < p_End; ++i)
                      {
                          const fvec3 &pos = p_Data.State.Positions[i];
                          const fvec3 &vel = p_Data.State.Velocities[i];

                          const f32 speed = glm::min(p_Settings.FastSpeed, glm::length(vel));
                          const Onyx::Color color = gradient.Evaluate(speed / p_Settings.FastSpeed);

                          p_Context->Push();
                          if (p_Data.UnderMouseInfluence[i] == 1)
                              p_Context->Outline(p_OutlinePressed);
                          else if (p_Data.UnderMouseInfluence[i] == 2)
                              p_Context->Outline(p_OutlineHighlight);

                          p_Context->Fill(color);
                          p_Context->Translate(pos);
                          p_Context->Sphere(psize, Core::Resolution);

                          p_Context->Pop();
                      }
                  });
}

template struct IVisualization<D2>;
template struct IVisualization<D3>;

} // namespace Driz
