"""Formats firmware source with clang-format (Google style).

Each season's rules live in its .clang-format. Run from anywhere in the repo:
  python scripts/format.py            # fix files you changed
  python scripts/format.py --check    # only report problems
"""

import argparse
import subprocess
import sys

import firmware_tools


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--check", action="store_true",
        help="don't modify files; fail if anything needs formatting")
    firmware_tools.add_file_selection_args(parser)
    args = parser.parse_args()

    files = firmware_tools.firmware_files(args)
    if not files:
        print("No changed source files to format.")
        return 0

    mode = ["--dry-run", "--Werror"] if args.check else ["-i"]
    result = subprocess.run(
        firmware_tools.clang_format_command() + mode + ["--style=file", *files],
        cwd=firmware_tools.REPO_ROOT)
    if result.returncode != 0:
        print("Formatting issues found. Run `python scripts/format.py` to fix "
              "them.", file=sys.stderr)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
