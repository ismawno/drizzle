#include "driz/app/argparse.hpp"
#include "driz/app/intro_layer.hpp"
#include "driz/app/sim_layer.hpp"
#include "onyx/serialization/color.hpp"
#include "tkit/reflection/driz/simulation/settings.hpp"
#include "tkit/reflection/driz/simulation/kernel.hpp"
#include "tkit/serialization/yaml/container.hpp"
#include "tkit/serialization/yaml/tensor.hpp"
#include "tkit/serialization/yaml/driz/simulation/settings.hpp"

#include <argparse/argparse.hpp>

namespace Driz
{
static std::string cliName(const char *p_Name)
{
    std::string result{"--"};
    for (const char *c = p_Name; *c != '\0'; ++c)
    {
        if (std::isupper(*c) && c != p_Name && !std::isupper(c[-1]))
            result.push_back('-');

        result.push_back(static_cast<char>(std::tolower(*c)));
    }

    return result;
}

ParseResult ParseArgs(int argc, char **argv)
{
    argparse::ArgumentParser parser{"drizzle", DRIZ_VERSION, argparse::default_arguments::all};
    parser.add_description(
        "Drizzle is a small project I have made inspired by Sebastian Lague's fluid simulation video. It "
        "features a 2D and 3D fluid simulation using the Smoothed Particle Hydrodynamics method. The "
        "simulation itself is simple, performance oriented and can be simulated both in 2D and 3D.");
    parser.add_epilog("For similar projects, visit my GitHub at https://github.com/ismawno");

    parser.add_argument("--settings")
        .help("A path pointing to a .yaml file with simulation settings. The file must be compliant with the "
              "program's structure to work.");
    parser.add_argument("--state").help(
        "A path pointing to a .yaml file with the simulation state. The file must be compliant with the program's "
        "structure to work. Trying to load a 2D state in a 3D simulation and vice versa will result in an error.");
    parser.add_argument("--no-intro").flag().help("Skip the intro layer and start the simulation directly.");
    parser.add_argument("-s", "--seconds", "--run-time")
        .scan<'f', f32>()
        .help("The amount of time the simulation will run for in seconds. If not "
              "specified, the simulation will run indefinitely.");

    auto &group = parser.add_mutually_exclusive_group();
    group.add_argument("--2-dim").flag().help("Run the simulation in 2D mode.");
    group.add_argument("--3-dim").flag().help("Run the simulation in 3D mode.");

    TKit::Reflect<SimulationSettings>::ForEachCommandLineMemberField([&parser](const auto &p_Field) {
        using Type = TKIT_REFLECT_FIELD_TYPE(p_Field);

        argparse::Argument &arg = parser.add_argument(cliName(p_Field.Name));
        if constexpr (std::is_same_v<Type, f32>)
            arg.scan<'f', f32>();
        else if constexpr (std::is_same_v<Type, u32>)
            arg.scan<'u', u32>();

        if constexpr (std::is_enum_v<Type>)
            arg.help(TKit::Format("'SimulationSettings' enum field of type '{}'. You may specify it with a string.",
                                  p_Field.TypeString));
        else
            arg.help(TKit::Format("'SimulationSettings' field of type '{}'.", p_Field.TypeString));
    });

    parser.parse_args(argc, argv);
    ParseResult result{};

    SimulationSettings settings{};
    result.Intro = !parser.get<bool>("--no-intro");
    const bool noDim = !parser.get<bool>("--2-dim") && !parser.get<bool>("--3-dim");
    if (!result.Intro && noDim)
    {
        std::cerr << "A dimension must be specified when skipping the intro layer.\n";
        std::exit(EXIT_FAILURE);
    }

    const bool is2D = parser.get<bool>("--2-dim") || !parser.get<bool>("--3-dim");
    result.Dim = is2D ? D2 : D3;

    if (const auto path = parser.present("--settings"))
        settings = TKit::Yaml::Deserialize<SimulationSettings>(*path);

    if (const auto path = parser.present("--state"))
    {
        if (is2D)
            result.State2 = TKit::Yaml::Deserialize<SimulationState<D2>>(*path);
        else
            result.State3 = TKit::Yaml::Deserialize<SimulationState<D3>>(*path);
    }
    else if (!result.Intro)
    {
        result.State2.emplace();
        result.State3.emplace();
    }

    if (const auto runTime = parser.present<f32>("--run-time"))
    {
        result.RunTime = *runTime;
        result.HasRunTime = true;
    }
    else
        result.HasRunTime = false;

    TKit::Reflect<SimulationSettings>::ForEachCommandLineMemberField([&parser, &settings](const auto &p_Field) {
        using Type = TKIT_REFLECT_FIELD_TYPE(p_Field);
        if constexpr (std::is_enum_v<Type>)
        {
            if (const auto value = parser.present(cliName(p_Field.Name)))
                p_Field.Set(settings, TKit::Reflect<Type>::FromString(*value));
        }
        else
        {
            if (const auto value = parser.present<Type>(cliName(p_Field.Name)))
                p_Field.Set(settings, *value);
        }
    });

    result.Settings = settings;
    return result;
}
} // namespace Driz
