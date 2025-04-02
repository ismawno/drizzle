from argparse import ArgumentParser, Namespace
from pathlib import Path
from dataclasses import dataclass
from collections.abc import Callable
from convoy import Convoy

import sys
import os
import subprocess
import importlib.util
import shutil
import functools

import zipfile
import tarfile

import re


def parse_arguments() -> tuple[Namespace, list[str]]:
    desc = """
    This is a general installation scripts designed to download and install various recurring
    dependencies for my projects across different operating systems. The supported operating systems are Windows, Linux (Ubuntu, Fedora and Arch), and MacOS.

    This script will generate an 'install-list.txt' so that when passing the '--uninstall' flag, only the dependencies installed by this script are removed.
    This is to prevent the removal of other dependencies that may have been installed manually.

    If python packages required by these scripts are also to be uninstalled, use the '--uninstall-python-packages' flag once all other dependencies have been uninstalled.

    WARNING: This kind of scripts are hard to test due to the vast amount of different configurations and setups.
    I, particularly, do not have access to all of them. That is why I have provided a safe mode ('-s', '--safe') that will prompt
    the user before executing any command. This way, the user can review the commands before they are executed. Because this
    script will attempt to install system-wide dependencies, it is recommended to run it with caution. This is especially important
    for linux operating systems, as I do not possess one to test the script on.
    """

    parser = ArgumentParser(description=desc)

    parser.add_argument(
        "-s",
        "--safe",
        action="store_true",
        default=False,
        help="Prompt before executing commands. This setting cannot be used with the '-y' flag.",
    )
    parser.add_argument(
        "-y",
        "--yes",
        action="store_true",
        default=False,
        help="Skip all prompts and proceed with everything.",
    )
    if Convoy.is_linux:
        parser.add_argument(
            "--linux-devtools",
            dest="manage_linux_devtools",
            action="store_true",
            default=False,
            help="Install or uninstall Linux development tools, such as 'build-essential' on Ubuntu, 'Development Tools' on Fedora, or 'base-devel' on Arch.",
        )
    if Convoy.is_macos:
        parser.add_argument(
            "--xcode-command-line-tools",
            dest="manage_xcode_command_line_tools",
            action="store_true",
            default=False,
            help="Install or uninstall the Xcode Command Line Tools.",
        )
        parser.add_argument(
            "--brew",
            dest="manage_brew",
            action="store_true",
            default=False,
            help="Install or uninstall 'Homebrew'.",
        )

    parser.add_argument(
        "--vulkan-sdk",
        dest="manage_vulkan_sdk",
        action="store_true",
        default=False,
        help="Install or uninstall the Vulkan SDK.",
    )
    parser.add_argument(
        "--cmake",
        dest="manage_cmake",
        action="store_true",
        default=False,
        help="Install or uninstall CMake.",
    )
    parser.add_argument(
        "--uninstall",
        action="store_true",
        default=False,
        help="Trigger the uninstallation of the selected dependencies.",
    )
    parser.add_argument(
        "--uninstall-python-packages",
        action="store_true",
        default=False,
        help="Uninstall python packages needed by this script.",
    )
    parser.add_argument(
        "--ignore-install-list",
        action="store_true",
        default=False,
        help="Ignore the installation list when uninstalling, and uninstall only the dependencies explicitly marked by command line. The installation list is a temp file that keeps track of the dependencies installed by this script, so that when uninstalling, only those dependencies are removed.",
    )
    parser.add_argument(
        "--build-script",
        type=Path,
        default=None,
        help="Path to the build script to run after the setup. If provided, unknown arguments will be passed to the build script. Default is None.",
    )
    parser.add_argument(
        "-f",
        "--force",
        "--force-install",
        action="store_true",
        default=False,
        help="Force the installation of dependencies without checking if they are already installed.",
    )

    parser.add_argument(
        "--vulkan-version",
        type=str,
        default="1.3.250.1",
        help="The Vulkan SDK version to install. Default is '1.3.250.1'.",
    )
    if Convoy.is_windows:
        parser.add_argument(
            "--visual-studio",
            dest="manage_visual_studio",
            action="store_true",
            default=False,
            help="Install or uninstall Visual Studio Community Edition.",
        )
        parser.add_argument(
            "--visual-studio-version",
            type=str,
            default="17",
            help="The Visual Studio version to install. Default is '17'.",
        )
        parser.add_argument(
            "--cmake-version",
            type=str,
            default="3.21.3",
            help="The CMake version to install. This setting is only applicable to Windows. In other OS, the latest version is installed through a package manager. Default is '3.21.3'.",
        )

    return parser.parse_known_args()


@dataclass(frozen=True)
class VulkanVersion:
    major: int
    minor: int
    patch: int
    micro: int

    def no_micro(self) -> str:
        return f"{self.major}.{self.minor}.{self.patch}"

    def __str__(self) -> str:
        return f"{self.major}.{self.minor}.{self.patch}.{self.micro}"

    def __repr__(self) -> str:
        return f"VulkanVersion({self.major}, {self.minor}, {self.patch}, {self.micro})"


def step(msg: str, /) -> Callable:
    def decorator(func: Callable) -> Callable:

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            Convoy.log(f"<bold>{msg}")
            Convoy.push_indent()
            result = func(*args, **kwargs)
            Convoy.log("<fgreen>Ok.")
            Convoy.pop_indent()
            Convoy.log("")
            return result

        return wrapper

    return decorator


@step("--Validating arguments--")
def validate_arguments() -> None:
    if g_args.safe and g_args.yes:
        Convoy.exit_error(
            "The <bold>--safe</bold> and <bold>--yes</bold> flags cannot be used together."
        )

    if (
        not g_args.uninstall
        and not g_args.uninstall_python_packages
        and g_args.ignore_install_list
    ):
        Convoy.exit_error(
            "The <bold>--ignore-install-list</bold> flag can only be used with the <bold>--uninstall*</bold> flags."
        )

    def explicit_mark() -> list[str]:
        present = []
        for key, value in vars(g_args).items():
            if key.startswith("manage_") and value:
                present.append(f"--{key[7:].replace('_', '-')}")
        return present

    if g_args.uninstall and not g_args.ignore_install_list and explicit_mark():
        explicit = explicit_mark()
        Convoy.exit_error(
            f"Found explicitly marked dependencies: <bold>{' '.join(explicit)}</bold>. The <bold>--ignore-install-list</bold> flag must be used when explicitly marking dependencies for uninstallation. Otherwise, the installation list will be used to determine the dependencies to uninstall."
        )

    if g_args.uninstall_python_packages and (g_args.uninstall or explicit_mark()):
        explicit = explicit_mark()
        error = (
            f"Foud explicitly marked dependencies: <bold>{' '.join(explicit)}</bold>. "
            if explicit
            else ""
        )
        Convoy.exit_error(
            f"{error}Some python packages are required by this script to install/uninstall certain dependencies. You may not attempt to uninstall those packages while trying to install/uninstall other dependencies at the same time."
        )

    if not explicit_mark() and (not g_args.uninstall or g_args.ignore_install_list):
        if not g_args.ignore_install_list:
            Convoy.log(
                "<fyellow>No explicit dependencies marked to install. Use the command line arguments to specify them."
            )
        else:
            Convoy.log(
                "<fyellow>When using the <bold>--ignore-install-list</bold> argument, dependencies to uninstall must be marked explicitly."
            )

    if g_args.force and g_args.uninstall:
        Convoy.exit_error(
            "The <bold>--force</bold> flag cannot be used with the <bold>--uninstall</bold> flag. The former is only used to force installations."
        )

    if g_args.uninstall and g_args.build_script is not None:
        Convoy.exit_error(
            "The <bold>--uninstall</bold> flag cannot be used with the <bold>--build-script</bold> flag. The latter is used to run a build script after the setup. <bold>--uninstall</bold> is used to uninstall dependencies once the project is meant to be removed."
        )

    if g_args.build_script is None and g_unknown:
        Convoy.exit_error(
            f"Unknown arguments were detected: <bold>{' '.join(g_unknown)}</bold>. Note that you may only provide unknown arguments when a build script is provided. The unknown arguments will be forwarded to the build script."
        )

    if g_unknown:
        Convoy.log(
            f"Unknown arguments detected: <bold>{' '.join(g_unknown)}</bold>. These will be forwarded to the build script at <underline>{g_args.build_script}</underline>."
        )

    if not g_args.safe:
        Convoy.log(
            f"<fyellow>Safe mode is disabled. This script may execute commands without prompting the user. Safe mode can be enabled with the <bold>-s</bold> or <bold>--safe</bold> flag."
        )
        if not Convoy.prompt("Do you wish to continue?"):
            Convoy.exit_error()


