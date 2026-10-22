#!/usr/bin/env python3
"""Point the newest hyprpm commit pin at a commit that already exists."""

import re
import sys
from pathlib import Path

PIN = re.compile(r'(\["[0-9a-fA-F]{40}",\s*")([0-9a-fA-F]{40})("\])')
ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "hyprpm.toml"


def main() -> int:
    if len(sys.argv) != 2 or not re.fullmatch(r"[0-9a-fA-F]{40}", sys.argv[1]):
        print("usage: update-commit-pin.py <40-character commit>", file=sys.stderr)
        return 2

    sha = sys.argv[1].lower()
    text = MANIFEST.read_text()
    matches = list(PIN.finditer(text))
    if not matches:
        print("hyprpm.toml has no commit_pins entry", file=sys.stderr)
        return 1

    latest = matches[-1]
    if latest.group(2).lower() == sha:
        return 0

    updated = text[: latest.start(2)] + sha + text[latest.end(2) :]
    MANIFEST.write_text(updated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
