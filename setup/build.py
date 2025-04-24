from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import subprocess
import sys
import shutil

sys.path.append(str(Path(__file__).parent.parent))

from convoy import Convoy


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
        Convoy.exit_error(
            f"No configuration file found at <underline>{root}</underline>."
        )

    with open(path) as f:
        cfg.read_file(f)

    return cfg


def parse_default_values(cfg: ConfigParser, /) -> dict[str, tuple[str, str | bool]]:
    cmake_vname_map = {}
    for cmake_varname, kv in cfg["default-values"].items():
        custom_varname, val = kv.split(": ")
        if custom_varname in cmake_vname_map:
            Convoy.exit_error(
                f"Name mismatch: Variable <bold>{custom_varname}</bold> already exists. Custom variable names must be unique."
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
        "--build-path",
        default=Path("build"),
        type=Path,
        help="The location directory where the project will be built. Default is 'build'.",
    )
    parser.add_argument(
        "-s",
        "--source-path",
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
    parser.add_argument(
        "--fetch-dependencies",
        nargs="+",
        type=str,
        default=None,
        help="Force CMake to re-fetch the specified dependencies. This is useful if you want to re-fetch a dependency without having to delete the entire build directory. Specifying no dependency will re-fetch all dependencies. Default is None.",
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


Convoy.log_label = "BUILD"
cfg = load_build_ini()
cmake_vname_map = parse_default_values(cfg)
(args, unknown), custom_vname_map = parse_arguments(cmake_vname_map)
Convoy.is_verbose = args.verbose

if args.build_command is None and unknown:
    Convoy.exit_error(
        f"Unknown arguments were detected: <bold>{' '.join(unknown)}</bold>. Note that you may only provide unknown arguments when a build command is provided. The unknown arguments will be forwarded to the build command."
    )

if unknown:
    Convoy.verbose(
        f"Unknown arguments detected: <bold>{' '.join(unknown)}</bold>. These will be forwarded to the build command: <bold>{args.build_command}</bold>."
    )

build_path: Path = args.build_path.resolve()
source_path: Path = args.source_path.resolve()

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
            Convoy.exit_error(
                f"Cannot set both <bold>{argname}</bold> and <bold>no_{argname}</bold> to True. Please choose one."
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
                f"Default value override <bold>{custom_varname}</bold> was not found in the <bold>default-values</bold> section."
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
cmake_args.append("-DCMAKE_POLICY_VERSION_MINIMUM=3.5")
build_path.mkdir(exist_ok=True, parents=True)

Convoy.verbose("Running <bold>CMake</bold> with the following arguments:")
for arg in cmake_args:
    Convoy.verbose(f"    {arg}")

refetch: list[str] | None = args.fetch_dependencies
if refetch is not None and (build_path / "_deps").exists():

    def on_error(func, path, _) -> None:
        import stat
        import os

        try:
            os.chmod(path, stat.S_IWRITE)
            func(path)
        except Exception as e:
            Convoy.verbose(
                f"<fyellow>Failed to remove <bold>{path}</bold> due to: <underline>{e}</underline>. Skipping..."
            )

    if not refetch:
        Convoy.verbose("Removing all dependencies to force CMake to re-fetch them...")
        shutil.rmtree(build_path / "_deps")
    else:
        for dep in refetch:
            for thingy in ["build", "src", "subbuild"]:
                path = build_path / "_deps" / f"{dep}-{thingy}"
                if not path.exists():
                    Convoy.verbose(
                        f"<fyellow>Dependency subfolder <bold>{dep}-{thingy}</bold> not found. Skipping..."
                    )
                    continue
                Convoy.verbose(
                    f"Removing dependency subfolder <bold>{dep}-{thingy}</bold> to force CMake to re-fetch it..."
                )
                shutil.rmtree(path, onexc=on_error if Convoy.is_windows else None)

if not Convoy.run_process_success(
    ["cmake", str(source_path)] + cmake_args,
    stdout=subprocess.DEVNULL if not args.verbose else None,
    cwd=build_path,
):
    Convoy.exit_error("Failed to execute <bold>CMake</bold> command.")

if args.build_command is not None:
    bcmd: list[str] = args.build_command.split(" ")
    Convoy.verbose(
        f"Running build command: <bold>{args.build_command} {' '.join(unknown)}"
    )
    if not Convoy.run_process_success(
        bcmd + unknown,
        stdout=subprocess.DEVNULL if not args.verbose else None,
        cwd=build_path,
    ):
        Convoy.exit_error(
            f"Failed to execute build command: <bold>{args.build_command}"
        )

    Convoy.exit_ok(
        f"Build files were generated and the project compiled successfully. You may found the build files at <underline>{build_path}</underline>, along with the compiled binaries."
    )

Convoy.exit_ok(
    f"Build files were generated successfully. You may found the build files at <underline>{build_path}</underline>. As no build command was provided, the project must be compiled manually. You may do so with <bold>make</bold> if you are using a unix-like system or <bold>Visual Studio</bold> if you are using Windows."
)
