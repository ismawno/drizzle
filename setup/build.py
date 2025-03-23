from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import subprocess
import os
import sys


class Style:
    RESET = "\033[0m"
    BOLD = "\033[1m"

    FG_RED = "\033[31m"
    FG_GREEN = "\033[32m"
    FG_YELLOW = "\033[33m"
    FG_BLUE = "\033[34m"
    FG_CYAN = "\033[36m"

    BG_YELLOW = "\033[43m"


def exit_ok(msg: str, /) -> None:
    print(build_label + Style.FG_GREEN + msg + Style.RESET)
    sys.exit()


def exit_error(msg: str, /) -> None:
    print(
        build_label + Style.FG_RED + Style.BOLD + f"Error: {msg}" + Style.RESET,
        file=sys.stderr,
    )
    sys.exit(1)


def try_convert_bool(val: str, /) -> bool | str:
    if val == "True":
        return True
    elif val == "False":
        return False
    return val


def load_build_ini() -> ConfigParser:
    cfg = ConfigParser()

    root = Path(__file__).parent
    path = root / "build.ini"
    if not path.exists():
        exit_error(f"No 'build.ini' file found in '{root}'")

    with open(path) as f:
        cfg.read_file(f)

    return cfg


def parse_default_values(cfg: ConfigParser, /) -> dict[str, tuple[str, str | bool]]:
    cmake_vname_map = {}
    for cmake_varname, kv in cfg["default-values"].items():
        custom_varname, val = kv.split(": ")
        if custom_varname in cmake_vname_map:
            exit_error(
                f"Name mismatch! Variable '{custom_varname}' already exists in the 'cmake_vname_map' dictionary"
            )

        cmake_vname_map[cmake_varname] = (custom_varname, try_convert_bool(val))

    return cmake_vname_map


