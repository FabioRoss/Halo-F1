#!/usr/bin/env python3
"""
Regenerate embedded LVGL Montserrat fonts used by Halo-F1.

This script generates:
  - montserrat_12.c
  - montserrat_14.c
  - montserrat_18.c
  - montserrat_20.c
  - montserrat_24.c
  - montserrat_38.c

with a Unicode coverage that supports all project languages:
English, Italian, Spanish, French, Dutch, German, Portuguese, Norwegian, Polish.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


DEFAULT_SIZES = [12, 14, 18, 20, 24, 38]

# Keep the existing icon coverage used by the project (FontAwesome glyphs).
ICON_RANGES = [
    61441, 61448, 61451, 61452, 61453, 61457, 61459, 61461, 61465, 61468,
    61473, 61478, 61479, 61480, 61502, 61512, 61515, 61516, 61517, 61521,
    61522, 61523, 61524, 61543, 61544, 61550, 61552, 61553, 61556, 61559,
    61560, 61561, 61563, 61587, 61589, 61636, 61637, 61639, 61671, 61674,
    61683, 61724, 61732, 61787, 61931, 62016, 62017, 62018, 62019, 62020,
    62189, 62212, 62810, 63426,
]

FULL_TEXT_RANGES = "32-126,160-383,8364"
RSS_EXTRA_CHARS = "“”„’‘–—…•«»‹›"


def parse_args() -> argparse.Namespace:
    default_montserrat = Path("assets/fonts/Montserrat-Regular.ttf")
    default_fontawesome = Path("assets/fonts/Font Awesome 7 Free-Solid-900.otf")
    parser = argparse.ArgumentParser(
        description="Generate LVGL Montserrat font sources with full language coverage."
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Path to the Halo-F1 project root (default: script parent project).",
    )
    parser.add_argument(
        "--montserrat-font",
        type=Path,
        default=default_montserrat,
        help="Path to Montserrat-Regular.ttf (default: assets/fonts/Montserrat-Regular.ttf)",
    )
    parser.add_argument(
        "--fontawesome-font",
        type=Path,
        default=default_fontawesome,
        help="Path to Font Awesome 7 Free-Solid-900.otf (default: assets/fonts/Font Awesome 7 Free-Solid-900.otf)",
    )
    parser.add_argument(
        "--sizes",
        type=int,
        nargs="+",
        default=DEFAULT_SIZES,
        help="Font sizes to generate (default: 12 14 18 20 24 38).",
    )
    parser.add_argument(
        "--font-conv-cmd",
        default="npx lv_font_conv",
        help='Command used to run lv_font_conv (default: "npx lv_font_conv").',
    )
    parser.add_argument(
        "--profile",
        choices=("lean", "full"),
        default="lean",
        help='Text coverage profile: "lean" (UI strings + common RSS punctuation) or "full" (32-126,160-383,8364).',
    )
    parser.add_argument(
        "--no-compress",
        action="store_true",
        help="Disable lv_font_conv compression (larger output).",
    )
    return parser.parse_args()


def _extract_c_string_literals(content: str) -> list[str]:
    matches = re.findall(r'"((?:\\.|[^"\\])*)"', content)
    result: list[str] = []
    for m in matches:
        s = m.replace(r"\\", "\\")
        s = s.replace(r"\"", "\"")
        s = s.replace(r"\n", "\n")
        s = s.replace(r"\r", "\r")
        s = s.replace(r"\t", "\t")
        result.append(s)
    return result


def _to_compact_ranges(codepoints: list[int]) -> str:
    if not codepoints:
        return ""
    ranges: list[str] = []
    start = prev = codepoints[0]
    for cp in codepoints[1:]:
        if cp == prev + 1:
            prev = cp
            continue
        ranges.append(f"{start}-{prev}" if start != prev else f"{start}")
        start = prev = cp
    ranges.append(f"{start}-{prev}" if start != prev else f"{start}")
    return ",".join(ranges)


def build_lean_text_ranges(project_root: Path) -> str:
    localized_strings = project_root / "localized_strings.h"
    if not localized_strings.exists():
        raise FileNotFoundError(f"Missing required file: {localized_strings}")

    content = localized_strings.read_text(encoding="utf-8")
    literals = _extract_c_string_literals(content)

    cps = set(range(32, 127))  # Always keep printable ASCII.
    cps.add(8364)              # Euro sign.

    for s in literals:
        for ch in s:
            cp = ord(ch)
            if cp >= 32:
                cps.add(cp)

    for ch in RSS_EXTRA_CHARS:
        cps.add(ord(ch))

    return _to_compact_ranges(sorted(cps))


def run_font_conv(
    font_conv_cmd: str,
    project_root: Path,
    montserrat_font: Path,
    fontawesome_font: Path,
    size: int,
    text_ranges: str,
    no_compress: bool,
) -> None:
    out_file = project_root / f"montserrat_{size}.c"
    icon_range_arg = ",".join(str(v) for v in ICON_RANGES)
    command = (
        f'{font_conv_cmd} '
        f'--bpp 4 --size {size} '
        f'{"--no-compress " if no_compress else ""}'
        f'--font "{montserrat_font}" --range {text_ranges} '
        f'--font "{fontawesome_font}" --range {icon_range_arg} '
        f'--format lvgl -o "{out_file}"'
    )

    print(f"[fonts] Generating {out_file.name} ...")
    subprocess.run(command, shell=True, check=True, cwd=str(project_root))


def print_missing_font_help(project_root: Path, missing_paths: list[Path]) -> None:
    print("[error] Missing required font files:", file=sys.stderr)
    for path in missing_paths:
        print(f"  - {path}", file=sys.stderr)

    print("\n[hint] Put the font files here:", file=sys.stderr)
    print(
        f"  - {project_root / 'assets/fonts/Montserrat-Regular.ttf'}",
        file=sys.stderr,
    )
    print(
        f"  - {project_root / 'assets/fonts/Font Awesome 7 Free-Solid-900.otf'}",
        file=sys.stderr,
    )

    print("\n[hint] PowerShell download commands:", file=sys.stderr)
    print(
        f'  New-Item -ItemType Directory -Path "{project_root / "assets/fonts"}" -Force | Out-Null',
        file=sys.stderr,
    )
    print(
        '  Invoke-WebRequest -Uri "https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Regular.ttf" '
        f'-OutFile "{project_root / "assets/fonts/Montserrat-Regular.ttf"}"',
        file=sys.stderr,
    )
    print(
        '  Invoke-WebRequest -Uri "https://raw.githubusercontent.com/FortAwesome/Font-Awesome/7.x/otfs/Font%20Awesome%207%20Free-Solid-900.otf" '
        f'-OutFile "{project_root / "assets/fonts/Font Awesome 7 Free-Solid-900.otf"}"',
        file=sys.stderr,
    )


def main() -> int:
    args = parse_args()
    project_root = args.project_root.resolve()
    montserrat_font = args.montserrat_font.resolve()
    fontawesome_font = args.fontawesome_font.resolve()

    if not project_root.exists():
        print(f"[error] Project root does not exist: {project_root}", file=sys.stderr)
        return 1

    missing_paths = []
    if not montserrat_font.exists():
        missing_paths.append(montserrat_font)
    if not fontawesome_font.exists():
        missing_paths.append(fontawesome_font)
    if missing_paths:
        print_missing_font_help(project_root, missing_paths)
        return 1

    if args.profile == "full":
        text_ranges = FULL_TEXT_RANGES
    else:
        text_ranges = build_lean_text_ranges(project_root)
    print(f"[fonts] Text range profile: {args.profile}")
    print(f"[fonts] Text ranges: {text_ranges}")

    for size in args.sizes:
        run_font_conv(
            font_conv_cmd=args.font_conv_cmd,
            project_root=project_root,
            montserrat_font=montserrat_font,
            fontawesome_font=fontawesome_font,
            size=size,
            text_ranges=text_ranges,
            no_compress=args.no_compress,
        )

    print("[fonts] Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

