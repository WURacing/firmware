"""Builds and runs the firmware host tests with CMake and GoogleTest.

Each season with a tests/CMakeLists.txt is built for this computer against
the fake Arduino core in <season>/tests/fakes. Run from anywhere in the repo:
  python scripts/test.py              # build and run every season's tests
  python scripts/test.py -R Clutch    # only tests whose name matches "Clutch"
Needs CMake 3.16+ and a C++ compiler (Xcode command line tools on macOS,
Visual Studio Build Tools on Windows). GoogleTest is downloaded on first run.
"""

import argparse
import shutil
import subprocess
import sys

import firmware_tools

# Multi-config generators (Visual Studio) need a config name; others ignore it.
_CONFIG = "Debug"


def _run(command: list[str]) -> int:
    print("+ " + " ".join(command), flush=True)
    return subprocess.run(command, cwd=firmware_tools.REPO_ROOT).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "-R", "--tests-regex", metavar="REGEX",
        help="only run tests whose name matches REGEX")
    args = parser.parse_args()

    if shutil.which("cmake") is None:
        sys.exit("cmake not found. Install CMake 3.16+ and a C++ compiler.")

    test_dirs = sorted(firmware_tools.REPO_ROOT.glob("*/tests/CMakeLists.txt"))
    if not test_dirs:
        print("No season has host tests.")
        return 0

    failed = False
    for cmake_lists in test_dirs:
        source = cmake_lists.parent
        build = firmware_tools.REPO_ROOT / "build" / f"{source.parent.name}-tests"
        ctest = ["ctest", "--test-dir", str(build), "-C", _CONFIG,
                 "--output-on-failure"]
        if args.tests_regex:
            ctest += ["-R", args.tests_regex]
        if (_run(["cmake", "-S", str(source), "-B", str(build)]) != 0
                or _run(["cmake", "--build", str(build), "--config", _CONFIG,
                         "--parallel"]) != 0
                or _run(ctest) != 0):
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
