from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser
from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    This script will scan recursively all 'CMakeLists.txt' files it finds under the provided path and will detect
    configuration options (with the help of a little hint), so that it can create a 'build.ini' file to better define
    argument defaults that are consistenly applied and specify them explicitly to nullify the effects of the cache.

    The reason behind this is that CMake sometimes stores some variables in cache that you may not want to persist.
    This results in some default values for variables being only relevant if the variable itself is not already stored in cache.
    The problem with this is that I feel it is very easy to lose track of what configuration is being built unless I type in all
    my CMake flags explicitly every time I build the project, and that is just unbearable. Hence, this python script, along with `build.py`,
    provide flags with reliable defaults stored in a `build.ini` file that are always applied unless explicitly changed with a command line argument.
    
    This script will create a 'build.ini' file in the same directory this file lives, to be used by 'build.py' to do
    the actual building.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-p",
        "--path",
        type=Path,
        required=True,
        help="The path to search for CMakeLists.txt.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )
    parser.add_argument(
        "--hint",
        default="define_option",
        type=str,
        help="A hint to let the scanner now how to look for options. Default is 'define_option'.",
    )
    parser.add_argument(
        "--cmake-name",
        default="CMakeLists.txt",
        type=str,
        help="The name of the CMake file. Default is 'CMakeLists.txt'.",
    )
    parser.add_argument(
        "--keep-preffixes",
        action="store_true",
        default=False,
        help="Keep the preffixes in the option name. Default is to strip them.",
    )
    parser.add_argument(
        "--preffixes",
        action="append",
        # Make sure largest preffixes are first
        default=[
            "TOOLKIT_ENABLE_",
            "VULKIT_ENABLE_",
            "ONYX_ENABLE_",
            "TOOLKIT_",
            "VULKIT_",
            "CMAKE_",
            "ONYX_",
        ],
        help="The preffixes to strip.",
    )
    parser.add_argument(
        "--keep-case",
        action="store_true",
        default=False,
        help="Keep the case of the option name. Default is to lowercase it.",
    )
    parser.add_argument(
        "--keep-underscore",
        action="store_true",
        default=False,
        help="Keep the underscore in the option name. Default is to replace it with a hyphen.",
    )

    return parser.parse_args()


def create_custom_varname(
    cmake_varname: str, /, *, override_strip_preffix: bool = False
) -> str:
    custom_varname = cmake_varname
    if strip_preffix and not override_strip_preffix:
        for preffix in preffixes:
            if custom_varname.startswith(preffix):
                custom_varname = custom_varname.removeprefix(preffix)
    if lower_case:
        custom_varname = custom_varname.lower()
    if dyphen_separator:
        custom_varname = custom_varname.replace("_", "-")
    return custom_varname


def process_option(content: list[str]) -> tuple[str, str, str | bool]:
    cmake_varname, val = content
    Convoy.verbose(
        f"    Detected option <bold>{cmake_varname}</bold> with default value <bold>{val}</bold>."
    )

    custom_varname = create_custom_varname(cmake_varname)

    if val == "ON":
        val = True
    elif val == "OFF":
        val = False

    return cmake_varname, custom_varname, val


Convoy.log_label = "SCANNER"
args = parse_arguments()
Convoy.is_verbose = args.verbose

hint: str = args.hint
cmake_path: Path = args.path
strip_preffix: bool = not args.keep_preffixes
preffixes: list[str] = args.preffixes
lower_case: bool = not args.keep_case
dyphen_separator: bool = not args.keep_underscore

contents = []
for cmake_file in cmake_path.rglob(args.cmake_name):
    Convoy.verbose(
        f"Scanning file at <underline>{cmake_file}</underline>. Looking for lines starting with <bold>{hint}</bold>..."
    )
    with open(cmake_file, "r") as f:
        options = [
            process_option(
                content.removeprefix(f"{hint}(")
                .removesuffix(")")
                .replace('"', "")
                .split(" ")
            )
            for content in f.read().splitlines()
            if content.startswith(hint)
        ]
        contents.extend(options)
    if not options:
        Convoy.verbose("    Nothing found...")

contents = {content[0]: (content[1], content[2]) for content in contents}
unique_varnames = {}

for cmake_varname, (custom_varname, val) in contents.items():
    if custom_varname not in unique_varnames:
        unique_varnames[custom_varname] = cmake_varname
        continue
    Convoy.verbose(
        f"<fyellow>Warning: Found name clash with <bold>{custom_varname}</bold>. Trying to resolve by restoring preffixes..."
    )
    if not strip_preffix:
        Convoy.exit_error(
            f"Name clash with <bold>{custom_varname}</bold> that was not caused by preffix stripping. Aborting..."
        )
    other_cmake_varname = unique_varnames[custom_varname]
    contents[other_cmake_varname] = (
        create_custom_varname(other_cmake_varname, override_strip_preffix=True),
        contents[other_cmake_varname][1],
    )
    contents[cmake_varname] = (
        create_custom_varname(cmake_varname, override_strip_preffix=True),
        contents[cmake_varname][1],
    )
    Convoy.verbose("<fgreen>Name clash resolved!")

root = Path(__file__).parent
cfg = ConfigParser(allow_no_value=True)
cfg.read(root / "build.ini")

sections = {sc: dict(cfg[sc]) for sc in cfg.sections()}
cfg.clear()

cfg.add_section("default-values")
cfg.set(
    "default-values",
    ";This section format goes as follows: <lowercase-cmake-option> = <lowercase-custom-formatted-option>: <default-value>",
)
for cmake_varname, (custom_varname, val) in contents.items():
    cmake_varname = cmake_varname.lower()
    cfg["default-values"][cmake_varname] = (
        f"{custom_varname}: {val}"
        if "default-values" not in sections
        or cmake_varname not in sections["default-values"]
        else sections["default-values"][cmake_varname]
    )

cfg.set(
    "default-values",
    ";You can override default values by creating new sections as follows:",
)
cfg.set("default-values", ";[<lowercase-cmake-option>.<value-that-triggers-override>]")
cfg.set("default-values", ";<lowercase-custom-formatted-option> = <new-default-value>")

for section, contents in sections.items():
    if section == "default-values":
        continue
    cfg.add_section(section)
    for option, value in contents.items():
        cfg[section][option] = value

with open(root / "build.ini", "w") as f:
    cfg.write(f)

Convoy.exit_ok()
