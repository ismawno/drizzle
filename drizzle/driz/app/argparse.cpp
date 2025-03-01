#include "driz/app/argparse.hpp"
#include "driz/app/intro_layer.hpp"
#include "driz/app/sim_layer.hpp"
#include "onyx/serialization/color.hpp"
#include "tkit/reflection/driz/simulation/settings.hpp"
#include "tkit/serialization/yaml/container.hpp"
#include "tkit/serialization/yaml/glm.hpp"

#include <argparse/argparse.hpp>

namespace Driz
{
static std::string cliName(const char *p_Name) noexcept
{
    std::string result{"--"};
    for (const char *c = p_Name; *c != '\0'; ++c)
    {
        if (std::isupper(*c) && c != p_Name && !std::isupper(c[-1]))
            result.push_back('-');

        result.push_back(std::tolower(*c));
    }

    return result;
}

ParseResult ParseArgs(int argc, char **argv) noexcept
{
    argparse::ArgumentParser parser{"Drizzle", "1.0", argparse::default_arguments::all};

    parser.add_argument("--settings")
        .help("A path pointing to a .yaml file with simulation settings. The file must be compliant with the program's "
              "structure to work.");
    parser.add_argument("--state").help(
        "A path pointing to a .yaml file with the simulation state. The file must be compliant with the program's "
        "structure to work. Trying to load a 2D state in a 3D simulation and vice versa will result in an error.");
    parser.add_argument("--no-intro").flag().help("Skip the intro layer and start the simulation directly.");
    parser.add_argument("-s", "--seconds", "--run-time")
        .scan<'f', f32>()
        .help("The amount of time the simulation will run for in seconds. If not "
              "specified, the simulation will run indefinitely.");

    auto &group = parser.add_mutually_exclusive_group(true);
    group.add_argument("--2-dim").flag().help("Run the simulation in 2D mode.");
    group.add_argument("--3-dim").flag().help("Run the simulation in 3D mode.");

    SimulationSettings settings{};
    TKit::Reflect<SimulationSettings>::ForEachCommandLineField([&parser](const auto &p_Field) {
        using Field = TKit::NoCVRef<decltype(p_Field)>;
        using Type = typename Field::Type;

        argparse::Argument &arg = parser.add_argument(cliName(p_Field.Name));
        if constexpr (std::is_same_v<Type, f32>)
            arg.scan<'f', f32>();

        if constexpr (std::is_enum_v<Type>)
        {
            using EType = std::underlying_type_t<Type>;
            arg.scan<'i', EType>().help(
                TKIT_FORMAT("'SimulationSettings' enum field of type '{}'. You may specify it with an integer.",
                            p_Field.TypeString));
        }
        else
            arg.help(TKIT_FORMAT("'SimulationSettings' field of type '{}'.", p_Field.TypeString));
    });

    try
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ParseResult result{};

    result.Intro = !parser.get<bool>("--no-intro");
    const bool is2D = parser.get<bool>("--2-dim");
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
        result.State2 = SimulationState<D2>{};
        result.State3 = SimulationState<D3>{};
    }

    if (const auto runTime = parser.present<f32>("--run-time"))
    {
        result.RunTime = *runTime;
        result.HasRunTime = true;
    }
    else
        result.HasRunTime = false;

    TKit::Reflect<SimulationSettings>::ForEachCommandLineField([&parser, &settings](const auto &p_Field) {
        using Field = TKit::NoCVRef<decltype(p_Field)>;
        using Type = typename Field::Type;
        if (const auto value = parser.present<Type>(cliName(p_Field.Name)))
            p_Field.Set(settings, *value);
    });

    result.Settings = settings;
    return result;
}
} // namespace Driz