@step("--Validating OS--")
def validate_operating_system() -> None:
    if not Convoy.is_admin and not Convoy.is_macos:
        Convoy.exit_error(
            "This script requires administrative privileges to run. Execute it "
            + (
                "using the <bold>sudo</bold> keyword."
                if not Convoy.is_windows
                else "using an elevated shell."
            )
        )
    elif Convoy.is_admin and Convoy.is_macos:
        Convoy.exit_error(
            "This script must not be explicitly executed with administrative privileges on MacOS (no <bold>sudo</bold> keyword). Such privileges will be requested when necessary. This is because <bold>Homebrew</bold> commands do not support administrative privileges."
        )

    os = Convoy.operating_system
    Convoy.log(f"Operating system: {os}")
    if not Convoy.is_macos and not Convoy.is_windows and not Convoy.is_linux:
        Convoy.exit_error(f"Unsupported operating system: {os}")

    if Convoy.is_windows:
        Convoy.log(f"Architecture: {Convoy.architecure}")
        Convoy.log(f"ARM: {Convoy.is_arm}")

    if Convoy.is_linux:
        global g_linux_devtools, g_linux_dependencies

        Convoy.log(f"Distribution: {g_linux_distro} {g_linux_version}")
        if g_linux_distro is None:
            Convoy.exit_error("Failed to detect Linux distribution.")

        if g_linux_version is None:
            Convoy.exit_error("Failed to detect Linux version.")

        if g_linux_distro not in ["ubuntu", "fedora", "arch"]:
            Convoy.exit_error(
                f"Unsupported Linux distribution: <bold>{g_linux_distro}</bold>."
            )

        if (
            g_linux_distro == "ubuntu"
            and g_linux_version != "20.04"
            and g_linux_version != "22.04"
        ):
            Convoy.exit_error(
                f"Unsupported Ubuntu version: <bold>{g_linux_version}</bold>."
            )

        if (
            g_linux_distro == "fedora"
            and g_linux_version != "36"
            and g_linux_version != "37"
        ):
            Convoy.exit_error(
                f"Unsupported Fedora version: <bold>{g_linux_version}</bold>."
            )

        if g_linux_distro == "ubuntu":
            g_linux_devtools = "build-essential"
            g_linux_dependencies = [
                "qtbase5-dev" if g_linux_version == "22.04" else "qt5-default",
                "libxcb-xinput0",
                "libxcb-xinerama0",
            ]
        elif g_linux_distro == "fedora":
            g_linux_devtools = "Development Tools"
            g_linux_dependencies = ["qt", "xinput", "libXinerama"]
        elif g_linux_distro == "arch":
            g_linux_devtools = "base-devel"
            g_linux_dependencies = ["qt5-base", "libxcb", "libxinerama"]


def prompt_to_install(dependency: str, /) -> bool:
    return Convoy.prompt(
        f"<bold>{dependency}</bold> marked for installation. Do you wish to continue?"
    )


def prompt_to_uninstall(dependency: str, /) -> bool:
    return Convoy.prompt(
        f"<bold>{dependency}</bold> marked for uninstallation. Do you wish to continue?"
    )


@dataclass(frozen=True)
class InstallList:
    dependencies: list[str]

    def has_dependency(self, dependency: str, /) -> bool:
        for dep in self.dependencies:
            if dep.startswith(dependency):
                return True
        return False

    def packages(self, kind: str, /) -> list[str]:
        return [
            dep.split(": ")[1].strip("\n")
            for dep in self.dependencies
            if dep.startswith(f"{kind}-package: ")
        ]

    def find_versions(self, dependency: str, /) -> list[str]:
        versions = []
        for dep in self.dependencies:
            if dep.startswith(f"{dependency} = "):
                versions.append(dep.split(" = ")[1].strip("\n"))
        return versions


def read_install_list() -> InstallList:
    path = g_root / "install-list.txt"
    if not path.exists():
        Convoy.exit_error(
            f"Cannot proceed with uninstallation: Install list not found at <underline>{path}</underline>. This may be because this script has not installed anything yet, or you have deleted the file. If the latter is the case, you may uninstall dependencies manually by specifying them through the command line and entering <bold>--ignore-install-list</bold>."
        )

    Convoy.log(f"Install list found at <underline>{path}</underline>.")
    with open(path, "r") as f:
        return InstallList(f.readlines())


def write_install_list(dependency: str, /) -> None:
    path = g_root / "install-list.txt"

    exists = path.exists()
    with open(path, "r+" if exists else "w") as f:
        text = f.read() if exists else ""
        if dependency not in text:
            f.write(f"{dependency}\n")

    global g_needs_restart
    g_needs_restart = True


@step("--Validating python version--")
def validate_python_version(req_major: int, req_minor: int, req_micro: int, /) -> None:
    major, minor, micro = (
        sys.version_info.major,
        sys.version_info.minor,
        sys.version_info.micro,
    )
    for required, current in zip(
        [req_major, req_minor, req_micro], [major, minor, micro]
    ):
        if current < required:
            Convoy.exit_error(
                f"Python version required: <bold>{req_major}.{req_minor}.{req_micro}</bold> Python version found: <bold>{major}.{minor}.{micro}</bold>.",
            )
        elif current > required:
            Convoy.log(
                f"Valid python version detected: <bold>{major}.{minor}.{micro}</bold>."
            )
            return
    Convoy.log(f"Valid python version detected: <bold>{major}.{minor}.{micro}</bold>.")


def is_python_package_installed(package: str, /) -> bool:
    if importlib.util.find_spec(package) is None:
        Convoy.log(f"<fyellow>Package <bold>{package}</bold> not found.")
        return False

    Convoy.log(f"Package <bold>{package}</bold> found.")
    return True


def try_install_python_package(package: str, /) -> bool:
    Convoy.log(f"Installing python package <bold>{package}</bold>...")
    if not Convoy.run_process_success(
        [sys.executable, "-m", "pip", "install", package]
    ):
        Convoy.log(f"<fyellow>Failed to install <bold>{package}</bold>.")
        return False

    write_install_list(f"python-package: {package}")
    Convoy.log(f"Successfully installed <bold>{package}</bold>.")
    return True


