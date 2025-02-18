from pathlib import Path
from argparse import ArgumentParser, Namespace
from configparser import ConfigParser

import subprocess
import os


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
        raise FileNotFoundError(f"No 'build.ini' file found in {root}")

    with open(path) as f:
        cfg.read_file(f)

    return cfg


def parse_default_values(cfg: ConfigParser, /) -> dict[str, tuple[str, str | bool]]:
    cmake_vname_map = {}
    for cmake_varname, kv in cfg["default-values"].items():
        custom_varname, val = kv.split(": ")
        if custom_varname in cmake_vname_map:
            raise ValueError(
                f"Name mismatch! Variable '{custom_varname}' already exists in the 'cmake_vname_map' dictionary"
            )

        cmake_vname_map[cmake_varname] = (custom_varname, try_convert_bool(val))

    return cmake_vname_map


def parse_arguments(
    cmake_vname_map: dict[str, tuple[str, str | bool]], /
) -> tuple[Namespace, dict[str, tuple[str, str | bool]]]:
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
    return parser.parse_args(), custom_vname_map


def main() -> None:
    cfg = load_build_ini()
    cmake_vname_map = parse_default_values(cfg)
    args, custom_vname_map = parse_arguments(cmake_vname_map)

    def log(*pargs, **kwargs) -> None:
        if args.verbose:
            print(*pargs, **kwargs)

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
                raise ValueError(
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
    build_path.mkdir(exist_ok=True, parents=True)

    log("Running CMake with the following arguments:")
    for arg in cmake_args:
        log(f"    {arg}")

    os.chdir(build_path)
    subprocess.run(
        ["cmake", str(source_path)] + cmake_args,
        check=True,
        stdout=subprocess.DEVNULL if not args.verbose else None,
    )


if __name__ == "__main__":
    main()
