from argparse import ArgumentParser, Namespace
from pathlib import Path

import subprocess
import time


def parse_arguments() -> tuple[Namespace, list[str]]:
    desc = """
    This python script automates the profiling process of the fluid simulation by executing it at the same time the
    profiler is running so that it can generate a trace.
    """
    parser = ArgumentParser(description=desc)
    parser.add_argument(
        "-o", "--output", type=Path, required=True, help="The path to the output trace."
    )
    parser.add_argument(
        "--drizzle-exec",
        type=Path,
        default=None,
        help="The path to the 'drizzle' executable. Default is '<root>/build/drizzle/drizzle'.",
    )
    parser.add_argument(
        "--profiler-exec",
        type=Path,
        default=None,
        help="The path to the 'drizzle' executable. Default is '<root>/build/profiler/cli/tracy-capture'.",
    )
    parser.add_argument(
        "-s",
        "--seconds",
        "--run-time",
        type=float,
        default=None,
        help="The amount of time the profiler will run in seconds. If not specified, the profiler will run indefinitely.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Print more information.",
    )
    parser.add_argument(
        "-f",
        "--overwrite",
        action="store_true",
        default=False,
        help="If output exists, overwrite the file.",
    )

    return parser.parse_known_args()


def main() -> None:
    args, unknown = parse_arguments()
    output: Path = args.output

    output.parent.mkdir(exist_ok=True, parents=True)

    drizzle_exec = args.drizzle_exec
    if drizzle_exec is None:
        drizzle_exec = Path(__file__).parent.parent / "build" / "drizzle" / "drizzle"

    profiler_exec: Path = args.profiler_exec
    if profiler_exec is None:
        profiler_exec = (
            Path(__file__).parent.parent
            / "build"
            / "profiler"
            / "cli"
            / "tracy-capture"
        )

    def log(*pargs, **kwargs) -> None:
        if args.verbose:
            print(*pargs, **kwargs)

    rt: float = args.seconds

    profiler = [str(profiler_exec), "-o", str(output), "-s", str(rt)]
    if args.overwrite:
        profiler.append("-f")

    drizzle = [str(drizzle_exec), "--run-time", str(rt + 1.0)] + unknown

    log(f"Executing profiler: {' '.join(profiler)}")
    log(f"Executing drizzle: {' '.join(drizzle)}")

    p1 = subprocess.Popen(drizzle)
    time.sleep(0.5)
    p2 = subprocess.Popen(profiler)

    rcode = p1.wait()
    if rcode != 0:
        p2.kill()
    else:
        p2.wait()


if __name__ == "__main__":
    main()
