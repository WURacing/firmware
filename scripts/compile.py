"""Compiles every sketch in each season folder with arduino-cli.

Finds each sketch (a folder holding a .ino of the same name) under every
season with a CPPLINT.cfg, compiles all of them even when some fail, and
prints a summary. The board is the sketch's sketch.yaml `default_fqbn`, or
the Feather M4 CAN. Run from anywhere in the repo:
  python scripts/compile.py                     # every sketch
  python scripts/compile.py 2026/ControlBoard   # just these sketch folders
In GitHub Actions, errors also become annotations on the commit, and the
summary is added to the run page.

Needs arduino-cli with the Adafruit SAMD core and the libraries listed in
.github/workflows/firmware-compile.yml.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys

import firmware_tools

DEFAULT_FQBN = "adafruit:samd:adafruit_feather_m4_can"

# Team libraries kept in the repo instead of the Library Manager. Copy these
# folders into your Arduino libraries folder to build locally.
LOCAL_LIBRARIES = [
    "2024/BMX160",
    "2026/VCSEL Sensor/Recursive Update lib",
]

_COMPILER_MESSAGE = re.compile(
    r"^(?P<file>[^:\s][^:]*):(?P<line>\d+):(?:(?P<col>\d+):)? "
    r"(?P<level>fatal error|error|warning): (?P<message>.*)$")
_LINKER_ERROR = re.compile(
    r"(multiple definition of|undefined reference to) .*")

GITHUB = os.environ.get("GITHUB_ACTIONS") == "true"


def find_sketches(selected: list[str]) -> tuple[list[pathlib.Path],
                                                list[pathlib.Path]]:
    """Returns (sketch folders, .ino files that aren't in a sketch folder)."""
    if selected:
        return [firmware_tools.REPO_ROOT / s for s in selected], []
    sketches, strays = [], []
    for season in firmware_tools.season_dirs():
        for ino in sorted((firmware_tools.REPO_ROOT / season).rglob("*.ino")):
            relative = ino.relative_to(firmware_tools.REPO_ROOT)
            if "tests" in relative.parts:
                continue
            if ino.parent.name == ino.stem:
                sketches.append(ino.parent)
            else:
                strays.append(ino)
    return sketches, strays


def board_for(sketch: pathlib.Path) -> str:
    project = sketch / "sketch.yaml"
    if project.exists():
        match = re.search(r"^default_fqbn:\s*(\S+)", project.read_text(),
                          re.MULTILINE)
        if match:
            return match.group(1)
    return DEFAULT_FQBN


def relative(path: str) -> str:
    """Makes `path` repo-relative when it's inside the repo."""
    try:
        return str(pathlib.Path(path).resolve().relative_to(
            firmware_tools.REPO_ROOT)).replace(os.sep, "/")
    except ValueError:
        return path


def annotate(level: str, message: str, title: str, file: str = "",
             line: str = "", col: str = "") -> None:
    if not GITHUB:
        return
    where = ",".join(part for part in (
        f"file={file}" if file else "", f"line={line}" if line else "",
        f"col={col}" if col else "", f"title={title}") if part)
    # Annotations are one line; escape what GitHub would otherwise parse.
    message = message.replace("%", "%25").replace("\r", "").replace(
        "\n", "%0A")
    print(f"::{level} {where}::{message}", flush=True)


def compile_sketch(sketch: pathlib.Path, fqbn: str) -> list[str]:
    """Compiles one sketch and returns its error messages (empty = built)."""
    name = relative(str(sketch))
    main_file = f"{name}/{sketch.name}.ino"
    command = ["arduino-cli", "compile", "--fqbn", fqbn, "--warnings",
               "default"]
    for library in LOCAL_LIBRARIES:
        command += ["--library", str(firmware_tools.REPO_ROOT / library)]
    command.append(str(sketch))

    if GITHUB:
        print(f"::group::{name} ({fqbn})", flush=True)
    else:
        print(f"\n=== {name} ({fqbn})", flush=True)
    result = subprocess.run(command, cwd=firmware_tools.REPO_ROOT,
                            capture_output=True, text=True)
    output = (result.stdout + result.stderr).replace(
        str(firmware_tools.REPO_ROOT) + os.sep, "")
    print(output, flush=True)
    if GITHUB:
        print("::endgroup::", flush=True)
    if result.returncode == 0:
        return []

    errors = []
    title = f"Compile {name}"
    for raw in output.splitlines():
        match = _COMPILER_MESSAGE.match(raw)
        if match:
            level = "warning" if match["level"] == "warning" else "error"
            file = relative(match["file"])
            # Errors inside a library point at the sketch that pulled it in.
            if not (firmware_tools.REPO_ROOT / file).exists():
                file, line, col = main_file, "", ""
                message = f"{match['file']}:{match['line']}: {match['message']}"
            else:
                line, col, message = match["line"], match["col"] or "", match[
                    "message"]
            annotate(level, message, title, file, line, col)
            if level == "error":
                errors.append(f"{file}:{line}: {message}" if line else message)
            continue
        linker = _LINKER_ERROR.search(raw)
        if linker:
            annotate("error", f"Link error: {linker.group(0)}", title,
                     main_file)
            errors.append(f"link: {linker.group(0)}")
    if not errors:
        tail = "\n".join(output.strip().splitlines()[-5:])
        annotate("error", tail or "arduino-cli failed", title, main_file)
        errors.append(tail.splitlines()[-1] if tail else "arduino-cli failed")
    return errors


def write_summary(rows: list[tuple[str, str, list[str]]],
                  strays: list[pathlib.Path]) -> None:
    lines = ["## Firmware compile", "",
             "| Sketch | Board | Result |", "| --- | --- | --- |"]
    for name, fqbn, errors in rows:
        if errors:
            first = errors[0].replace("|", "\\|")
            more = f" (+{len(errors) - 1} more)" if len(errors) > 1 else ""
            result = f"❌ `{first}`{more}"
        else:
            result = "✅"
        lines.append(f"| `{name}` | `{fqbn.split(':')[-1]}` | {result} |")
    for stray in strays:
        lines.append(f"| `{relative(str(stray))}` | - | ❌ not in a sketch "
                     "folder of the same name |")
    lines += ["", "Reproduce locally with `python scripts/compile.py "
              "<sketch folder>`. Each error is also annotated on the commit."]
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("sketches", nargs="*",
                        help="sketch folders to compile (default: all)")
    args = parser.parse_args()
    if shutil.which("arduino-cli") is None:
        sys.exit("arduino-cli not found. See "
                 ".github/workflows/firmware-compile.yml for the setup.")

    sketches, strays = find_sketches(args.sketches)
    for stray in strays:
        annotate("error",
                 "Not built: a sketch's .ino must be in a folder of the same "
                 "name, or the Arduino IDE and arduino-cli can't open it. "
                 "Move it into its own folder or delete it.",
                 "Compile", relative(str(stray)))

    rows = []
    for sketch in sketches:
        fqbn = board_for(sketch)
        rows.append((relative(str(sketch)), fqbn, compile_sketch(sketch,
                                                                 fqbn)))

    failed = [name for name, _, errors in rows if errors]
    print(f"\nCompiled {len(rows) - len(failed)} of {len(rows)} sketches.")
    for name, _, errors in rows:
        if errors:
            print(f"  FAILED {name}: {len(errors)} error(s), first: "
                  f"{errors[0]}")
    for stray in strays:
        print(f"  FAILED {relative(str(stray))}: not in a sketch folder of "
              "the same name")
    write_summary(rows, strays)
    return 1 if failed or strays else 0


if __name__ == "__main__":
    sys.exit(main())