def try_uninstall_python_package(package: str, /) -> bool:
    Convoy.log(f"Uninstalling python package <bold>{package}</bold>...")
    if not Convoy.run_process_success(
        [sys.executable, "-m", "pip", "uninstall", "-y", package]
    ):
        Convoy.log(f"<fyellow>Failed to uninstall <bold>{package}</bold>.")
        return False

    Convoy.log(f"Successfully uninstalled <bold>{package}</bold>.")
    return True


def must_install(dependency: str, exists: bool, /) -> bool:
    if not exists or not g_args.force:
        return not exists

    Convoy.log(f"Forcing <bold>{dependency}</bold> installation.")
    return True


@step("--Validating python packages--")
def validate_python_packages(*packages: str) -> None:

    needs_restart = False
    for package in packages:
        must = must_install(package, is_python_package_installed(package))

        needs_restart = needs_restart or must
        if must and (
            not prompt_to_install(package) or not try_install_python_package(package)
        ):
            Convoy.exit_error()

    if needs_restart:
        Convoy.log(
            "Packages were installed. Re-run the script for the changes to take effect."
        )
        Convoy.exit_restart()

    Convoy.log("All python packages found.")


@step("--Uninstalling python packages--")
def uninstall_python_packages(*packages: str) -> None:

    uninstalled = False
    for package in packages:
        if not is_python_package_installed(package):
            continue

        if not prompt_to_uninstall(package):
            Convoy.log(f"<fyellow>Skipping uninstallation of <bold>{package}</bold>.")
            continue
        uninstalled = try_uninstall_python_package(package) or uninstalled

    if uninstalled:
        Convoy.log(
            f"<fyellow>As one or more required python packages were uninstalled, this script will now terminate."
        )
        Convoy.exit_ok()


def is_linux_devtools_installed() -> bool:
    tools = ["gcc", "g++", "make"]

    installed = True
    for tool in tools:
        path = shutil.which(tool)
        if path is None:
            Convoy.log(f"<fyellow><bold>{tool}</bold> not found.")
            installed = False
        else:
            Convoy.log(f"<bold>{tool}</bold> found at <underline>{path}</underline>.")
    return installed


def is_linux_package_installed(package: str, /) -> bool:
    success = False
    if g_linux_distro == "ubuntu":
        success = Convoy.run_process_success(
            ["dpkg", "-l", package], capture_output=True
        )

    elif g_linux_distro == "fedora":
        success = Convoy.run_process_success(
            ["dnf", "list", "installed", package],
            capture_output=True,
        )

    elif g_linux_distro == "arch":
        success = Convoy.run_process_success(
            ["pacman", "-Q", package],
            capture_output=True,
        )

    if success:
        Convoy.log(f"<bold>{package}</bold> found.")
        return True

    Convoy.log(f"<fyellow><bold>{package}</bold> not found.")
    return False


def linux_install_package(
    package: str, /, *, ubuntu_update: str = False, group_install: str = False
) -> bool:
    Convoy.log(f"Installing <bold>{package}</bold>...")
    success = False

    if g_linux_distro == "ubuntu":
        if ubuntu_update and not Convoy.run_process_success(["apt-get", "update"]):
            Convoy.log("<fyellow>Failed to update apt-get")
        success = Convoy.run_process_success(["apt-get", "install", "-y", package])

    if g_linux_distro == "fedora":
        success = Convoy.run_process_success(
            [
                "dnf",
                "install" if not group_install else "groupinstall",
                "-y",
                package,
            ]
        )
    if g_linux_distro == "arch":
        success = Convoy.run_process_success(["pacman", "-S", "--noconfirm", package])

    if success:
        write_install_list(f"linux-package: {package}")
        Convoy.log(f"Successfully installed <bold>{package}</bold>.")
        return True

    Convoy.log(f"<fyellow>Failed to install <bold>{package}</bold>.")
    return False


def linux_uninstall_package(package: str, /, *, group_remove: str = False) -> bool:
    Convoy.log(f"Uninstalling <bold>{package}</bold>...")
    success = False

    if g_linux_distro == "ubuntu":
        success = Convoy.run_process_success(
            ["apt-get", "remove", "--purge", "-y", package]
        ) and Convoy.run_process_success(["apt-get", "autoremove", "-y"])
    if g_linux_distro == "fedora":
        success = Convoy.run_process_success(
            [
                "dnf",
                "remove" if not group_remove else "groupremove",
                "-y",
                package,
            ]
        )
    if g_linux_distro == "arch":
        success = Convoy.run_process_success(["pacman", "-Rns", "--noconfirm", package])

    if success:
        Convoy.log(f"Successfully uninstalled <bold>{package}</bold>.")
        return True

    Convoy.log(f"<fyellow>Failed to uninstall <bold>{package}</bold>.")
    return False


def try_install_linux_devtools() -> bool:
    return linux_install_package(
        g_linux_devtools, ubuntu_update=True, group_install=True
    )


def try_uninstall_linux_devtools() -> bool:
    return linux_uninstall_package(g_linux_devtools, group_remove=True)


@step("--Validating linux devtools--")
def validate_linux_devtools() -> None:

    def install() -> None:
        if not prompt_to_install(g_linux_devtools) or not try_install_linux_devtools():
            Convoy.exit_error()

    if must_install(g_linux_devtools, is_linux_devtools_installed()):
        install()


@step("--Uninstalling linux devtools--")
def uninstall_linux_devtools() -> None:
    if not is_linux_devtools_installed():
        return

    if not prompt_to_uninstall(g_linux_devtools):
        Convoy.log(
            f"<fyellow>Skipping uninstallation of <bold>{g_linux_devtools}</bold>."
        )
        return

    try_uninstall_linux_devtools()


def is_xcode_command_line_tools_installed() -> bool:
    if not Convoy.run_process_success(
        ["xcode-select", "-p"],
        capture_output=True,
    ):
        Convoy.log("<fyellow><bold>Xcode Command Line Tools</bold> not found.")
        return False

    Convoy.log("<bold>Xcode Command Line Tools</bold> found.")
    return True


def try_install_xcode_command_line_tools() -> bool:
    Convoy.log("Installing <bold>Xcode Command Line Tools</bold>...")
    if not Convoy.run_process_success(["xcode-select", "--install"]):
        Convoy.log("<fyellow>Failed to install <bold>Xcode Command Line Tools</bold>.")
        return False

    write_install_list("xcode-command-line-tools")
    Convoy.empty_prompt(
        "The <bold>Xcode Command Line Tools</bold> should now have opened. Please follow the instructions to complete the installation. Press any key to continue."
    )
    return True


def try_uninstall_xcode_command_line_tools() -> bool:
    Convoy.log("Uninstalling <bold>Xcode Command Line Tools</bold>...")
    if not Convoy.run_process_success(
        ["sudo", "rm", "-rf", "/Library/Developer/CommandLineTools"]
    ):
        Convoy.log(
            "<fyellow>Failed to uninstall <bold>Xcode Command Line Tools</bold>."
        )
        return False

    Convoy.log("Successfully uninstalled <bold>Xcode Command Line Tools</bold>.")
    return True


