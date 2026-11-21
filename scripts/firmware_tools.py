"""Shared helpers for format.py and lint.py.

Finds the firmware source files the style rules apply to and the pinned
clang-format and cpplint tools. Works on Windows and macOS.
"""

from __future__ import annotations

import argparse
import os
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


# Compiler and linker output, shared by compile.py and test.py.

GITHUB_ACTIONS = os.environ.get("GITHUB_ACTIONS") == "true"

_COMPILER_MESSAGE = re.compile(
    r"^(?P<file>[^:\s][^:]*?):(?P<line>\d+):(?:(?P<col>\d+):)? "
    r"(?P<level>fatal error|error|warning): (?P<message>.*)$")
_LINKER_ERROR = re.compile(r"(multiple definition of|undefined reference to)"
                           r" [`'‘]([^'’]+)['’]")


_INCLUDED_FROM = re.compile(
    r"^(?:In file included from|\s+from) (?P<file>.+?):(?P<line>\d+)[,:]$")


class CompilerError:
    """One compiler or linker error.

    `file` and `line` are where to point the developer: the repo file with
    the error, or, for an error inside a library or fake header, the line in
    our code that included it (`library` then names the header). Both are ""
    for linker errors and for errors with no repo file involved, which keep
    the raw location in `outside`.
    """

    def __init__(self, file: str, line: str, col: str, message: str,
                 outside: str = "", library: str = ""):
        self.file, self.line, self.col = file, line, col
        self.message, self.outside, self.library = message, outside, library

    @property
    def in_tests(self) -> bool:
        return "/tests/" in f"/{self.file}"

    @property
    def double_include(self) -> bool:
        return bool(self.library) and self.message.startswith("redefinition")

    def __str__(self) -> str:
        if self.file:
            via = f" (in {self.library})" if self.library else ""
            return f"{self.file}:{self.line}{via}: {self.message}"
        return f"{self.outside}: {self.message}" if self.outside else (
            self.message)


def repo_relative(path: str) -> str:
    """Returns `path` relative to the repo, or "" if it's outside it or in a
    build folder."""
    candidate = pathlib.Path(path)
    if not candidate.is_absolute():
        candidate = REPO_ROOT / candidate
    try:
        rel = candidate.resolve().relative_to(REPO_ROOT)
    except (ValueError, OSError):
        return ""
    if rel.parts and rel.parts[0] == "build" or not (REPO_ROOT / rel).exists():
        return ""
    return rel.as_posix()


def _is_library(rel: str) -> bool:
    """True for headers we didn't write: outside the repo, or test fakes."""
    return not rel or "/tests/fakes/" in f"/{rel}"


def parse_compiler_errors(output: str) -> list[CompilerError]:
    """Returns the distinct errors in compiler or linker `output`."""
    errors, seen = [], set()
    chain: list[tuple[str, str]] = []  # Innermost include first.
    for raw in output.splitlines():
        text = raw.strip() if raw.startswith(" ") and " from " not in raw else (
            raw)
        included = _INCLUDED_FROM.match(text)
        if included:
            chain.append((included["file"], included["line"]))
            continue
        match = _COMPILER_MESSAGE.match(text.strip())
        if match:
            includes, chain = chain, []
            if match["level"] == "warning":
                continue
            rel = repo_relative(match["file"])
            message = match["message"]
            if not _is_library(rel):
                error = CompilerError(rel, match["line"], match["col"] or "",
                                      message)
            else:
                header = rel or match["file"]
                header_line = f"{pathlib.PurePath(header).name}:{match['line']}"
                via = next(((repo_relative(f), n) for f, n in includes
                            if not _is_library(repo_relative(f))), None)
                if message.startswith("redefinition"):
                    message += (" (this library header has no include guard"
                                " and the sketch includes it twice, e.g."
                                " <CAN.h> and <CANSAME5x.h>; include only"
                                " one)")
                if via:
                    error = CompilerError(via[0], via[1], "", message,
                                          library=header_line)
                else:
                    error = CompilerError("", "", "", message,
                                          f"{header}:{match['line']}")
        else:
            if not text.startswith(("In function", "In member")) and (
                    "|" not in text):
                chain = []
            linker = _LINKER_ERROR.search(raw)
            if not linker:
                continue
            what, symbol = linker.group(1), linker.group(2)
            message = f"link error: {what} `{symbol}`"
            if what == "multiple definition of":
                message += (" (defined in the sketch and again in a library or"
                            " another file")
                message += ("; CAN Adafruit Fork already defines `CAN`, so "
                            "delete the sketch's `CANSAME5x CAN;`)"
                            if symbol == "CAN" else ")")
            error = CompilerError("", "", "", message)
        key = str(error)
        if key not in seen:
            seen.add(key)
            errors.append(error)
    return errors


def ranked_errors(errors: list[CompilerError]) -> list[CompilerError]:
    """Orders errors so the ones to fix first come first.

    Firmware errors (including a library header included twice), then test
    code, then link errors, then other errors inside library headers (shown
    at the line that included them; usually follow-ons), then errors with no
    repo file at all, which only matter when there is nothing else.
    """
    firmware = [e for e in errors
                if e.file and not e.in_tests and (not e.library or
                                                  e.double_include)]
    tests = [e for e in errors if e.file and e.in_tests and not e.library]
    linker = [e for e in errors if not e.file and not e.outside]
    via_library = [e for e in errors if e.file and e.library and
                   not (not e.in_tests and e.double_include)]
    outside = [e for e in errors if e.outside]
    ours = firmware + tests + linker + via_library
    return ours if ours else outside


def annotate(level: str, message: str, title: str, file: str = "",
             line: str = "", col: str = "") -> None:
    """Prints a GitHub Actions annotation (does nothing outside Actions)."""
    if not GITHUB_ACTIONS:
        return
    fields = [f"file={file}" if file else "", f"line={line}" if line else "",
              f"col={col}" if col else "", f"title={title}"]
    message = message.replace("%", "%25").replace("\r", "").replace(
        "\n", "%0A")
    print(f"::{level} {','.join(f for f in fields if f)}::{message}",
          flush=True)


def report_errors(errors: list[CompilerError], title: str,
                  fallback_file: str = "", limit: int = 15) -> None:
    """Prints the ranked errors as a short list and annotates them."""
    ranked = ranked_errors(errors)
    if not ranked:
        return
    print(f"\n{title}: {len(ranked)} error(s) in our code"
          if ranked[0].file or not ranked[0].outside else
          f"\n{title}: errors only in libraries", flush=True)
    for error in ranked[:limit]:
        print(f"  {error}", flush=True)
        annotate("error", error.message if error.file else str(error), title,
                 error.file or fallback_file, error.line, error.col)
    if len(ranked) > limit:
        print(f"  ... and {len(ranked) - limit} more", flush=True)
