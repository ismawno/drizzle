#pragma once

#include "driz/core/glm.hpp"
#include "onyx/rendering/render_context.hpp"
#include "onyx/serialization/color.hpp"
#include "onyx/app/user_layer.hpp"
#include "tkit/profiling/timespan.hpp"
#include "tkit/reflection/driz/simulation/settings.hpp"
#include "tkit/serialization/yaml/container.hpp"
#include <imgui.h>

namespace Driz
{
struct SimulationSettings;
template <Dimension D> struct IVisualization
{
    static void AdjustRenderingContext(Onyx::RenderContext<D> *p_Context, TKit::Timespan p_DeltaTime) noexcept;

    static void DrawParticles(Onyx::RenderContext<D> *p_Context, const SimulationSettings &p_Settings,
                              const SimulationState<D> &p_State) noexcept;

    static void DrawBoundingBox(Onyx::RenderContext<D> *p_Context, const fvec<D> &p_Min, const fvec<D> &p_Max,
                                const Onyx::Color &p_Color) noexcept;

    static void DrawCell(Onyx::RenderContext<D> *p_Context, const ivec<D> &p_Position, f32 p_Size,
                         const Onyx::Color &p_Color, f32 p_Thickness = 0.1f) noexcept;

    static void RenderSettings(SimulationSettings &p_Settings) noexcept;
};

template <Dimension D> struct Visualization;

template <> struct Visualization<D2> : IVisualization<D2>
{
    static void DrawMouseInfluence(Onyx::RenderContext<D2> *p_Context, f32 p_Size, const Onyx::Color &p_Color) noexcept;
};

template <> struct Visualization<D3> : IVisualization<D3>
{
    using IVisualization<D3>::DrawParticles;

    static void DrawParticles(Onyx::RenderContext<D3> *p_Context, const SimulationSettings &p_Settings,
                              const SimulationData<D3> &p_Data, const Onyx::Color &p_OutlineHover,
                              const Onyx::Color &p_OutlinePressed) noexcept;
};

template <typename T> void ExportWidget(const char *p_Name, const fs::path &p_DirPath, const T &p_Instance) noexcept
{
    static char xport[64] = {0};
    if (ImGui::InputTextWithHint(p_Name, "Filename", xport, 64, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        fs::path path = p_DirPath / xport;
        if (path.extension().empty())
            path += ".yaml";

        TKit::Yaml::Serialize(path.c_str(), p_Instance);
        xport[0] = '\0';
    }
    Onyx::UserLayer::HelpMarkerSameLine("The file will be saved as a .yaml file. You do not need to include the "
                                        "extension, nor a complete path. A file name is enough.");
}

template <typename T> void ImportWidget(const char *p_Name, const fs::path &p_DirPath, T &p_Instance) noexcept
{
    TKit::StaticArray32<fs::path> paths;
    for (const auto &entry : fs::directory_iterator(p_DirPath))
        paths.push_back(entry.path());

    if (ImGui::BeginMenu(p_Name, !paths.empty()))
    {
        for (const fs::path &path : paths)
        {
            const std::string filename = path.filename().string();

            const bool erase = ImGui::Button("X");
            ImGui::SameLine();
            if (ImGui::MenuItem(filename.c_str()))
                p_Instance = TKit::Yaml::Deserialize<T>(path.c_str());

            if (erase)
                fs::remove(path);
        }
        ImGui::EndMenu();
    }
}

} // namespace Driz