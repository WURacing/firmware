"""Checks firmware source against the Google C++ style guide with cpplint.

Each season's settings live in its CPPLINT.cfg. Run from anywhere in the repo:
  python scripts/lint.py              # check files you changed
  python scripts/lint.py --all        # check everything
"""

import argparse
import subprocess
import sys

import firmware_tools


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    firmware_tools.add_file_selection_args(parser)
    args = parser.parse_args()

    files = firmware_tools.firmware_files(args)
    if not files:
        print("No changed source files to lint.")
        return 0

    return subprocess.run(
        firmware_tools.cpplint_command() + ["--quiet", *files],
        cwd=firmware_tools.REPO_ROOT).returncode


if __name__ == "__main__":
    sys.exit(main())
