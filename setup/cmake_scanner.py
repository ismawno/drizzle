from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import sys

sys.path.append(str(Path(__file__).parent.parent))

from convoy import Convoy


def parse_arguments() -> Namespace:
    desc = """
    This script will scan recursively all 'CMakeLists.txt' files it finds under the provided path and will detect
    configuration options (with the help of a little hint), so that it can create a 'build.ini' file to better define
    argument defaults that are consistenly applied and specify them explicitly to nullify the effects of the CMake cache.

    If no path is provided, this script will only generate a template 'build.ini' file.

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
        default=None,
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
        help="Keep the preffixes in the command line option names for the 'build.py' script. Default is to strip them.",
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
        help="The preffixes to strip for the command line options for the 'build.py' script.",
    )

    return parser.parse_args()


def create_cli_varname(cmake_varname: str, /, *, override_strip_preffix: bool = False) -> str:
    cli_varname = cmake_varname
    if strip_preffix and not override_strip_preffix:
        for preffix in preffixes:
            if cli_varname.startswith(preffix):
                cli_varname = cli_varname.removeprefix(preffix)

    return cli_varname.lower().replace("_", "-")


def process_option(content: str) -> tuple[str, str, str, str | bool]:
    vals = Convoy.nested_split(content, " ", openers='"', closers='"')

    Convoy.verbose(f"    Extracted variable information: <bold>{', '.join(vals)}</bold> from <bold>{content}</bold>.")
    vals = [v.replace('"', "") for v in vals]
    if len(vals) == 3:
        cmake_varname, val, section = vals
    else:
        cmake_varname, val = vals
        section = "General settings"
        Convoy.warning(f"   Failed to extract variable section.")
    Convoy.verbose(
        f"    Detected option <bold>{cmake_varname}</bold> with default value <bold>{val}</bold> and section <bold>{section}</bold>."
    )

    cli_varname = create_cli_varname(cmake_varname)

    if val == "ON":
        val = True
    elif val == "OFF":
        val = False

    return section, cmake_varname.lower().replace("_", "-"), cli_varname, val


Convoy.program_label = "SCANNER"
args = parse_arguments()
Convoy.is_verbose = args.verbose

hint: str = args.hint
cmake_path: Path | None = args.path
strip_preffix: bool = not args.keep_preffixes
preffixes: list[str] = args.preffixes


contents = None
per_section = None
if cmake_path is not None:
    contents = {}
    per_section = {}
    for cmake_file in cmake_path.rglob(args.cmake_name):
        Convoy.verbose(
            f"Scanning file at <underline>{cmake_file}</underline>. Looking for lines starting with <bold>{hint}</bold>..."
        )
        with open(cmake_file, "r") as f:
            for opt in [
                process_option(content.removeprefix(f"{hint}(").removesuffix(")"))
                for content in f.read().splitlines()
                if content.startswith(hint)
            ]:
                contents[opt[1]] = opt[2:]
                per_section.setdefault(opt[0], {})[opt[1]] = opt[2:]

    unique_varnames = {}

    for cmake_varname, (cli_varname, val) in contents.items():
        if cli_varname not in unique_varnames:
            unique_varnames[cli_varname] = cmake_varname
            continue
        Convoy.warning(
            f"Found name clash with <bold>{cli_varname}</bold>. Trying to resolve by restoring preffixes..."
        )
        if not strip_preffix:
            Convoy.exit_error(
                f"Name clash with <bold>{cli_varname}</bold> that was not caused by preffix stripping. Aborting..."
            )
        other_cmake_varname = unique_varnames[cli_varname]
        contents[other_cmake_varname] = (
            create_cli_varname(other_cmake_varname, override_strip_preffix=True),
            contents[other_cmake_varname][1],
        )
        contents[cmake_varname] = (
            create_cli_varname(cmake_varname, override_strip_preffix=True),
            contents[cmake_varname][1],
        )
        Convoy.verbose("<fgreen>Name clash resolved!")

root = Path(__file__).parent
cfg = ConfigParser(allow_no_value=True)
cfg.read(root / "build.ini")

sections = {sc: dict(cfg[sc]) for sc in cfg.sections()}
cfg.clear()


cfg.add_section("cmake-options")


def write_help(msg: str, /, *, newline: bool = False) -> None:
    if newline:
        cfg.set("cmake-options", f"\n;{msg}")
    else:
        cfg.set("cmake-options", f";{msg}")


write_help("This file was generated by the 'cmake_scanner.py' script.")
write_help(
    "You may add/edit key-value pairs in this section and changes will be reflected as long as the format is correct. Running 'cmake_scanner.py' again will not override your changes. Other sections you add will also remain untouched. All keys/section names should be lower case, except when creating override sections (explained above).",
    newline=True,
)
write_help(
    "Hyphen ('-') and underscore ('_') separators are interchangeable in this file. You may use either. The CMake options will always end up with underscores, but the command line options will be hyphenated, no matter what."
)
write_help(
    "This section format goes as follows: <lowercase-cmake-option> = <lowercase-build-cli-option>: <default-value>",
    newline=True,
)
write_help("You may write <lowercase-cmake-option> = <default-value> if you wish to use the same name for both.")
write_help(
    "<lowercase-cmake-option> -> It is the raw CMake option name in lower case (python parser requires it), except for the '-D' prefix."
)
write_help(
    "<lowercase-build-cli-option> -> It is the corresponding 'ergonomic' command line option for the 'build.py' script. It is just a convenience."
)
write_help(
    "<default-value> -> It is the value 'build.py' will use if no other value is provided through the command line. If the value is a boolean, you may use 'true/on/yes' or 'false/off/no'. Case insensitive."
)

if per_section is not None:
    for section, cnt in per_section.items():
        write_help(section, newline=True)
        for cmake_varname, (cli_varname, val) in cnt.items():
            cfg["cmake-options"][cmake_varname] = (
                f"{cli_varname}: {val}"
                if "cmake-options" not in sections or cmake_varname not in sections["cmake-options"]
                else sections["cmake-options"][cmake_varname]
            )

if "cmake-options" in sections:
    for k, v in sections["cmake-options"].items():
        if k not in cfg["cmake-options"]:
            cfg["cmake-options"][k] = v


write_help(
    "You can override default values for some options when other options take specific values by creating new sections as follows:",
    newline=True,
)
write_help("[<lowercase-cmake-option>.<value-that-triggers-override>]")
write_help("<lowercase-build-cli-option> = <new-default-value>")
write_help("This is useful when you would like to disable assertions or logs for distribution builds.")
write_help("You can stack empty override sections to have those inherit the values of the bottom section:")
write_help("[<lowercase-cmake-option-1>.<value-that-triggers-override-1>]")
write_help("[<lowercase-cmake-option-2>.<value-that-triggers-override-2>]")
write_help("<lowercase-build-cli-option> = <new-default-value>")
write_help(
    "<value-that-triggers-override> may have uppercase letters, as well as <new-default-value> or <default-value>."
)


for section, cnts in sections.items():
    if section == "cmake-options":
        continue
    cfg.add_section(section)
    for option, value in cnts.items():
        cfg[section][option] = value

with open(root / "build.ini", "w") as f:
    cfg.write(f)

Convoy.exit_ok()
