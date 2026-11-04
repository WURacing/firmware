# 2026 Code Standards

The team standard (WUFR23 *Code and GitHub Standards*) is the
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
This page is the short version for our Arduino sketches, plus the tools that
check it for you. `FSB_CODE/`, `ControlBoard/`, and
`Telemetry/Telemetry_Sender/` follow it and are good examples to copy from.

## Setup (once)

You need Python 3.8+ and Git. Run these from the repo root, in Terminal on
macOS or PowerShell/cmd on Windows. On macOS, type `python3` instead of
`python`.

```bash
python -m pip install -r scripts/requirements.txt
python scripts/install_hooks.py   # optional: check staged files on commit
```

`requirements.txt` pins the exact clang-format and cpplint versions, so
Windows and Mac machines format code identically.

In VS Code, install the *C/C++* extension and turn on **Format On Save**. It
picks up `2026/.clang-format` automatically. Its bundled clang-format can
differ slightly from the pinned one, so `scripts/format.py` has the final say.

## Before you open a PR

```bash
python scripts/format.py   # fixes formatting in files you changed
python scripts/lint.py     # reports style problems in files you changed
```

Both only look at files that differ from `origin/dev`, so you are never asked
to fix code you didn't touch. Pass a branch name to compare against something
else (`python scripts/lint.py master`), or `--all` to check everything.

## CI

GitHub Actions runs the same two checks on every push
(`.github/workflows/firmware-style.yml`). Pushes to `dev` are compared against
`master`, and pushes to any other branch are compared against `dev`. A red ❌
on your commit means one of them failed. Open the run to see which lines, then
fix them locally:

```bash
python scripts/format.py origin/dev   # on dev, use origin/master
python scripts/lint.py origin/dev
```

## Host tests

`tests/` builds the real sketches for your computer against fake versions of
`Arduino.h`, `CAN.h`, `Servo.h`, and `SPI.h` (in `tests/fakes/`). A test sets
the inputs (paddle pins, analog readings), calls `setup()` and `loop()`, and
checks what came out: which pins went HIGH and for how long, what angle the
servo was told, and which CAN frames were sent. Time only moves when the
sketch calls `delay()` or the test advances it, so timing checks are exact.

```bash
python scripts/test.py              # build and run all tests
python scripts/test.py -R Clutch    # only tests with "Clutch" in the name
```

You need CMake 3.16+ and a C++ compiler (Xcode command line tools on macOS,
Visual Studio Build Tools on Windows). CI runs the tests on every push.

Tests exist for ControlBoard, FSB_CODE, and Telemetry_Sender. The fakes also
cover I2C (`Wire`, with a TCA9548A-style mux and devices behind it), the
BMX160, the VL6180X, NeoPixels, CAN receive and filters, and QSPI flash.
`CANSAME5x.h` and `BMX160.h` have no include guard, like the real libraries,
so including one twice (for example `<CAN.h>` and `<CANSAME5x.h>`) fails in
the tests just as it does in the Arduino IDE.

## Compile check

Fakes only behave like the real libraries where someone made them. CI also
compiles each tested board with `arduino-cli`, the real Adafruit SAMD core,
and the real libraries (`.github/workflows/firmware-compile.yml`), so a sketch
that won't build in the Arduino IDE fails CI too. Library versions are pinned
there; update them in that file, and add a board to its list when you add
tests for it.

`tests/ControlBoard/control_board_test.cpp` is the example to copy. Tests
compare against pin numbers written out in the test, not the sketch's
constants, so a wrong pin edit in a sketch fails a test. If you change the
wiring on purpose, update the test in the same PR. To test another board, add
a `tests/<Board>/` folder and an `add_sketch_test()` line to
`tests/CMakeLists.txt`. A test can only call functions declared in the
board's `.h`, which the header rules above require anyway.

## The rules we follow

These are the sections of the Google guide called out in the standards doc.

### Headers

- Every sketch has a `.h` next to its `.ino` with a comment saying what the
  board does and a declaration plus comment for each function.
- Headers are self-contained: they have an include guard and include what
  their declarations need.

  ```cpp
  #ifndef FSB_CODE_FSB_CODE_H_
  #define FSB_CODE_FSB_CODE_H_

  #include <Arduino.h>
  ...
  #endif  // FSB_CODE_FSB_CODE_H_
  ```

