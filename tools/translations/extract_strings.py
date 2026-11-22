#!/usr/bin/env python3
"""Writes translations/en.json from the TRANS() and NEEDS_TRANS() strings in src/.

en.json is the source file Weblate translates from. Run this after adding or
changing a UI string: python tools/translations/extract_strings.py
Pass --check to only report whether en.json is up to date.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE_FILE = ROOT / "translations" / "en.json"
CALL = re.compile(r'\b(?:NEEDS_)?TRANS\(\s*"((?:[^"\\]|\\.)*)"\s*\)')
ESCAPES = {'"': '"', "'": "'", "\\": "\\", "n": "\n", "t": "\t", "r": "\r"}


def unescape(literal):
    return re.sub(r"\\(.)", lambda match: ESCAPES[match.group(1)], literal)


def find_strings():
    strings = set()
    for path in sorted((ROOT / "src").glob("*.cpp")) + sorted((ROOT / "include").glob("*.hpp")):
        for match in CALL.finditer(path.read_text(encoding="utf-8")):
            strings.add(unescape(match.group(1)))
    return strings


def main():
    strings = find_strings()
    text = json.dumps({string: string for string in sorted(strings)}, ensure_ascii=False, indent=4) + "\n"

    if "--check" in sys.argv:
        current = SOURCE_FILE.read_text(encoding="utf-8") if SOURCE_FILE.exists() else ""
        if current != text:
            print(f"{SOURCE_FILE.relative_to(ROOT)} is out of date, run {pathlib.Path(__file__).relative_to(ROOT)}")
            return 1
        return 0

    SOURCE_FILE.parent.mkdir(exist_ok=True)
    SOURCE_FILE.write_text(text, encoding="utf-8", newline="\n")
    print(f"Wrote {len(strings)} strings to {SOURCE_FILE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
