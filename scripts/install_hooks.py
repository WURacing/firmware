"""Installs a git pre-commit hook that runs format.py and lint.py.

The hook blocks commits whose staged firmware files aren't formatted or fail
cpplint. Run once per clone:
  python scripts/install_hooks.py
Skip the check for a single commit with `git commit --no-verify`.
"""

import subprocess
import sys

import firmware_tools

# Git runs hooks with sh, including on Windows (Git for Windows bundles it).
# The Python name differs by OS, and on Windows `python3` may be a Microsoft
# Store stub that fails, so try each name until one runs.
_HOOK = """\
#!/bin/sh
# Installed by scripts/install_hooks.py.
cd "$(git rev-parse --show-toplevel)" || exit 1
for py in python3 python py; do
  if "$py" -c "" > /dev/null 2>&1; then
    "$py" scripts/format.py --check --staged && \\
      "$py" scripts/lint.py --staged
    exit $?
  fi
done
echo "pre-commit: Python not found; skipping style checks." >&2
"""


def main() -> int:
    hooks_dir = subprocess.run(
        ["git", "rev-parse", "--git-path", "hooks"],
        cwd=firmware_tools.REPO_ROOT, capture_output=True, text=True,
        check=True).stdout.strip()
    hook = firmware_tools.REPO_ROOT / hooks_dir / "pre-commit"
    if hook.exists():
        print(f"A pre-commit hook already exists at {hook}; not overwriting "
              "it.", file=sys.stderr)
        return 1

    hook.parent.mkdir(parents=True, exist_ok=True)
    # newline="\n" keeps sh happy on Windows, where text files default to CRLF.
    with open(hook, "w", newline="\n") as f:
        f.write(_HOOK)
    hook.chmod(0o755)
    print(f"Installed pre-commit hook at {hook}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