- Include what you use: if a file uses `Wire`, it includes `<Wire.h>`, even if
  another header happens to pull it in.

### Scope, functions, and `const`

- Declare locals in the narrowest scope and initialize them where declared.
- Keep functions short. If `loop()` grows past a screen, split it into named
  steps (`UpdateShifting()`, `UpdateClutch()`, `SendClutchState()`).
- Prefer return values to output parameters. When you need outputs, put inputs
  first and pass outputs by reference.
- Use `++i`, not `i++`.
- Mark anything that doesn't change `const` (or `constexpr` for compile-time
  values), including pointer parameters you only read: `const int16_t* values`.

### Naming

| Kind | Style | Example |
| --- | --- | --- |
| Pins | `ALL_CAPS`, ending in `PIN` | `UP_IN_PIN`, `RFM95_CS_PIN` |
| Other constants, pin lists | `snake_case` | `can_baud_rate`, `mux_select_pins` |
| Functions | `PascalCase` | `Mux()`, `CanShortFrame()` |
| Variables, parameters | `snake_case` | `data_count`, `start_index` |
| Types | `PascalCase` | `ClutchPaddles` |
| Macros (avoid) | `ALL_CAPS` | `PRINT_DEBUG` |

Arduino's own `setup()` and `loop()` keep their names. Use `constexpr` instead
of `#define` for constants.

Team choice: Google names constants `kCamelCase`. We instead write pins in
`ALL_CAPS` ending in `PIN`, so they stand out and match the board schematics,
and other constants in `snake_case`, where the `constexpr` on the declaration
marks them. Pins are still `constexpr int`, not `#define`. Don't reuse a
constant's name for a local or parameter (e.g. a `can_id` parameter would hide
the `can_id` constant). Use fixed-width types (`int16_t`, `uint32_t`)
instead of `short`, `long`, and `unsigned long`.

### Comments

- Each file starts with a block comment header. Copy this and fill it in:

  ```cpp
  /**
   * @file ControlBoard.h
   * @brief Control Board firmware interface, WUFR26.
   *
   * One short paragraph on what the board does and how.
   *
   * Hardware: Adafruit Feather M4 CAN.
   */
  ```

  In the `.ino`, the `@brief` says "implementation", the paragraph covers how
  the code works (or is left out), and
  `@see ControlBoard.h for an overview of the board.` comes last. Use `@note`
  for known issues.
- Everywhere else, use `//` comments, including multi-line ones inside code.
  Each function declaration in the `.h` gets one saying what it does.
- Comments explain *why*; the code already says *what*.
- Delete commented-out code. Git keeps the history.
- TODOs name an owner: `// TODO(henry): Scale the paddle positions.`

### Formatting

Two-space indents, 80-column lines, braces on the same line. Don't do this by
hand: run `python scripts/format.py` or format on save.

## Arduino notes

`.ino` files are compiled as C++, not C. Our Feather M4 CAN boards use
`-std=gnu++11` with exceptions and RTTI turned off, so everything above
(`constexpr`, `static_cast`, references, range-based `for`) is supported.
A few Arduino-specific exceptions:

- `setup()` and `loop()` keep their Arduino names.
- Global hardware objects (`CANSAME5x CAN;`, `Servo clutch_servo;`) are the
  normal Arduino pattern and are fine.
- Use Arduino's `max()`/`min()` as-is; don't include `<algorithm>`. cpplint's
  standard-library header check is turned off for this reason.
- Some libraries (e.g. `BMX160.h`) have no include guard. Include those only
  once, from your guarded header.
- Don't use exceptions, `dynamic_cast`, or `typeid`; they are compiled out.
- Plain `.c` files are real C (`-std=gnu11`): use `#define` or `enum` instead
  of `constexpr`, C casts instead of `static_cast`, and pointers instead of
  references. Naming, comments, and formatting rules still apply.

## New season

Copy `.clang-format`, `.editorconfig`, `CPPLINT.cfg`, `CODE_STANDARDS.md`, and
`tests/` into the new season folder. The scripts pick up any folder with a
`CPPLINT.cfg` automatically.