def parse_arguments(
    cmake_vname_map: dict[str, tuple[str, str | bool]], /
) -> tuple[tuple[Namespace, list[str]], dict[str, tuple[str, str | bool]]]:
    desc = """
    This script takes in a 'build.ini' configuration file created from 'cmake_scanner.py' and
    runs CMake with the options specified in the configuration file, avoiding CMake cache problems.

    All values specified at command line will override the values in the 'build.ini' file and always
    take precedence no matter what.

    This file must be in the same directory as the 'build.ini' file to work.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-b",
        "--build",
        default=Path("build"),
        type=Path,
        help="The location directory where the project will be built. Default is 'build'.",
    )
    parser.add_argument(
        "-s",
        "--source",
        default=Path("."),
        type=Path,
        help="The location directory where the source files are found (where the root CMakeLists.txt is). Default is the curent directory",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )
    parser.add_argument(
        "--build-command",
        type=str,
        default=None,
        help="The build command to run after CMake is done. Such command will be ran from the provided build path. Any unknown arguments will be forwarded to such command. Default is None.",
    )

    custom_vname_map = {}
    for cmake_varname, (custom_varname, val) in cmake_vname_map.items():
        if isinstance(val, bool):
            cmakeval = "ON" if val else "OFF"
            parser.add_argument(
                f"--{custom_varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{cmake_varname.upper()}' is set of 'ON'. Its default is '{cmakeval}'.",
            )
            parser.add_argument(
                f"--no-{custom_varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{cmake_varname.upper()}' is set of 'OFF'. Its default is '{cmakeval}'.",
            )
        else:
            parser.add_argument(
                f"--{custom_varname}",
                type=str,
                default=val,
                help=f"Set the value of '{cmake_varname.upper()}' to the specified value. Default is '{val}'.",
            )

        custom_vname_map[custom_varname.replace("-", "_")] = cmake_varname, val
    return parser.parse_known_args(), custom_vname_map


def log(msg: str, /, *pargs, **kwargs) -> None:
    if args.verbose:
        print(build_label + msg, *pargs, **kwargs)


def run_process_success(*args, **kwargs) -> bool:
    return subprocess.run(*args, **kwargs).returncode == 0


build_label = Style.FG_BLUE + "[BUILD]" + Style.RESET + " "
cfg = load_build_ini()
cmake_vname_map = parse_default_values(cfg)
(args, unknown), custom_vname_map = parse_arguments(cmake_vname_map)

if args.build_command is None and unknown:
    exit_error(
        f"Unknown arguments were detected: '{' '.join(unknown)}'. Note that you may only provide unknown arguments when a build command is provided. The unknown arguments will be forwarded to the build command."
    )

if unknown:
    log(
        f"Unknown arguments detected: '{' '.join(unknown)}'. These will be forwarded to the build command: '{args.build_command}'."
    )

build_path: Path = args.build.resolve()
source_path: Path = args.source.resolve()

parser_args_dict = vars(args)
for argname, argvalue in parser_args_dict.items():
    if argname not in custom_vname_map:
        continue

    cmake_varname, default = custom_vname_map[argname]
    if not isinstance(argvalue, bool):
        value = argvalue
    else:
        nargvalue = parser_args_dict[f"no_{argname}"]
        if argvalue and nargvalue:
            exit_error(
                f"Cannot set both '{argname}' and 'no_{argname}' to True. Please choose one"
            )
        if argvalue:
            value = "True"
        elif nargvalue:
            value = "False"
        else:
            value = "True" if default else "False"

    override_section = f"{cmake_varname.replace('_', '-').lower()}.{value}"
    if override_section not in cfg:
        continue

    overrides = cfg[override_section]
    for custom_varname, new_default in overrides.items():
        custom_varname = custom_varname.replace("-", "_")
        if custom_varname not in custom_vname_map:
            raise ValueError(
                f"Default value override '{custom_varname}' was not found in the 'default-values' section"
            )
        new_value = (
            custom_vname_map[custom_varname][0],
            try_convert_bool(new_default),
        )
        custom_vname_map[custom_varname] = new_value

cmake_args = {}
for argname, argvalue in parser_args_dict.items():
    if argname not in custom_vname_map:
        continue

    cmake_varname, default = custom_vname_map[argname]
    if not isinstance(argvalue, bool):
        cmake_args[f"-D{cmake_varname.upper()}"] = argvalue.replace('"', "")
        continue

    nargvalue = parser_args_dict[f"no_{argname}"]

    if argvalue:
        cmake_args[f"-D{cmake_varname.upper()}"] = "ON"
    elif nargvalue:
        cmake_args[f"-D{cmake_varname.upper()}"] = "OFF"
    else:
        cmake_args[f"-D{cmake_varname.upper()}"] = "ON" if default else "OFF"

cmake_args = [f"{argname}={argvalue}" for argname, argvalue in cmake_args.items()]
cmake_args.append(f"-DTOOLKIT_PYTHON_EXECUTABLE={sys.executable}")
build_path.mkdir(exist_ok=True, parents=True)

log("Running CMake with the following arguments:")
for arg in cmake_args:
    log(f"    {arg}")

os.chdir(build_path)
if not run_process_success(
    ["cmake", str(source_path)] + cmake_args,
    stdout=subprocess.DEVNULL if not args.verbose else None,
):
    exit_error("Failed to execute CMake command.")

if args.build_command is not None:
    os.chdir(build_path)

    bcmd: list[str] = args.build_command.split(" ")
    log(f"Running build command: '{args.build_command} {' '.join(unknown)}'")
    if not run_process_success(
        bcmd + unknown,
        stdout=subprocess.DEVNULL if not args.verbose else None,
    ):
        exit_error(f"Failed to execute build command: '{args.build_command}'")

    exit_ok(
        f"Success! Build files were generated and the project compiled successfully. You may found the build files at '{build_path}', along with the compiled binaries."
    )

exit_ok(
    f"Success! Build files were generated successfully. You may found the build files at '{build_path}'. As no build command was provided, the project must be compiled manually. You may do so with 'make' if you are using a Unix-like system or Visual Studio if you are using Windows."
)