@step("--Validating Xcode Command Line Tools--")
def validate_xcode_command_line_tools() -> None:

    def install() -> None:
        if (
            not prompt_to_install("Xcode Command Line Tools")
            or not try_install_xcode_command_line_tools()
        ):
            Convoy.exit_error()

    if must_install(
        "Xcode Command Line Tools", is_xcode_command_line_tools_installed()
    ):
        install()


@step("--Uninstalling Xcode Command Line Tools--")
def uninstall_xcode_command_line_tools() -> None:
    if not is_xcode_command_line_tools_installed():
        return

    if not prompt_to_uninstall("Xcode Command Line Tools"):
        Convoy.log(
            "<fyellow>Skipping uninstallation of <bold>Xcode Command Line Tools</bold>"
        )
        return

    try_uninstall_xcode_command_line_tools()


def is_vulkan_installed(version: VulkanVersion, /):
    # def check_vulkaninfo() -> bool:
    #     exec = shutil.which("vulkaninfo")
    #     if exec is None:
    #         return False

    #     result = Convoy.run_process([exec], capture_output=True, text=True)
    #     return (
    #         result.returncode == 0
    #         and f"Vulkan Instance Version: {version.no_micro()}" in result.stdout
    #     )

    if "VULKAN_SDK" in os.environ:
        vulkan_sdk = os.environ["VULKAN_SDK"]
        Convoy.log(
            f"<bold>Vulkan SDK</bold> environment variable ('VULKAN_SDK') found: {vulkan_sdk}"
        )
        if str(version) in vulkan_sdk:
            return True

        Convoy.log(
            f"<fyellow><bold>Vulkan SDK</bold> version mismatch. Expected: {version}. Found: {vulkan_sdk}"
        )
    else:
        Convoy.log(
            "<fyellow><bold>Vulkan SDK</bold> environment variable ('VULKAN_SDK') not found."
        )

    # if check_vulkaninfo():
    #     Convoy.log(
    #         f"<bold>Vulkaninfo</bold> found at <underline>{shutil.which('vulkaninfo')}</underline>."
    #     )
    #     return True

    # Convoy.log("<bold>Vulkaninfo</bold> not found.")

    if Convoy.is_macos:
        vulkan_sdk = Path("~").expanduser() / "VulkanSDK" / str(version)
        if not vulkan_sdk.exists():
            Convoy.log(
                f"<fyellow><bold>Vulkan SDK</bold> not found at <underline>{vulkan_sdk}</underline>."
            )
            return False

        Convoy.log(
            f"<bold>Vulkan SDK</bold> found at <underline>{vulkan_sdk}</underline>."
        )
        if Path("/usr/local/lib/libvulkan.dylib").exists():
            Convoy.log(
                "<bold>Vulkan SDK</bold> found at <underline>/usr/local/lib/libvulkan.dylib</underline>."
            )
            return True

        Convoy.log(
            "<fyellow><bold>Vulkan SDK</bold> not found at <underline>/usr/local/lib/libvulkan.dylib</underline>."
        )
        return False

    if Convoy.is_linux:
        if any(
            [
                Path("/usr/lib/x86_64-linux-gnu/libvulkan.so.1").exists(),
                Path("/usr/lib/libvulkan.so").exists(),
                Path("/usr/local/lib/libvulkan.so.1").exists(),
            ]
        ):
            Convoy.log(
                "<bold>Vulkan SDK</bold> libraries found in <underline>/usr/lib</underline> or <underline>/usr/local/lib</underline>."
            )
            return True

        Convoy.log(
            "<fyellow><bold>Vulkan SDK</bold> libraries not found in <underline>/usr/lib</underline> or <underline>/usr/local/lib</underline>."
        )
        return False

    if Convoy.is_windows:
        dll = (
            Path(os.environ.get("SystemRoot", "C:\\Windows"))
            / "System32"
            / "vulkan-1.dll"
        )
        vulkan_sdk = Path("C:\\VulkanSDK") / str(version)
        if dll.exists() and vulkan_sdk.exists() and any(vulkan_sdk.iterdir()):
            Convoy.log(
                f"<bold>Vulkan SDK</bold> found at <underline>{vulkan_sdk}</underline>. Installation validated with the presence of <underline>{dll}</underline>."
            )
            return True

        Convoy.log(
            f"<fyellow><bold>Vulkan SDK</bold> not found at <underline>{vulkan_sdk}</underline> or was not validated with the presence of <underline>{dll}</underline>."
        )
        return False

    return False


def download_file(url: str, dest: Path, /) -> None:
    import requests
    from tqdm import tqdm

    if dest.exists():
        Convoy.log(
            f"<fyellow><underline>{url}</underline> already downloaded. To trigger a re-download, delete the file at <underline>{dest}</underline>."
        )
        return

    Convoy.log(
        f"Downloading <underline>{url}</underline> into <underline>{dest}</underline>..."
    )
    headers = {"User-Agent": "Mozilla/5.0"}
    response = requests.get(url, stream=True, headers=headers, allow_redirects=True)

    total = int(response.headers.get("content-length", 0))
    one_kb = 1024

    with open(dest, "wb") as f, tqdm(
        desc=dest.name,
        total=total,
        unit="MiB",
        unit_scale=True,
        unit_divisor=one_kb * one_kb,
    ) as bar:
        for data in response.iter_content(chunk_size=one_kb):
            size = f.write(data)
            bar.update(size)


def extract_file(path: Path, dest: Path, /) -> None:

    from tqdm import tqdm

    if dest.exists():
        Convoy.log(
            f"<fyellow><underline>{path}</underline> already extracted. To trigger a re-extraction, delete the file/folder at <underline>{dest}</underline>."
        )
        return

    dest.mkdir(exist_ok=True, parents=True)
    Convoy.log(
        f"Extracting <underline>{path}</underline> into <underline>{dest}</underline>..."
    )
    if path.suffix == ".zip":
        if Convoy.is_macos:
            if Convoy.run_process_success(["unzip", str(path), "-d", str(dest)]):
                return

            Convoy.log(
                f"<fyellow>Failed to extract <underline>{path}</underline> into <underline>{dest}</underline>. using the <bold>unzip</bold> command. Trying with python's <bold>zipfile</bold> module."
            )

        with zipfile.ZipFile(path, "r") as zip:
            for ffile in tqdm(iterable=zip.namelist(), total=len(zip.namelist())):
                zip.extract(member=ffile, path=dest)

    elif path.suffix in [".tar.gz", ".tar.xz"]:  # Assuming this is linux only
        suffix = path.suffix.split(".")[-1]
        with tarfile.TarFile.open(path, f"r:{suffix}") as tar:
            for ffile in tqdm(iterable=tar.getnames(), total=len(tar.getnames())):
                tar.extract(member=ffile, path=dest)
    else:
        Convoy.exit_error(f"Unsupported file extension: {path.suffix}")


