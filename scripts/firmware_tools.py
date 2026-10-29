"""Shared helpers for format.py and lint.py.

Finds the firmware source files the style rules apply to and the pinned
clang-format and cpplint tools. Works on Windows and macOS.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
INSTALL_HINT = ("Install the style tools with:\n"
                "  python -m pip install -r scripts/requirements.txt")

_SOURCE_PATTERN = re.compile(r"\.(ino|h|hpp|c|cpp)$")


def add_file_selection_args(parser: argparse.ArgumentParser) -> None:
    """Adds the --all / --staged / BASE_REF options shared by the scripts."""
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument(
        "--all", action="store_true",
        help="check every source file, not just the ones you changed")
    selection.add_argument(
        "--staged", action="store_true",
        help="check only files staged for commit")
    parser.add_argument(
        "base_ref", nargs="?", default="origin/dev",
        help="branch to compare against (default: origin/dev)")


def _git(*args: str) -> list[str]:
    result = subprocess.run(
        ["git", *args], cwd=REPO_ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"git {' '.join(args)} failed:\n{result.stderr.strip()}")
    return result.stdout.splitlines()


def season_dirs() -> list[str]:
    """Returns the season folders that opt in to the style rules.

    A season opts in by having a CPPLINT.cfg. Older seasons are archived as-is.
    """
    return sorted(config.parent.name
                  for config in REPO_ROOT.glob("*/CPPLINT.cfg"))


def firmware_files(args: argparse.Namespace) -> list[str]:
    """Returns the source files selected by `args`, relative to the repo root.

    By default only files changed from `args.base_ref` are returned, so
    untouched code isn't flagged until someone edits it.
    """
    seasons = season_dirs()
    if not seasons:
        return []
    if args.all:
        files = _git("ls-files", "--", *seasons)
    elif args.staged:
        files = _git("diff", "--cached", "--name-only", "--diff-filter=ACMR",
                     "--", *seasons)
    else:
        # Committed changes on this branch, plus staged and unstaged edits.
        files = _git("diff", "--name-only", "--diff-filter=ACMR",
                     f"{args.base_ref}...HEAD", "--", *seasons)
        files += _git("diff", "--name-only", "--diff-filter=ACMR", "HEAD",
                      "--", *seasons)
    return sorted({f for f in files if _SOURCE_PATTERN.search(f)})


def clang_format_command() -> list[str]:
    """Returns the command for the pinned clang-format.

    Prefers the copy installed from requirements.txt, since other versions may
    format code slightly differently.
    """
    try:
        import clang_format  # pylint: disable=import-outside-toplevel
        executable = pathlib.Path(clang_format.get_executable("clang-format"))
        if executable.exists():
            return [str(executable)]
    except ImportError:
        pass
    executable = shutil.which("clang-format")
    if executable is None:
        sys.exit(f"clang-format not found. {INSTALL_HINT}")
    print(f"Warning: using {executable}, not the pinned clang-format. "
          f"{INSTALL_HINT}", file=sys.stderr)
    return [executable]


def cpplint_command() -> list[str]:
    """Returns the command for cpplint, run through this Python."""
    try:
        import cpplint  # pylint: disable=import-outside-toplevel,unused-import
    except ImportError:
        sys.exit(f"cpplint not found. {INSTALL_HINT}")
    return [sys.executable, "-m", "cpplint"]
