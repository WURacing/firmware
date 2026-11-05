"""Prepares a sketch for the host-test build the way the Arduino builder does.

Writes a copy of the sketch's .ino files (the main one first, then the rest
alphabetically) with `#include <Arduino.h>` at the top and a prototype for
every function defined in them inserted before the first definition. Without
this, a sketch that compiles in the Arduino IDE could fail in the host tests
just because a function is used before it is defined. `#line` directives keep
compiler errors pointing at the original files and lines.

Used by tests/CMakeLists.txt:
  python arduino_preprocess.py <sketch folder> <output .ino>
"""

import pathlib
import re
import sys

_NOT_FUNCTIONS = {"if", "for", "while", "switch", "return", "else", "do",
                  "sizeof", "case"}
_DEFINITION = re.compile(
    r"^(?P<type>[A-Za-z_][\w:<>,]*(?:[\s\*&]+[A-Za-z_][\w:<>,]*)*[\s\*&]+)"
    r"(?P<name>[A-Za-z_]\w*)\s*\((?P<params>[^;{}]*)\)\s*(?:const\s*)?"
    r"(?P<brace>\{.*)?$")
_SKIP_PREFIXES = ("#", "//", "/*", "*", "class ", "struct ", "enum ",
                  "typedef ", "namespace ", "template", "using ", "extern ",
                  "static_assert", "}")


def _code_only(line: str, in_comment: bool) -> tuple[str, bool]:
    """Removes comments and string/char literals from one line."""
    out = []
    i = 0
    while i < len(line):
        if in_comment:
            end = line.find("*/", i)
            if end < 0:
                return "".join(out), True
            in_comment = False
            i = end + 2
        elif line.startswith("//", i):
            break
        elif line.startswith("/*", i):
            in_comment = True
            i += 2
        elif line[i] in "\"'":
            quote = line[i]
            i += 1
            while i < len(line) and line[i] != quote:
                i += 2 if line[i] == "\\" else 1
            i += 1
            out.append('""')
        else:
            out.append(line[i])
            i += 1
    return "".join(out), in_comment


def _prototypes(lines: list[str]) -> tuple[list[str], int]:
    """Returns (prototypes, index of the first function definition)."""
    prototypes = []
    first = -1
    depth = 0
    in_comment = False
    code_lines = []
    for line in lines:
        code, in_comment = _code_only(line, in_comment)
        code_lines.append(code)
    for i, code in enumerate(code_lines):
        stripped = code.strip()
        if (depth == 0 and code[:1] not in (" ", "\t", "") and
                not stripped.startswith(_SKIP_PREFIXES)):
            match = _DEFINITION.match(stripped)
            next_line = next((c.strip() for c in code_lines[i + 1:]
                              if c.strip()), "")
            if (match and match["name"] not in _NOT_FUNCTIONS and
                    not match["type"].strip().startswith(("return", "else"))
                    and (match["brace"] or next_line.startswith("{"))):
                params = re.sub(r"\s*=\s*[^,]+", "", match["params"])
                prototypes.append(
                    f"{match['type'].strip()} {match['name']}({params});")
                if first < 0:
                    first = i
        depth += code.count("{") - code.count("}")
        depth = max(depth, 0)
    return prototypes, first


def main() -> int:
    sketch = pathlib.Path(sys.argv[1]).resolve()
    output = pathlib.Path(sys.argv[2])
    main_ino = sketch / f"{sketch.name}.ino"
    others = sorted(p for p in sketch.glob("*.ino") if p != main_ino)

    lines, origins = [], []
    for ino in [main_ino, *others]:
        for number, line in enumerate(
                ino.read_text(encoding="utf-8").splitlines(), start=1):
            lines.append(line)
            origins.append((ino, number))

    prototypes, first = _prototypes(lines)
    out = ["#include <Arduino.h>"]
    current_file = None
    for index, line in enumerate(lines):
        ino, number = origins[index]
        if index == first and prototypes:
            out += prototypes
            current_file = None
        if ino != current_file:
            out.append(f'#line {number} "{ino.as_posix()}"')
            current_file = ino
        out.append(line)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(out) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
