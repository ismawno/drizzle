from argparse import ArgumentParser, Namespace
from pathlib import Path
from dataclasses import dataclass
from collections.abc import Callable

import sys
import os
import subprocess
import importlib.util
import shutil
import platform
import functools

import zipfile
import tarfile

import re


class Style:
    RESET = "\033[0m"
    BOLD = "\033[1m"

    FG_RED = "\033[31m"
    FG_GREEN = "\033[32m"
    FG_YELLOW = "\033[33m"
    FG_BLUE = "\033[34m"
    FG_CYAN = "\033[36m"

    BG_YELLOW = "\033[43m"


def exit_ok() -> None:
    log(Style.FG_GREEN + "Success!" + Style.RESET)
    sys.exit()


def exit_restart() -> None:
    log("Re-run the script for changes to take effect.")
    sys.exit()


def exit_error(msg: str | None = None, /) -> None:
    if msg is not None:
        log(Style.FG_RED + Style.BOLD + f"Error: {msg}" + Style.RESET, file=sys.stderr)
    else:
        log(
            Style.FG_RED
            + Style.BOLD
            + f"Installation failed, likely because a particular step failed or the user declined a prompt."
            + Style.RESET,
            file=sys.stderr,
        )
    sys.exit(1)


def is_windows() -> bool:
    return sys.platform.startswith("win")


def is_linux() -> bool:
    return sys.platform.startswith("linux")


def is_macos() -> bool:
    return sys.platform.startswith("darwin")


def get_os() -> str:
    if is_windows():
        return "Windows"
    if is_linux():
        return "Linux"
    if is_macos():
        return "MacOS"
    return sys.platform


def is_arm() -> bool:
    return platform.machine().lower() in ["arm64", "aarch64"]


def os_architecture() -> str:
    similars = {"i386": "x86", "amd64": "x86_64", "x32": "x86", "x64": "x86_64"}
    try:
        return similars[platform.machine().lower()]
    except KeyError:
        return platform.machine().lower()


def get_linux_distro() -> str | None:
    try:
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("ID="):
                    return line.split("=")[1].strip().strip('"')
    except FileNotFoundError:
        return None
    return None


def get_linux_version() -> str | None:
    try:
        with open("/etc/os-release") as f:
            for line in f:
                if line.startswith("VERSION_ID="):
                    return line.split("=")[1].strip().strip('"')
    except FileNotFoundError:
        return None
    return None