def try_install_vulkan(version: VulkanVersion, /) -> bool:
    Convoy.log("Installing <bold>Vulkan SDK</bold>...")

    vendor = g_root / "vendor"
    vendor.mkdir(exist_ok=True)
    if Convoy.is_macos:
        extension = "zip" if version.minor > 3 or version.patch > 290 else "dmg"
        filename = f"vulkansdk-macos-{version}.{extension}"
        osfolder = "mac"
    elif Convoy.is_windows:
        filename = (
            f"VulkanSDK-{version}-Installer.exe"
            if not Convoy.is_arm
            else f"InstallVulkanARM64-{version}.exe"
        )
        osfolder = "windows" if not Convoy.is_arm else "warm"
    elif Convoy.is_linux:
        extension = "tar.xz" if version.minor > 3 or version.patch > 250 else "tar.gz"
        filename = f"vulkansdk-linux-x86_64-{version}.{extension}"
        osfolder = "linux"

    download_path = vendor / filename
    url = f"https://sdk.lunarg.com/sdk/download/{version}/{osfolder}/{filename}"
    download_file(url, download_path)

    def macos_install(installer_path: Path, /, *, include_version: bool = True) -> bool:
        name = "InstallVulkan" if not include_version else f"InstallVulkan-{version}"
        path = str(installer_path / "Contents" / "MacOS" / name)
        vulkan_sdk = Path(f"~/VulkanSDK/{version}").expanduser()
        if not Convoy.run_process_success(
            [
                "sudo",
                path,
                "--root",
                str(vulkan_sdk),
                "--accept-licenses",
                "--default-answer",
                "--confirm-command",
                "install",
                "com.lunarg.vulkan.core",
                "com.lunarg.vulkan.usr",
            ]
        ):
            Convoy.log(
                f"<fyellow>Failed to run the <bold>Vulkan SDK</bold> installer at <underline>{path}</underline>."
            )
            return False

        write_install_list(f"vulkan-sdk = {version}")
        Convoy.log(
            f"Successfully ran the <bold>Vulkan SDK</bold> installer at <underline>{path}</underline>. The SDK is installed at <underline>{vulkan_sdk}</underline>."
        )
        return True

    if filename.endswith(("zip", "tar.gz", "tar.xz")):
        extract_path = vendor / "vulkan-extract" / str(version)
        extract_file(download_path, extract_path)

        if Convoy.is_macos:
            filepath = extract_path / f"InstallVulkan-{version}.app"
            include_version = True
            if not filepath.exists():
                filepath = vendor / f"InstallVulkan-{version}.app"
            if not filepath.exists():
                filepath = extract_path / "InstallVulkan.app"
                include_version = False
            if not filepath.exists():
                filepath = vendor / "InstallVulkan.app"

            return macos_install(filepath, include_version=include_version)

        if Convoy.is_linux:

            write_install_list(f"vulkan-sdk = {version}")
            vulkan_sdk = extract_path / str(version) / "x86_64"
            Convoy.log(
                f"The <bold>Vulkan SDK</bold> has been successfully installed for the linux distribution <bold>{g_linux_distro} {g_linux_version}</bold> at <underline>{vulkan_sdk}</underline>."
            )
            Convoy.log(
                "You must now setup the environment. You can do so in different ways:"
            )
            Convoy.log(
                f"1. Use the source command with the <bold>setup-env.sh</bold> script like so: <underline>{vulkan_sdk / 'setup-env.sh'}</underline>"
            )
            Convoy.log("2. Execute the commands yourself:")
            Convoy.log(f"   - export VULKAN_SDK={vulkan_sdk}")
            Convoy.log(f"   - export PATH=$VULKAN_SDK/bin:$PATH")
            Convoy.log(
                f"   - export LD_LIBRARY_PATH=$VULKAN_SDK/lib${{LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}}"
            )
            Convoy.log(
                f"   - export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d"
            )
            Convoy.log("3. Add the above commands to your shell profile file.")
            Convoy.log(
                "\nNote that steps 1 and 2 will only set the environment variables for your current shell session. They do not permanently 'setup' any of this configuration for other shell sessions or future logins."
            )
            return True

    if Convoy.is_macos:

        def mount() -> bool:
            if not Convoy.run_process_success(
                ["hdiutil", "attach", str(download_path)]
            ):
                Convoy.log(
                    "<fyellow>Failed to mount the <bold>Vulkan SDK</bold> disk image."
                )
                return False
            return True

        def unmount() -> bool:
            if not Convoy.run_process_success(
                ["hdiutil", "detach", str(installer_path.parent)]
            ):
                Convoy.log(
                    "<fyellow>Failed to unmount the <bold>Vulkan SDK</bold> disk image."
                )
                return False
            return True

        if not mount():
            return False

        installer_path = Path("/") / "Volumes" / "VulkanSDK" / "InstallVulkan.app"
        if not macos_install(installer_path, include_version=False):
            unmount()
            return False

        return unmount()

    if Convoy.is_windows:
        if not Convoy.run_process_success(
            [
                str(download_path),
                "--root",
                str(Path("C:\\VulkanSDK") / str(version)),
                "--accept-licenses",
                "--default-answer",
                "--confirm-command",
                "install",
                "com.lunarg.vulkan.core",
                "com.lunarg.vulkan.usr",
            ]
        ):
            Convoy.log(
                f"<fyellow>Failed to run the <bold>Vulkan SDK</bold> installer at <underline>{download_path}</underline>."
            )
            return False

        write_install_list(f"vulkan-sdk = {version}")
        Convoy.log("Successfully installed the <bold>Vulkan SDK</bold>.")
        return True

    return False


def try_uninstall_vulkan(version: VulkanVersion, /) -> bool:
    if Convoy.is_macos:
        path = Path("~/VulkanSDK").expanduser() / str(version)

        def remove_folder() -> bool:
            if not path.exists():
                return True
            if not Convoy.run_process_success(["sudo", "rm", "-rf", str(path)]):
                Convoy.log(
                    f"<fyellow>Failed to remove the <bold>Vulkan SDK</bold> at <underline>{path}</underline>."
                )
                return False

            Convoy.log(
                f"Successfully removed the <bold>Vulkan SDK</bold> at <underline>{path}</underline>."
            )
            return True

        maintenance = (
            path / "MaintenanceTool.app" / "Contents" / "MacOS" / "MaintenanceTool"
        )
        if not maintenance.exists():
            Convoy.log(
                f"<fyellow><bold>Vulkan SDK</bold> maintenance tool not found at <underline>{maintenance}</underline>."
            )
        elif Convoy.run_process_success(
            ["sudo", str(maintenance), "--confirm-command", "purge"]
        ):
            Convoy.log(
                f"Successfully uninstalled the <bold>Vulkan SDK</bold> at <underline>{path}</underline>."
            )
            return remove_folder()

        Convoy.log(
            "<fyellow>Failed to uninstall the <bold>Vulkan SDK</bold> using the maintenance tool. Trying with the uninstaller script."
        )
        uninstall = path / "uninstall.sh"

        if not uninstall.exists():
            Convoy.log(
                f"<fyellow><bold>Vulkan SDK</bold> uninstaller not found at <underline>{uninstall}</underline>. Failed to uninstall."
            )
            return False

        Convoy.log(
            f"<bold>Vulkan SDK</bold> uninstaller found at <underline>{uninstall}</underline>."
        )
        if not Convoy.run_process_success(["sudo", "/bin/bash", str(uninstall)]):
            Convoy.log(
                f"<fyellow>Failed to run the <bold>Vulkan SDK</bold> uninstaller at <underline>{uninstall}</underline>"
            )
            return False

        return remove_folder()

    if Convoy.is_linux:
        if not Convoy.run_process_success(
            ["sudo", "rm", "-rf", str(g_root / "vendor" / "vulkan-extract")]
        ):
            Convoy.log(
                "<fyellow>Failed to remove the extracted <bold>Vulkan SDK</bold>."
            )
            return False

        Convoy.log("Successfully uninstalled <bold>Vulkan SDK</bold>.")
        return True

    if Convoy.is_windows:
        vulkan_sdk = Path("C:\\VulkanSDK") / str(version)
        vulkan_uninstall = vulkan_sdk / "maintenancetool.exe"
        if not vulkan_uninstall.exists():
            Convoy.log(
                f"<fyellow><bold>Vulkan SDK</bold> maintenance tool not found at <underline>{vulkan_uninstall}</underline>."
            )
            return False

        Convoy.log(
            f"<bold>Vulkan SDK</bold> maintenance tool found at <underline>{vulkan_uninstall}</underline>."
        )
        if not Convoy.run_process_success(
            [str(vulkan_uninstall), "--confirm-command", "purge"]
        ):
            Convoy.log(
                f"<fyellow>Failed to uninstall <bold>Vulkan SDK</bold> using the maintenance tool at <underline>{vulkan_uninstall}</underline>."
            )
            return False

        if vulkan_sdk.exists() and any(vulkan_sdk.iterdir()):
            Convoy.log(
                f"<fyellow>The <bold>Vulkan SDK</bold> folder at <underline>{vulkan_sdk}</underline> is still detected. Check if it is empty to confirm the uninstallation."
            )

        Convoy.log(
            f"Successfully uninstalled <bold>Vulkan SDK</bold> at <underline>{vulkan_sdk}</underline>."
        )
        return True


