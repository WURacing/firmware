"""Builds and runs the firmware host tests with CMake and GoogleTest.

Each season with a tests/CMakeLists.txt is built for this computer against
the fake Arduino core in <season>/tests/fakes. Every board's tests are built
even if another board's fail to compile, and a summary at the end lists what
failed. Run from anywhere in the repo:
  python scripts/test.py              # build and run every season's tests
  python scripts/test.py -R Clutch    # only tests whose name matches "Clutch"
Needs CMake 3.16+ and a C++ compiler (Xcode command line tools on macOS,
Visual Studio Build Tools on Windows). GoogleTest is downloaded on first run.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

import firmware_tools

# Multi-config generators (Visual Studio) need a config name; others ignore it.
_CONFIG = "Debug"
_GITHUB = os.environ.get("GITHUB_ACTIONS") == "true"


def _run(command: list[str], capture: bool = False):
    print("+ " + " ".join(command), flush=True)
    return subprocess.run(command, cwd=firmware_tools.REPO_ROOT, text=True,
                          capture_output=capture)


def _test_targets(cmake_lists) -> list[str]:
    """Returns the add_sketch_test() target names in a CMakeLists.txt."""
    return re.findall(r"^add_sketch_test\(\s*(\w+)", cmake_lists.read_text(),
                      re.MULTILINE)


def _start_group(title: str) -> None:
    print(f"::group::{title}" if _GITHUB else f"\n=== {title}", flush=True)


def _end_group() -> None:
    if _GITHUB:
        print("::endgroup::", flush=True)


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

    build_failures = []
    test_failures = []
    for cmake_lists in test_dirs:
        source = cmake_lists.parent
        build = firmware_tools.REPO_ROOT / "build" / f"{source.parent.name}-tests"
        if _run(["cmake", "-S", str(source), "-B", str(build)]).returncode:
            build_failures.append(f"{source} (CMake configure)")
            continue
        # Build each board's tests on its own, so one that doesn't compile
        # doesn't stop the others from being built and run.
        for target in _test_targets(cmake_lists):
            _start_group(f"Build {target}")
            result = _run(["cmake", "--build", str(build), "--config", _CONFIG,
                           "--parallel", "--target", target])
            _end_group()
            if result.returncode:
                build_failures.append(target)
                print(f"::error title=Host tests::{target} does not compile; "
                      f"see the 'Build {target}' log group."
                      if _GITHUB else f"{target} does not compile.",
                      flush=True)

        ctest = ["ctest", "--test-dir", str(build), "-C", _CONFIG,
                 "--output-on-failure"]
        if args.tests_regex:
            ctest += ["-R", args.tests_regex]
        result = _run(ctest, capture=True)
        print(result.stdout + result.stderr, flush=True)
        test_failures += [
            line.split(" - ", 1)[1].split(" (")[0].strip()
            for line in result.stdout.splitlines()
            if re.match(r"^\s*\d+ - ", line) and "_NOT_BUILT" not in line]

    print("\nSummary:")
    for target in build_failures:
        print(f"  DOES NOT COMPILE: {target}")
    for test in test_failures:
        print(f"  FAILED: {test}")
    if not build_failures and not test_failures:
        print("  All host tests passed.")

    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        lines = ["## Host tests", ""]
        lines += [f"- ❌ `{t}` does not compile" for t in build_failures]
        lines += [f"- ❌ `{t}` failed" for t in test_failures]
        if not build_failures and not test_failures:
            lines.append("- ✅ All host tests passed.")
        lines += ["", "Reproduce locally with `python scripts/test.py`."]
        with open(summary, "a", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
    return 1 if build_failures or test_failures else 0


if __name__ == "__main__":
    sys.exit(main())
