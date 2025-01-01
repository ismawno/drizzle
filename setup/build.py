from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import subprocess
import os


def try_convert_bool(val: str) -> bool | str:
    if val == "True":
        return True
    elif val == "False":
        return False
    return val


def parse_build_ini() -> dict[str, tuple[str, str | bool]]:
    cfg = ConfigParser()

    root = Path(__file__).parent
    path = root / "build.ini"
    if not path.exists():
        raise FileNotFoundError(f"No 'build.ini' file found in {root}")

    with open(path) as f:
        cfg.read_file(f)

    parsed = {}
    for ogvarname, kv in cfg["default-values"].items():
        varname, val = kv.split(": ")
        if varname in parsed:
            raise ValueError(
                f"Name mismatch! Variable '{varname}' already exists in the parsed dictionary"
            )

        parsed[ogvarname] = (varname, try_convert_bool(val))

    return parsed


def parse_arguments(
    arguments: dict[str, tuple[str, str | bool]]
) -> tuple[Namespace, dict[str, str | bool]]:
    desc = """
    This script takes in a .ini configuration file created from cmake_scanner.py and
    runs CMake with the options specified in the configuration file, avoiding cache shit.

    This file must be in the same directory as build.ini file from where create the building arguments.
    """

    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-b",
        "--build",
        default=Path("build"),
        type=Path,
        help="The location where the project will be built.",
    )
    parser.add_argument(
        "-s",
        "--source",
        default=Path("."),
        type=Path,
        help="The location where the source files are found (where the root CMakeLists.txt is).",
    )
    parser.add_argument(
        "--disable-toolkit-conditional-logs",
        action="store_true",
        default=False,
        help="Disable conditional logs for the toolkit package if exists. By default, toolkit logging will only be enabled on Dist builds. Disabling this option removes this behavior. Logging can still be enabled by entering the argument in the command line no matter what.",
    )

    arg_map = {}
    for ogvarname, (varname, val) in arguments.items():
        if isinstance(val, bool):
            cmakeval = "ON" if val else "OFF"
            parser.add_argument(
                f"--{varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{ogvarname.upper()}' is set of 'ON'. Its default is '{cmakeval}'.",
            )
            parser.add_argument(
                f"--no-{varname}",
                action="store_true",
                default=False,
                help=f"Ensures that the CMake option '{ogvarname.upper()}' is set of 'OFF'. Its default is '{cmakeval}'.",
            )
        else:
            parser.add_argument(
                f"--{varname}",
                type=str,
                default=val,
                help=f"Set the value of '{ogvarname.upper()}' to the specified value. Default is '{val}'.",
            )

        arg_map[varname.replace("-", "_")] = ogvarname, val
    return parser.parse_args(), arg_map


def main() -> None:
    arguments = parse_build_ini()
    args, arg_map = parse_arguments(arguments)

    build_path: Path = args.build.resolve()
    source_path: Path = args.source.resolve()

    cnd_logs = not args.disable_toolkit_conditional_logs
    arg_dict = vars(args)

    cmake_args = {}
    for k, v in arg_dict.items():
        if k not in arg_map:
            continue

        ogvarname, default = arg_map[k]
        if not isinstance(v, bool):
            cmake_args[f"-D{ogvarname.upper()}"] = v.replace('"', "")
            continue

        nv = arg_dict[f"no_{k}"]
        if v and nv:
            raise ValueError(
                f"Cannot set both '{k}' and 'no_{k}' to True. Please choose one"
            )
        if v:
            cmake_args[f"-D{ogvarname.upper()}"] = "ON"
        elif nv:
            cmake_args[f"-D{ogvarname.upper()}"] = "OFF"
        elif cnd_logs and (
            "log" in ogvarname.lower() or "enable_asserts" in ogvarname.lower()
        ):
            btype = arguments["cmake_build_type"][0]
            btype = arg_dict[btype.replace("-", "_")]
            cmake_args[f"-D{ogvarname.upper()}"] = "ON" if btype != "Dist" else "OFF"
        else:
            cmake_args[f"-D{ogvarname.upper()}"] = "ON" if default else "OFF"

    cmake_args = [f"{k}={v}" for k, v in cmake_args.items()]
    build_path.mkdir(exist_ok=True, parents=True)

    print("Running CMake with the following arguments:")
    for arg in cmake_args:
        print(f"    {arg}")

    os.chdir(build_path)
    subprocess.run(["cmake", str(source_path)] + cmake_args, check=True)


if __name__ == "__main__":
    main()