@step("--Validating Vulkan SDK--")
def validate_vulkan(version: VulkanVersion, /) -> None:
    Convoy.log(f"Requested version: <bold>{version}</bold>.")
    if Convoy.is_linux:

        def install_dependency(dependency: str, /) -> None:
            if not prompt_to_install(dependency) or not linux_install_package(
                dependency
            ):
                Convoy.exit_error()

        for dep in g_linux_dependencies:
            if must_install(dep, is_linux_package_installed(dep)):
                install_dependency(dep)

    if must_install("Vulkan SDK", is_vulkan_installed(version)) and (
        not prompt_to_install("Vulkan SDK") or not try_install_vulkan(version)
    ):
        Convoy.exit_error()


def uninstall_linux_dependencies(dependencies: list[str] | None = None, /) -> None:
    if dependencies is None:
        dependencies = g_linux_dependencies

    def try_uninstall_dependency(dependency: str, /) -> None:
        if not prompt_to_uninstall(dependency):
            Convoy.log(
                f"<fyellow>Skipping uninstallation of <bold>{dependency}</bold>."
            )
            return

        linux_uninstall_package(dependency)

    for dep in dependencies:
        if is_linux_package_installed(dep):
            try_uninstall_dependency(dep)


@step("--Uninstalling Vulkan SDK--")
def uninstall_vulkan(version: VulkanVersion, /) -> None:
    Convoy.log(f"Requested version: <bold>{version}</bold>.")
    if Convoy.is_linux and g_args.ignore_install_list:
        uninstall_linux_dependencies()

    if not is_vulkan_installed(version):
        return

    if not prompt_to_uninstall("Vulkan SDK"):
        Convoy.log("<fyellow>Skipping uninstallation of <bold>Vulkan SDK</bold>")
        return

    try_uninstall_vulkan(version)


def look_for_visual_studio_installer(file: Path | str | None = None, /) -> Path | None:
    Convoy.log("Looking for <bold>Visual Studio Installer</bold>...")
    for p in g_vs_paths:
        path = p / "Microsoft Visual Studio" / "Installer"
        if file is not None:
            path /= file

        if path.exists() and (file is not None or any(path.iterdir())):
            Convoy.log(
                f"<bold>Visual Studio Installer</bold> found at <underline>{path}</underline>."
            )
            return path

        Convoy.log(
            f"<fyellow><bold>Visual Studio Installer</bold> not found at <underline>{path}</underline>."
        )
    return None


def is_visual_studio_installed(version: str | None = None, /) -> bool:
    if version is None:
        return look_for_visual_studio_installer() is not None

    for p in g_vs_paths:
        path = p / "Microsoft Visual Studio" / g_vs_year_map[version]
        installations = ["Community", "Professional", "Enterprise", "BuildTools"]
        for install in installations:
            subpath = path / install
            if subpath.exists() and any(subpath.iterdir()):
                Convoy.log(
                    f"<bold>Visual Studio</bold> found at <underline>{subpath}</underline>."
                )
                break
        else:
            Convoy.log(
                f"<fyellow><bold>Visual Studio</bold> not found at <underline>{path}</underline>."
            )
            continue
        break
    else:
        return False

    vswhere = look_for_visual_studio_installer("vswhere.exe")
    if vswhere is None:
        return False

    result = Convoy.run_process(
        [
            str(vswhere),
            "-version",
            f"[{version},{int(version) + 1})",
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Workload.NativeDesktop",
        ],
        stdout=subprocess.PIPE,
        text=True,
    )
    if result.returncode != 0:
        Convoy.log(
            "<fyellow>Failed to query <bold>Visual Studio</bold> installations with <bold>vswhere.exe</bold>."
        )
    output = result.stdout
    if "instanceid" not in output.lower():
        Convoy.log(
            "<fyellow>No valid <bold>Visual Studio</bold> installations found with <bold>vswhere.exe</bold>."
        )
        return False

    Convoy.log("A valid <bold>Visual Studio</bold> installation was found.")
    return True


def try_install_visual_studio(version: str, /) -> bool:
    Convoy.log("Installing <bold>Visual Studio</bold>...")

    def install(commands: list[str], /, *, kind: str) -> bool:
        if not Convoy.run_process_success(commands):
            Convoy.log("<fyellow>Failed to install <bold>Visual Studio</bold>.")
            return False

        write_install_list(f"visual-studio-{kind} = {version}")
        Convoy.log("Successfully installed <bold>Visual Studio</bold>.")
        return True

    installer_path = look_for_visual_studio_installer("setup.exe")
    if installer_path is None:
        installer_path = look_for_visual_studio_installer("vs_installer.exe")

    if installer_path is not None:
        return install(
            [
                str(installer_path),
                "install",
                "--norestart",
                "--passive",
                "--add",
                "Microsoft.VisualStudio.Workload.NativeDesktop",
                "--includeRecommended",
                "--productId",
                "Microsoft.VisualStudio.Product.Community",
                "--channelUri",
                f"https://aka.ms/vs/{version}/release/channel",
            ],
            kind="partial",
        )

    url = f"https://aka.ms/vs/{version}/release/vs_community.exe"
    installer_path = g_root / "vendor" / "vs_community.exe"
    download_file(url, installer_path)

    return install(
        [
            str(installer_path),
            "--wait",
            "--norestart",
            "--passive",
            "--add",
            "Microsoft.VisualStudio.Workload.NativeDesktop",
            "--includeRecommended",
        ],
        kind="full",
    )