def parse_arguments() -> tuple[Namespace, list[str]]:
    desc = """
    This is a general installation scripts designed to download and install various recurring
    dependencies for my projects across different operating systems. The supported operating systems are Windows, Linux (Ubuntu, Fedora and Arch), and MacOS.

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
    if is_linux():
        parser.add_argument(
            "--linux-devtools",
            action="store_true",
            default=False,
            help="Install or uninstall Linux development tools, such as 'build-essential' on Ubuntu, 'Development Tools' on Fedora, or 'base-devel' on Arch.",
        )
    if is_macos():
        parser.add_argument(
            "--xcode-command-line-tools",
            "--xcode-clt",
            action="store_true",
            default=False,
            help="Install or uninstall the Xcode Command Line Tools.",
        )
        parser.add_argument(
            "--brew",
            action="store_true",
            default=False,
            help="Install or uninstall 'Homebrew'.",
        )

    parser.add_argument(
        "--vulkan",
        action="store_true",
        default=False,
        help="Install or uninstall the Vulkan SDK.",
    )
    parser.add_argument(
        "--cmake",
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
        "--build-script",
        type=Path,
        default=None,
        help="Path to the build script to run after the setup. If provided, unknown arguments will be passed to the build script. Default is None.",
    )

    parser.add_argument(
        "--vulkan-version",
        type=str,
        default="1.3.250.1",
        help="The Vulkan SDK version to install. Default is '1.3.250.1'.",
    )
    if is_windows():
        parser.add_argument(
            "--cmake-version",
            type=str,
            default="3.21.3",
            help="The CMake version to install. This setting is only applicable to Windows. In other OS, the latest version is installed through a package manager. Default is '3.21.3'.",
        )

    return parser.parse_known_args()


@dataclass
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


indent = 0
setup_label = Style.FG_BLUE + "[SETUP]" + Style.RESET + " "


def log(msg: str, /, *args, **kwargs) -> None:
    print(f"{setup_label}{'  ' * indent}{msg}", *args, **kwargs)


def step(msg: str, /) -> Callable:
    def decorator(func: Callable) -> Callable:

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            log(msg)
            push_indent()
            result = func(*args, **kwargs)
            pop_indent()
            return result

        return wrapper

    return decorator


def push_indent() -> None:
    global indent
    indent += 1


def pop_indent() -> None:
    global indent
    indent -= 1


def prompt(msg: str, /, *, default: bool = True) -> bool:
    if g_args.yes:
        return True
    msg = (
        setup_label
        + Style.FG_CYAN
        + indent * "  "
        + (f"{msg} [Y]/N " if default else f"{msg} Y/[N] ")
        + Style.RESET
    )
    while True:
        answer = input(msg).strip().lower()
        if answer in ["y", "yes"] or (default and answer == ""):
            return True
        elif answer in ["n", "no"] or (not default and answer == ""):
            return False


@step(Style.BOLD + "--Validating arguments--" + Style.RESET)
def validate_arguments() -> None:
    if g_args.safe and g_args.yes:
        exit_error("The '-s' and '-y' flags cannot be used together.")

    if g_args.build_script is None and g_unknown:
        exit_error(
            f"Unknown arguments were detected: '{' '.join(g_unknown)}'. Note that you may only provide unknown arguments when a build script is provided. The unknown arguments will be forwarded to the build script."
        )

    if g_unknown:
        log(
            f"Unknown arguments detected: '{' '.join(g_unknown)}'. These will be forwarded to the build script at '{g_args.build_script}'."
        )

    if not g_args.safe:
        log(
            Style.BOLD
            + Style.BG_YELLOW
            + "Safe mode is disabled. This script may execute commands without prompting the user. Safe mode can be enabled with the '-s' or '--safe' flag."
            + Style.RESET
        )
        if not prompt("Do you wish to continue?"):
            exit_error()

    log("Arguments validated.")


@step(Style.BOLD + "--Validating OS--" + Style.RESET)
def validate_operating_system() -> None:
    os = get_os()
    log(f"Operating system: {os}")
    if not is_macos() and not is_windows() and not is_linux():
        exit_error(f"Unsupported operating system: {os}")

    if is_windows():
        log(f"Architecture: {os_architecture()}")
        log(f"ARM: {is_arm()}")

    if is_linux():
        global g_linux_devtools, g_linux_dependencies

        log(f"Distribution: {g_linux_distro} {g_linux_version}")
        if g_linux_distro is None:
            exit_error("Failed to detect Linux distribution.")

        if g_linux_version is None:
            exit_error("Failed to detect Linux version.")

        if g_linux_distro not in ["ubuntu", "fedora", "arch"]:
            exit_error(f"Unsupported Linux distribution: '{g_linux_distro}'.")

        if (
            g_linux_distro == "ubuntu"
            and g_linux_version != "20.04"
            and g_linux_version != "22.04"
        ):
            exit_error(f"Unsupported Ubuntu version: '{g_linux_version}'.")

        if (
            g_linux_distro == "fedora"
            and g_linux_version != "36"
            and g_linux_version != "37"
        ):
            exit_error(f"Unsupported Fedora version: '{g_linux_version}'.")

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

    log("Operating system validated.")


def run_process(
    command: str | list[str], /, *args, exit_on_decline: bool = True, **kwargs
) -> subprocess.CompletedProcess | None:
    if g_args.safe and not prompt(
        f"The command '{command if isinstance(command, str) else ' '.join(command)}' is about to be executed. Do you wish to continue?"
    ):
        if exit_on_decline:
            exit_error()
        return None

    return subprocess.run(command, *args, **kwargs)


def run_process_success(command: str | list[str], /, *args, **kwargs) -> bool:
    result = run_process(command, *args, **kwargs, exit_on_decline=False)
    return result is not None and result.returncode == 0


def run_file(path: Path | str, /) -> None:
    if g_args.safe and not prompt(
        f"The file '{path}' is about to be executed. Do you wish to continue?"
    ):
        exit_error()

    if isinstance(path, Path):
        path = str(path.resolve())
    if is_windows():
        os.startfile(path)
    elif is_linux():
        run_process(["xdg-open", path])
    elif is_macos():
        run_process(["open", path])


def prompt_to_install(dependency: str, /) -> bool:
    return prompt(f"'{dependency}' not found. Do you wish to install?")


def prompt_to_uninstall(dependency: str, /) -> bool:
    return prompt(f"'{dependency}' marked for uninstallation. Do you wish to continue?")


@step(Style.BOLD + "--Validating python version--" + Style.RESET)
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
            exit_error(
                f"Python version required: '{req_major}.{req_minor}.{req_micro}' Python version found: '{major}.{minor}.{micro}'.",
            )
        elif current > required:
            log(f"Valid python version detected: '{major}.{minor}.{micro}'.")
            return
    log(f"Valid python version detected: '{major}.{minor}.{micro}'.")


def is_python_package_installed(package: str, /) -> bool:
    if importlib.util.find_spec(package) is None:
        log(Style.FG_YELLOW + f"Package '{package}' not found" + Style.RESET)
        return False

    log(f"Package '{package}' found")
    return True


def try_install_python_package(package: str, /) -> bool:
    log(f"Installing python package '{package}'...")
    if not run_process_success([sys.executable, "-m", "pip", "install", package]):
        log(f"Failed to install '{package}'.")
        return False

    log(f"Successfully installed '{package}'.")
    return True


def try_uninstall_python_package(package: str, /) -> bool:
    log(f"Uninstalling python package '{package}'...")
    if not run_process_success(
        [sys.executable, "-m", "pip", "uninstall", "-y", package]
    ):
        log(f"Failed to uninstall '{package}'.")
        return False

    log(f"Successfully uninstalled '{package}'.")
    return True


@step(Style.BOLD + "--Validating python packages--" + Style.RESET)
def validate_python_packages(*packages: str) -> None:

    needs_restart = False
    for package in packages:
        exists = is_python_package_installed(package)
        needs_restart = needs_restart or not exists
        if not exists and (
            not prompt_to_install(package) or not try_install_python_package(package)
        ):
            exit_error()

    if needs_restart:
        exit_restart()

    log("All python packages found.")


@step(Style.BOLD + "--Uninstalling python packages--" + Style.RESET)
def uninstall_python_packages(*packages: str) -> None:

    uninstalled = False
    for package in packages:
        if not is_python_package_installed(package):
            continue

        if not prompt_to_uninstall(package):
            log(f"Skipping uninstallation of '{package}'.")
            continue
        uninstalled = try_uninstall_python_package(package) or uninstalled

    if uninstalled:
        log(
            Style.FG_YELLOW
            + "As one or more required python packages were uninstalled, this script will now terminate."
            + Style.RESET
        )
        exit_ok()


def is_linux_devtools_installed() -> bool:
    tools = ["gcc", "g++", "make"]

    installed = True
    for tool in tools:
        path = shutil.which(tool)
        if path is None:
            log(Style.FG_YELLOW + f"'{tool}' not found." + Style.RESET)
            installed = False
        else:
            log(f"'{tool}' found at '{path}'.")
    return installed


def is_linux_package_installed(package: str, /) -> bool:
    success = False
    if g_linux_distro == "ubuntu":
        success = run_process_success(["dpkg", "-l", package], capture_output=True)

    elif g_linux_distro == "fedora":
        success = run_process_success(
            ["dnf", "list", "installed", package],
            capture_output=True,
        )

    elif g_linux_distro == "arch":
        success = run_process_success(
            ["pacman", "-Q", package],
            capture_output=True,
        )

    if success:
        log(f"'{package}' found.")
        return True

    log(Style.FG_YELLOW + f"'{package}' not found." + Style.RESET)
    return False


def linux_install_package(
    package: str, /, *, ubuntu_update: str = False, group_install: str = False
) -> bool:
    log(f"Installing '{package}'...")
    success = False

    if g_linux_distro == "ubuntu":
        if ubuntu_update and not run_process_success(["sudo", "apt-get", "update"]):
            log("Failed to update apt-get")
        success = run_process_success(["sudo", "apt-get", "install", "-y", package])

    if g_linux_distro == "fedora":
        success = run_process_success(
            [
                "sudo",
                "dnf",
                "install" if not group_install else "groupinstall",
                "-y",
                package,
            ]
        )
    if g_linux_distro == "arch":
        success = run_process_success(["sudo", "pacman", "-S", "--noconfirm", package])

    if success:
        log(f"Successfully installed '{package}'.")
        return True

    log(f"Failed to install '{package}'.")
    return False


def linux_uninstall_package(package: str, /, *, group_remove: str = False) -> bool:
    log(f"Uninstalling '{package}'...")
    success = False

    if g_linux_distro == "ubuntu":
        success = run_process_success(
            ["sudo", "apt-get", "remove", "--purge", "-y", package]
        ) and run_process_success(["sudo", "apt-get", "autoremove", "-y"])
    if g_linux_distro == "fedora":
        success = run_process_success(
            [
                "sudo",
                "dnf",
                "remove" if not group_remove else "groupremove",
                "-y",
                package,
            ]
        )
    if g_linux_distro == "arch":
        success = run_process_success(
            ["sudo", "pacman", "-Rns", "--noconfirm", package]
        )

    if success:
        log(f"Successfully uninstalled '{package}'.")
        return True

    log(f"Failed to uninstall '{package}'.")
    return False


def try_install_linux_devtools() -> bool:
    return linux_install_package(
        g_linux_devtools, ubuntu_update=True, group_install=True
    )


def try_uninstall_linux_devtools() -> bool:
    return linux_uninstall_package(g_linux_devtools, group_remove=True)


@step(Style.BOLD + "--Validating linux devtools--" + Style.RESET)
def validate_linux_devtools() -> None:

    def install() -> None:
        if not prompt_to_install(g_linux_devtools) or not try_install_linux_devtools():
            exit_error()

    if not is_linux_devtools_installed():
        install()


@step(Style.BOLD + "--Uninstalling linux devtools--" + Style.RESET)
def uninstall_linux_devtools() -> None:
    if not is_linux_devtools_installed():
        return

    if not prompt_to_uninstall(g_linux_devtools):
        log(f"Skipping uninstallation of '{g_linux_devtools}'.")
        return

    try_uninstall_linux_devtools()


def is_xcode_command_line_tools_installed() -> bool:
    if not run_process_success(
        ["xcode-select", "-p"],
        capture_output=True,
    ):
        log(Style.FG_YELLOW + "'Xcode Command Line Tools' not found." + Style.RESET)
        return False

    log("'Xcode Command Line Tools' found.")
    return True


def try_install_xcode_command_line_tools() -> bool:
    log("Installing 'Xcode Command Line Tools'...")
    if not run_process_success(["xcode-select", "--install"]):
        log("Failed to install 'Xcode Command Line Tools'.")
        return False

    log("Successfully installed 'Xcode Command Line Tools'.")
    return True


def try_uninstall_xcode_command_line_tools() -> bool:
    log("Uninstalling 'Xcode Command Line Tools'...")
    if not run_process_success(
        ["sudo", "rm", "-rf", "/Library/Developer/CommandLineTools"]
    ):
        log("Failed to uninstall 'Xcode Command Line Tools'.")
        return False

    log("Successfully uninstalled 'Xcode Command Line Tools'.")
    return True


@step(Style.BOLD + "--Validating Xcode Command Line Tools--" + Style.RESET)
def validate_xcode_command_line_tools() -> None:

    def install() -> None:
        if (
            not prompt_to_install("Xcode Command Line Tools")
            or not try_install_xcode_command_line_tools()
        ):
            exit_error()

    if not is_xcode_command_line_tools_installed():
        install()


@step(Style.BOLD + "--Uninstalling Xcode Command Line Tools--" + Style.RESET)
def uninstall_xcode_command_line_tools() -> None:
    if not is_xcode_command_line_tools_installed():
        return

    if not prompt_to_uninstall("Xcode Command Line Tools"):
        log("Skipping uninstallation of 'Xcode Command Line Tools'")
        return

    try_uninstall_xcode_command_line_tools()


def is_vulkan_installed():
    def check_vulkaninfo() -> bool:
        exec = shutil.which("vulkaninfo")
        if exec is None:
            return False

        result = subprocess.run([exec], capture_output=True, text=True)
        return (
            result.returncode == 0
            and f"Vulkan Instance Version: {g_vulkan_version.no_micro()}"
            in result.stdout
        )

    if "VULKAN_SDK" in os.environ:
        log(
            f"Vulkan SDK environment variable ('VULKAN_SDK') found: {os.environ['VULKAN_SDK']}"
        )
        return True

    log(
        Style.FG_YELLOW
        + "Vulkan SDK environment variable ('VULKAN_SDK') not found."
        + Style.RESET
    )

    if check_vulkaninfo():
        log("'vulkaninfo' found.")
        return True

    log(Style.FG_YELLOW + "'vulkaninfo' not found." + Style.RESET)

    if is_macos() and Path("/usr/local/lib/libvulkan.dylib").exists():
        log("Vulkan SDK found at '/usr/local/lib/libvulkan.dylib'.")
        return True

    if is_macos():
        log(
            Style.FG_YELLOW
            + "Vulkan SDK not found at '/usr/local/lib/libvulkan.dylib'."
            + Style.RESET
        )

    if is_linux() and any(
        [
            Path("/usr/lib/x86_64-linux-gnu/libvulkan.so.1").exists(),
            Path("/usr/lib/libvulkan.so").exists(),
            Path("/usr/local/lib/libvulkan.so.1").exists(),
        ]
    ):
        log("Vulkan SDK libraries found in '/usr/lib' or '/usr/local/lib'.")
        return True

    if is_linux():
        log(
            Style.FG_YELLOW
            + "Vulkan SDK libraries not found in '/usr/lib' or '/usr/local/lib'."
            + Style.RESET
        )

    dll = (
        Path(os.environ.get("SystemRoot", "C:\\Windows")) / "System32" / "vulkan-1.dll"
    )
    vulkan_sdk = Path("C:\\VulkanSDK") / g_vulkan_version.__str__()
    if is_windows() and dll.exists() and vulkan_sdk.exists():
        log(
            f"Vulkan SDK found at '{vulkan_sdk}'. Installation validated with the presence of '{dll}'."
        )
        return True

    if is_windows():
        log(
            Style.FG_YELLOW
            + f"Vulkan SDK not found at '{vulkan_sdk}' or was not validated with the presence of '{dll}'."
            + Style.RESET
        )

    return False


def download_file(url: str, path: Path, /) -> None:
    import requests
    from tqdm import tqdm

    response = requests.get(url, stream=True, allow_redirects=True)
    total = int(response.headers.get("content-length", 0))
    one_kb = 1024

    with open(path, "wb") as f, tqdm(
        desc=path.name,
        total=total,
        unit="MiB",
        unit_scale=True,
        unit_divisor=one_kb * one_kb,
    ) as bar:
        for data in response.iter_content(chunk_size=one_kb):
            size = f.write(data)
            bar.update(size)


def extract_file(path: Path, dest: Path, /) -> None:
    dest.mkdir(exist_ok=True)

    if path.suffix == ".zip":
        with zipfile.ZipFile(path, "r") as zip_ref:
            zip_ref.extractall(dest)

    elif path.suffix in [".tar.gz", ".tar.xz"]:
        suffix = path.suffix.split(".")[-1]
        with tarfile.open(path, f"r:{suffix}") as tar_ref:
            tar_ref.extractall(dest)


def try_install_vulkan() -> bool:
    log("Installing Vulkan SDK...")

    vendor = g_root / "vendor"
    vendor.mkdir(exist_ok=True)
    if is_macos():
        extension = (
            "zip"
            if g_vulkan_version.minor > 3 or g_vulkan_version.patch > 290
            else "dmg"
        )
        filename = f"vulkansdk-macos-{g_vulkan_version}-installer.{extension}"
        osfolder = "mac"
    elif is_windows():
        filename = (
            f"VulkanSDK-{g_vulkan_version}-Installer.exe"
            if not is_arm()
            else f"InstallVulkanARM64-{g_vulkan_version}.exe"
        )
        osfolder = "windows" if not is_arm() else "warm"
    elif is_linux():
        extension = (
            "tar.xz"
            if g_vulkan_version.minor > 3 or g_vulkan_version.patch > 250
            else "tar.gz"
        )
        filename = f"vulkansdk-linux-x86_64-{g_vulkan_version}.{extension}"
        osfolder = "linux"

    download_path = vendor / filename
    url = (
        f"https://sdk.lunarg.com/sdk/download/{g_vulkan_version}/{osfolder}/{filename}"
    )
    if not download_path.exists():
        download_file(url, download_path)
    else:
        log(
            f"'{filename}' already downloaded. To trigger a re-download, delete the file at '{download_path}'."
        )

    def macos_install(installer_path: Path, /, *, include_version: bool = True) -> bool:
        name = (
            "InstallVulkan"
            if not include_version
            else f"InstallVulkan-{g_vulkan_version}"
        )
        path = str(installer_path / "Contents" / "MacOS" / name)
        vulkan_sdk = Path(f"~/VulkanSDK/{g_vulkan_version}").expanduser()
        if not run_process_success(
            [
                path,
                "--root",
                vulkan_sdk.__str__(),
                "--accept-licenses",
                "--default-answer",
                "--confirm-command",
                "install",
                "com.lunarg.vulkan.core",
                "com.lunarg.vulkan.usr",
            ]
        ):
            log(f"Failed to run the Vulkan SDK installer at '{path}'.")
            return False

        log(
            f"Successfully ran the Vulkan SDK installer at '{path}'. The SDK is installed at {vulkan_sdk}."
        )
        return True

    if filename.endswith(("zip", "tar.gz", "tar.xz")):
        extract_path = vendor / "vulkan-extract"

        if not extract_path.exists():
            extract_file(download_path, extract_path)
        else:
            log(
                f"'{extract_path}' already extracted. To trigger a re-extraction, delete the folder."
            )

        if is_macos():
            filepath = extract_path / f"InstallVulkan-{g_vulkan_version}.app"
            if not filepath.exists():
                filepath = vendor / f"InstallVulkan-{g_vulkan_version}.app"

            return macos_install(filepath)

        if is_linux():

            vulkan_sdk = extract_path / g_vulkan_version.__str__() / "x86_64"
            log(
                f"The Vulkan SDK has been successfully installed for the linux distribution {g_linux_distro} {g_linux_version} at '{vulkan_sdk}'."
            )
            log("You must now setup the environment. You can do so in different ways:")
            log(
                f"1. Use the source command with the 'setup-env.sh' script like so: '{vulkan_sdk / 'setup-env.sh'}'"
            )
            log("2. Execute the commands yourself:")
            log(f"   - export VULKAN_SDK={vulkan_sdk}")
            log(f"   - export PATH=$VULKAN_SDK/bin:$PATH")
            log(
                f"   - export LD_LIBRARY_PATH=$VULKAN_SDK/lib${{LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}}"
            )
            log(f"   - export VK_LAYER_PATH=$VULKAN_SDK/etc/vulkan/explicit_layer.d")
            log("3. Add the above commands to your shell profile file.")
            log(
                "\nNote that steps 1 and 2 will only set the environment variables for your current shell session. They do not permanently 'setup' any of this configuration for other shell sessions or future logins."
            )
            return True

    if is_macos():
        if not run_process_success(["hdiutil", "attach", download_path]):
            log("Failed to mount the Vulkan SDK disk image.")
            return False

        path = Path("/") / "Volumes" / "VulkanSDK" / "InstallVulkan.app"
        if not macos_install(path, include_version=False):
            return False

        if not run_process_success(["hdiutil", "detach", path]):
            log("Failed to unmount the Vulkan SDK disk image.")
            return False

        return True

    if is_windows():
        log(
            f"The Vulkan SDK installer will now run. Follow the instructions to install the SDK. Ensure the SDK binaries are installed at 'C:\\VulkanSDK\\{g_vulkan_version}'."
        )
        input("Press enter to begin the installation...")
        run_file(download_path)
        input("Press enter to continue once the installation is complete...")
        return True

    return False


def try_uninstall_vulkan() -> bool:
    if is_macos():
        path = Path("~/VulkanSDK").expanduser() / g_vulkan_version.__str__()

        if not path.exists():
            log(Style.FG_YELLOW + f"Vulkan SDK not found at '{path}'." + Style.RESET)
            return False

        log(f"Vulkan SDK found at '{path}'.")

        if not run_process_success(["sudo", str(path / "uninstall.sh")]):
            log(
                f"Failed to run the Vulkan SDK uninstaller at '{path / 'uninstall.sh'}'"
            )
            return False

        if not run_process_success(["rm", "-rf", str(path)]):
            log(f"Failed to remove the Vulkan SDK at '{path}'.")
            return False

        log(f"Successfully uninstalled Vulkan SDK at '{path}'.")
        return True

    if is_linux():
        if not run_process_success(
            ["rm", "-rf", str(g_root / "vendor" / "vulkan-extract")]
        ):
            log("Failed to remove the extracted Vulkan SDK.")
            return False

        log("Successfully uninstalled Vulkan SDK.")
        return True

    if is_windows():
        vulkan_sdk = Path("C:\\VulkanSDK") / g_vulkan_version.__str__()
        vulkan_uninstall = vulkan_sdk / "Bin" / "uninstall.exe"
        if not vulkan_uninstall.exists():
            log(
                Style.FG_YELLOW
                + f"Vulkan SDK uninstaller not found at '{vulkan_uninstall}'."
                + Style.RESET
            )
            return False

        if not run_process_success([str(vulkan_uninstall)]):
            log(f"Failed to run the Vulkan SDK uninstaller at '{vulkan_uninstall}'.")
            return False

        if not run_process_success(["rmdir", "/s", "/q", str(vulkan_sdk)]):
            log(f"Failed to remove the Vulkan SDK at '{vulkan_sdk}'.")
            return False

        log(f"Successfully uninstalled Vulkan SDK at '{vulkan_sdk}'.")
        return True


@step(Style.BOLD + "--Validating Vulkan SDK--" + Style.RESET)
def validate_vulkan() -> None:
    if is_linux():

        def install_dependency(dependency: str, /) -> None:
            if not prompt_to_install(dependency) or not linux_install_package(
                dependency
            ):
                exit_error()

        for dep in g_linux_dependencies:
            if not is_linux_package_installed(dep):
                install_dependency(dep)

    if not is_vulkan_installed() and (
        not prompt_to_install("Vulkan SDK") or not try_install_vulkan()
    ):
        exit_error()


@step(Style.BOLD + "--Uninstalling Vulkan SDK--" + Style.RESET)
def uninstall_vulkan() -> None:
    if is_linux():

        def try_uninstall_dependency(dependency: str, /) -> None:
            if not prompt_to_uninstall(dependency):
                log(f"Skipping uninstallation of '{dependency}'.")
                return

            linux_uninstall_package(dependency)

        for dep in g_linux_dependencies:
            if is_linux_package_installed(dep):
                try_uninstall_dependency(dep)

    if not is_vulkan_installed():
        return

    if not prompt_to_uninstall("Vulkan SDK"):
        log("Skipping uninstallation of 'Vulkan SDK'")
        return

    try_uninstall_vulkan()


def is_homebrew_installed() -> bool:
    brew_path = shutil.which("brew")
    if brew_path is None:
        log(Style.FG_YELLOW + "Homebrew not found." + Style.RESET)
        return False

    log(f"Homebrew found at '{brew_path}'.")
    return True


def try_install_homebrew() -> bool:
    log("Installing Homebrew...")
    if not run_process_success(
        [
            "/bin/bash",
            "-c",
            '"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"',
        ]
    ):
        log("Failed to install Homebrew.")
        return False

    log("Successfully installed Homebrew.")
    return True


def try_uninstall_homebrew() -> bool:
    if not run_process_success(
        [
            "/bin/bash",
            "-c",
            '"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/uninstall.sh)"',
        ]
    ):
        log("Failed to uninstall Homebrew.")
        return False

    log("Successfully uninstalled Homebrew.")
    return True


@step(Style.BOLD + "--Validating Homebrew--" + Style.RESET)
def validate_homebrew() -> None:
    if not is_homebrew_installed() and (
        not prompt_to_install("Homebrew") or not try_install_homebrew()
    ):
        exit_error()


@step(Style.BOLD + "--Uninstalling Homebrew--" + Style.RESET)
def uninstall_homebrew() -> None:
    if not is_homebrew_installed():
        return

    if not prompt_to_uninstall("Homebrew"):
        log("Skipping uninstallation of 'Homebrew'")
        return

    try_uninstall_homebrew()


def is_cmake_installed() -> bool:
    cmake_path = shutil.which("cmake")
    if cmake_path is None:
        log(Style.FG_YELLOW + "CMake not found." + Style.RESET)
        return False

    log(f"CMake found at '{cmake_path}'.")
    return True


def try_install_cmake() -> bool:
    if is_macos():
        log("Homebrew is needed to install CMake.")
        if is_homebrew_installed() and not (
            prompt_to_install("Homebrew") or not try_install_homebrew()
        ):
            return False

        log("Installing CMake...")
        if not run_process_success(["brew", "install", "cmake"]):
            log("Failed to install CMake.")
            return False

        log("Successfully installed CMake.")
        return True

    if is_linux():
        return linux_install_package("cmake")

    if is_windows():
        log("Installing CMake...")
        vendor = g_root / "vendor"
        vendor.mkdir(exist_ok=True)
        arch = "i386" if os_architecture() == "x86" else os_architecture()
        installer_url = f"https://github.com/Kitware/CMake/releases/download/v{g_cmake_version}/cmake-{g_cmake_version}-windows-{arch}.msi"
        installer_path = vendor / f"cmake-{g_cmake_version}-windows-{arch}.msi"

        log(
            f"Downloading CMake installer from '{installer_url}' into '{installer_path}'"
        )
        download_file(installer_url, installer_path)
        log(
            "The CMake installer will now run. Please follow the instructions and make sure the CMake executable is added to Path."
        )
        input("Press enter to begin the installation...")
        run_file(installer_path)
        input("Press enter to continue once the installation is complete...")
        return True


def try_uninstall_cmake() -> bool:
    if is_macos():
        log("Homebrew is needed to uninstall CMake.")
        if is_homebrew_installed() and not (
            prompt_to_install("Homebrew") or not try_install_homebrew()
        ):
            return False

        log("Uninstalling CMake...")
        if not run_process_success(["brew", "uninstall", "cmake"]):
            log("Failed to uninstall CMake.")
            return False

        log("Successfully uninstalled CMake.")
        return True

    if is_linux():
        return linux_uninstall_package("cmake")

    if is_windows():
        cmd_query = (
            "wmic product where \"name like '%CMake%'\" get Name, IdentifyingNumber"
        )
        result = subprocess.run(
            cmd_query, shell=True, stdout=subprocess.PIPE, text=True
        )
        if result.returncode != 0:
            log("Failed to query CMake installation.")
            return False

        output = result.stdout
        log(f"Query result: '{output}'.")

        guid = re.match(r"\{[0-9A-Fa-f\-]+\}", output)
        if guid is None:
            log(
                f"Failed to find CMake's guid in the query result. Pattern used: '{guid}'"
            )
            return False

        guid = guid.group()
        log(f"CMake found with guid: '{guid}'.")
        if not run_process_success(f"msiexec /x {guid}"):
            log(f"Failed to uninstall CMake with guid: '{guid}'.")
            return False

        log(f"Successfully uninstalled CMake with guid: '{guid}'.")
        return True

    return False


@step(Style.BOLD + "--Validating CMake--" + Style.RESET)
def validate_cmake() -> None:
    if not is_cmake_installed() and (
        not prompt_to_install("CMake") or not try_install_cmake()
    ):
        exit_error()


@step(Style.BOLD + "--Uninstalling CMake--" + Style.RESET)
def uninstall_cmake() -> None:
    if not is_cmake_installed():
        return

    if not prompt_to_uninstall("CMake"):
        log("Skipping uninstallation of 'CMake'")
        return

    try_uninstall_cmake()


@step(Style.BOLD + "--Executing build script--" + Style.RESET)
def execute_build_script() -> None:
    build_script: Path = g_args.build_script.resolve()
    if not build_script.exists():
        exit_error(f"Build script not found at '{build_script}'.")

    if not prompt(
        f"Build script provided at '{build_script}'. Do you wish to exexcute it?"
    ):
        exit_error()

    if build_script.suffix == ".py":
        cmd = [sys.executable, str(build_script), *g_unknown]
    else:
        cmd = [str(build_script), *g_unknown]

    if not run_process(cmd):
        exit_error(f"Failed to execute build script at '{build_script}'.")


if is_linux():
    g_linux_distro = get_linux_distro()
    g_linux_version = get_linux_version()
    g_linux_devtools = None
    g_linux_dependencies = None

g_args, g_unknown = parse_arguments()
g_root = Path(__file__).parent.resolve()

g_vulkan_version = VulkanVersion(*[int(v) for v in g_args.vulkan_version.split(".")])
if is_windows():
    g_cmake_version: str = g_args.cmake_version

validate_python_version(3, 10, 0)
validate_arguments()
validate_operating_system()

if not g_args.uninstall_python_packages:
    validate_python_packages("requests", "tqdm")
else:
    uninstall_python_packages("requests", "tqdm")


if not g_args.uninstall:
    if is_linux() and g_args.g_linux_devtools:
        validate_linux_devtools()

    if is_macos() and g_args.xcode_command_line_tools:
        validate_xcode_command_line_tools()

    if g_args.vulkan:
        validate_vulkan()

    if is_macos() and g_args.brew:
        validate_homebrew()

    if g_args.cmake:
        validate_cmake()
else:
    if is_linux() and g_args.g_linux_devtools:
        uninstall_linux_devtools()

    if is_macos() and g_args.xcode_command_line_tools:
        uninstall_xcode_command_line_tools()

    if g_args.vulkan:
        uninstall_vulkan()

    if is_macos() and g_args.brew:
        uninstall_homebrew()

    if g_args.cmake:
        uninstall_cmake()

if g_args.build_script is not None:
    execute_build_script()

exit_ok()