def try_uninstall_visual_studio(kind: str, version: str | None = None, /) -> bool:
    Convoy.log("Uninstalling <bold>Visual Studio</bold>...")
    installer_path = look_for_visual_studio_installer("setup.exe")
    if installer_path is None:
        installer_path = look_for_visual_studio_installer("vs_installer.exe")
    if installer_path is None:
        Convoy.log("<fyellow> Failed to uninstall <bold>Visual Studio</bold>.")
        return False

    if kind == "full" and Convoy.run_process_success(
        [str(installer_path), "/uninstall"]
    ):
        Convoy.log("Successfully uninstalled <bold>Visual Studio</bold>.")
        return True

    if version is None:
        Convoy.exit_error(
            "Version must be provided for a partial uninstallation of <bold>Visual Studio</bold>."
        )

    if kind == "partial" and Convoy.run_process_success(
        [
            str(installer_path),
            "modify",
            "--norestart",
            "--passive",
            "--remove",
            "Microsoft.VisualStudio.Workload.NativeDesktop",
            "--includeRecommended",
            "--productId",
            "Microsoft.VisualStudio.Product.Community",
            "--channelUri",
            f"https://aka.ms/vs/{version}/release/channel",
        ]
    ):
        Convoy.log(
            f"Successfully uninstalled the <bold>Native Desktop</bold> subset features for <bold>Visual Studio Community {g_vs_year_map[version]}</bold>. Please, keep in mind that this operation, in case the <bold>Native Desktop</bold> features were the only ones installed in your <bold>Visual Studio Community {g_vs_year_map[version]}</bold> installation, may have left you with an empty <bold>Visual Studio Community {g_vs_year_map[version]}</bold> shell. If that is the case, you may want to remove it from the <bold>Visual Studio Installer</bold>."
        )
        return True

    Convoy.log("<fyellow>Failed to uninstall <bold>Visual Studio</bold>.")
    return False


@step("--Validating Visual Studio--")
def validate_visual_studio(version: str, /) -> None:
    Convoy.log(f"Requested version: <bold>{version} ({g_vs_year_map[version]})</bold>.")
    if must_install("Visual Studio", is_visual_studio_installed(version)) and (
        not prompt_to_install("Visual Studio") or not try_install_visual_studio(version)
    ):
        Convoy.exit_error()


@step("--Uninstalling Visual Studio--")
def uninstall_visual_studio(kind: str, version: str | None = None, /) -> None:
    if version is not None:
        Convoy.log(
            f"Requested version: <bold>{version} ({g_vs_year_map[version]})</bold>."
        )
    if not is_visual_studio_installed(version):
        return

    if not prompt_to_uninstall("Visual Studio"):
        Convoy.log("<fyellow>Skipping uninstallation of <bold>Visual Studio</bold>")
        return

    try_uninstall_visual_studio(kind, version)


def is_homebrew_installed() -> bool:
    brew_path = shutil.which("brew")
    if brew_path is None:
        Convoy.log("<fyellow><bold>Homebrew</bold> not found.")
        return False

    Convoy.log(f"<bold>Homebrew</bold> found at <underline>{brew_path}</underline>.")
    return True


def try_install_homebrew() -> bool:
    Convoy.log("Installing <bold>Homebrew</bold>...")
    if not Convoy.run_process_success(
        '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"',
        shell=True,
    ):
        Convoy.log("<fyellow>Failed to install <bold>Homebrew</bold>.")
        return False

    write_install_list("homebrew")
    Convoy.log("Successfully installed <bold>Homebrew</bold>.")
    return True


def try_uninstall_homebrew() -> bool:
    if not Convoy.run_process_success(
        '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"',
        shell=True,
    ):
        Convoy.log("<fyellow>Failed to uninstall <bold>Homebrew</bold>.")
        return False

    Convoy.log("Successfully uninstalled <bold>Homebrew</bold>.")
    return True


@step("--Validating Homebrew--")
def validate_homebrew() -> None:
    if must_install("Homebrew", is_homebrew_installed()) and (
        not prompt_to_install("Homebrew") or not try_install_homebrew()
    ):
        Convoy.exit_error()


@step("--Uninstalling Homebrew--")
def uninstall_homebrew() -> None:
    if not is_homebrew_installed():
        return

    if not prompt_to_uninstall("Homebrew"):
        Convoy.log("<fyellow>Skipping uninstallation of <bold>Homebrew</bold>")
        return

    try_uninstall_homebrew()


def is_cmake_installed() -> bool:
    cmake_path = shutil.which("cmake")
    if cmake_path is None:
        Convoy.log(
            "<fyellow><bold>CMake</bold> not found. Note that <bold>CMake</bold> must be added to path for this script to detect it."
        )
        return False

    Convoy.log(f"<bold>CMake</bold> found at <underline>{cmake_path}</underline>.")
    return True


def try_install_cmake(version: str | None = None, /) -> bool:
    if Convoy.is_macos:
        Convoy.log("<bold>Homebrew</bold> is needed to install <bold>CMake</bold>.")
        if not is_homebrew_installed() and (
            not prompt_to_install("Homebrew") or not try_install_homebrew()
        ):
            return False

        Convoy.log("Installing <bold>CMake</bold>...")
        if not Convoy.run_process_success(["brew", "install", "cmake"]):
            Convoy.log("<fyellow>Failed to install <bold>CMake</bold>.")
            return False

        write_install_list("cmake")
        Convoy.log("Successfully installed <bold>CMake</bold>.")
        return True

    if Convoy.is_linux:
        return linux_install_package("cmake")

    if Convoy.is_windows:
        Convoy.log("Installing <bold>CMake</bold>...")
        vendor = g_root / "vendor"
        vendor.mkdir(exist_ok=True)
        arch = "i386" if Convoy.architecure == "x86" else Convoy.architecure
        installer_url = f"https://github.com/Kitware/CMake/releases/download/v{version}/cmake-{version}-windows-{arch}.msi"
        installer_path = vendor / f"cmake-{version}-windows-{arch}.msi"

        download_file(installer_url, installer_path)
        if not Convoy.run_process(
            f"msiexec /i {installer_path} ALLUSERS=1 ADD_CMAKE_TO_PATH=System /qn"
        ):
            Convoy.log("<fyellow>Failed to install <bold>CMake</bold>.")
            return False

        write_install_list(f"cmake = {version}")
        Convoy.log("Successfully installed <bold>CMake</bold>.")
        return True


def try_uninstall_cmake() -> bool:
    if Convoy.is_macos:
        Convoy.log("<bold>Homebrew</bold> is needed to uninstall <bold>CMake</bold>.")
        if not is_homebrew_installed() and (
            not prompt_to_install("Homebrew") or not try_install_homebrew()
        ):
            return False

        Convoy.log("Uninstalling <bold>CMake</bold>...")
        if not Convoy.run_process_success(["brew", "uninstall", "cmake"]):
            Convoy.log("<fyellow>Failed to uninstall <bold>CMake</bold>.")
            return False

        Convoy.log("Successfully uninstalled <bold>CMake</bold>.")
        return True

    if Convoy.is_linux:
        return linux_uninstall_package("cmake")

    if Convoy.is_windows:
        cmd_query = (
            "wmic product where \"name like '%CMake%'\" get Name, IdentifyingNumber"
        )
        result = Convoy.run_process(
            cmd_query, shell=True, stdout=subprocess.PIPE, text=True
        )
        if result.returncode != 0:
            Convoy.log("<fyellow>Failed to query <bold>CMake</bold> installation.")
            return False

        output: str = result.stdout
        Convoy.log(f"Query result: <bold>{output}</bold>.")

        guid = re.match(r".*(\{[a-fA-F0-9\-]+\})", output.replace("\n", ""))
        if guid is None:
            Convoy.log(
                "<fyellow>Failed to find <bold>CMake</bold>'s guid in the query result."
            )
            return False

        guid = guid.group(1)
        Convoy.log(f"<bold>CMake</bold> found with guid: <bold>{guid}</bold>.")
        if not Convoy.run_process_success(f"msiexec /x {guid}"):
            Convoy.log(
                f"<fyellow>Failed to uninstall <bold>CMake</bold> with guid: <bold>{guid}</bold>."
            )
            return False

        Convoy.log(
            f"Successfully uninstalled <bold>CMake</bold> with guid: <bold>{guid}</bold>."
        )
        return True

    return False


@step("--Validating CMake--")
def validate_cmake(version: str | None = None, /) -> None:
    if Convoy.is_windows:
        Convoy.log(f"Requested version: <bold>{version}</bold>.")

    if must_install("CMake", is_cmake_installed()) and (
        not prompt_to_install("CMake") or not try_install_cmake(version)
    ):
        Convoy.exit_error()


@step("--Uninstalling CMake--")
def uninstall_cmake() -> None:
    if not is_cmake_installed():
        return

    if not prompt_to_uninstall("CMake"):
        Convoy.log("<fyellow>Skipping uninstallation of <bold>CMake</bold>")
        return

    try_uninstall_cmake()


@step("--Executing build script--")
def execute_build_script() -> None:
    build_script: Path = g_args.build_script.resolve()
    if not build_script.exists():
        Convoy.exit_error(
            f"Build script not found at <underline>{build_script}</underline>."
        )

    if not Convoy.prompt(
        f"Build script provided at <underline>{build_script}</underline>. Do you wish to exexcute it?"
    ):
        Convoy.exit_error()

    if build_script.suffix == ".py":
        cmd = [sys.executable, str(build_script), *g_unknown]
    else:
        cmd = [str(build_script), *g_unknown]

    if not Convoy.run_process(cmd):
        Convoy.exit_error(
            f"Failed to execute build script at <underline>{build_script}</underline>."
        )


Convoy.log_label = "SETUP"
g_needs_restart = False
g_root = Path(__file__).parent.resolve()
g_args, g_unknown = parse_arguments()
validate_arguments()
Convoy.all_yes = g_args.yes
Convoy.safe = g_args.safe

install_list = (
    read_install_list()
    if (g_args.uninstall or g_args.uninstall_python_packages)
    and not g_args.ignore_install_list
    else None
)
if Convoy.is_linux:
    g_linux_distro = Convoy.linux_distro()
    g_linux_version = Convoy.linux_version()
    g_linux_devtools = None
    g_linux_dependencies = None

if Convoy.is_windows:
    g_vs_year_map = {
        "10": "2010",
        "11": "2012",
        "12": "2013",
        "14": "2015",
        "15": "2017",
        "16": "2019",
        "17": "2022",
    }
    if g_args.visual_studio_version not in g_vs_year_map:
        Convoy.exit_error(
            f"Unsupported <bold>Visual Studio</bold> version: {g_args.visual_studio_version}"
        )
    g_vs_paths = [Path("C:\\Program Files"), Path("C:\\Program Files (x86)")]

validate_python_version(3, 10, 0)
validate_operating_system()

if not g_args.uninstall_python_packages:
    validate_python_packages("requests", "tqdm")
elif g_args.ignore_install_list:
    uninstall_python_packages("requests", "tqdm")
else:
    packages = install_list.packages("python")
    if not packages:
        Convoy.log("<fyellow>No python packages found installed by this script.")
        Convoy.exit_ok()
    uninstall_python_packages(*packages)


if not g_args.uninstall:
    if Convoy.is_linux and g_args.manage_linux_devtools:
        validate_linux_devtools()

    if Convoy.is_macos and g_args.manage_xcode_command_line_tools:
        validate_xcode_command_line_tools()

    if Convoy.is_macos and g_args.manage_brew:
        validate_homebrew()

    if g_args.manage_cmake:
        validate_cmake(g_args.cmake_version if Convoy.is_windows else None)

    if g_args.manage_vulkan_sdk:
        version = VulkanVersion(*[int(v) for v in g_args.vulkan_version.split(".")])
        validate_vulkan(version)

    if Convoy.is_windows and g_args.manage_visual_studio:
        validate_visual_studio(g_args.visual_studio_version)

elif g_args.ignore_install_list:

    if g_args.manage_vulkan_sdk:
        version = VulkanVersion(*[int(v) for v in g_args.vulkan_version.split(".")])
        uninstall_vulkan(version)

    if g_args.manage_cmake:
        uninstall_cmake()

    if Convoy.is_windows and g_args.manage_visual_studio:
        if Convoy.prompt(
            "Do you wish to fully uninstall <bold>Visual Studio</bold>? A full uninstallation will remove all components of <bold>Visual Studio</bold>. A partial uninstallation will only remove the <bold>Native Desktop</bold> workload that this project requires."
        ):
            uninstall_visual_studio("full")
        else:
            uninstall_visual_studio("partial", g_args.visual_studio_version)

    if Convoy.is_linux and g_args.manage_linux_devtools:
        uninstall_linux_devtools()

    if Convoy.is_macos and g_args.manage_brew:
        uninstall_homebrew()

    if Convoy.is_macos and g_args.manage_xcode_command_line_tools:
        uninstall_xcode_command_line_tools()
else:
    if install_list.has_dependency("vulkan-sdk"):
        for version in install_list.find_versions("vulkan-sdk"):
            version = VulkanVersion(*[int(v) for v in version.split(".")])
            uninstall_vulkan(version)
    else:
        Convoy.log("<fyellow><bold>Vulkan SDK</bold> was not installed by this script")

    if install_list.has_dependency("cmake"):
        uninstall_cmake()
    else:
        Convoy.log("<fyellow><bold>CMake</bold> was not installed by this script.")

    if Convoy.is_windows and install_list.has_dependency("visual-studio-full"):
        uninstall_visual_studio("full")
    elif Convoy.is_windows and install_list.has_dependency("visual-studio-partial"):
        for version in install_list.find_versions("visual-studio-partial"):
            uninstall_visual_studio("partial", version)
    elif Convoy.is_windows:
        Convoy.log(
            "<fyellow><bold>Visual Studio</bold> was not installed by this script."
        )

    packages = install_list.packages("linux")
    if Convoy.is_linux and packages:
        uninstall_linux_dependencies(packages)
    elif Convoy.is_linux:
        Convoy.log("<fyellow>No linux packages found installed by this script.")

    if Convoy.is_macos and install_list.has_dependency("homebrew"):
        uninstall_homebrew()
    elif Convoy.is_macos:
        Convoy.log("<fyellow><bold>Homebrew</bold> was not installed by this script.")

    if Convoy.is_macos and install_list.has_dependency("xcode-command-line-tools"):
        uninstall_xcode_command_line_tools()
    elif Convoy.is_macos:
        Convoy.log(
            "<fyellow><bold>XCode Command Line Tools</bold> were not installed by this script."
        )

if g_needs_restart:
    Convoy.log(
        "Dependencies were installed. Re-run the setup for the changes to take effect."
    )
    if g_args.build_script is not None:
        Convoy.log(
            "<fyellow>Build script execution was aborted because of the dependency changes."
        )

    Convoy.exit_restart()

if g_args.build_script is not None:
    execute_build_script()

Convoy.exit_ok()
