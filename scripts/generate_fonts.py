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
    62087, 62099, 62189, 62212, 62810, 63426, 63650,
]

# Unicode coverage:
# - Basic printable ASCII
# - Latin-1 Supplement (accents like ä, ö, ü, é, ç, ñ, etc.)
# - Latin Extended-A (Polish and other European diacritics)
# - Euro sign
TEXT_RANGES = "32-126,160-383,8364"


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
    return parser.parse_args()


def run_font_conv(
    font_conv_cmd: str,
    project_root: Path,
    montserrat_font: Path,
    fontawesome_font: Path,
    size: int,
) -> None:
    out_file = project_root / f"montserrat_{size}.c"
    icon_range_arg = ",".join(str(v) for v in ICON_RANGES)

    command = (
        f'{font_conv_cmd} '
        f'--bpp 4 --size {size} --no-compress --stride 1 --align 1 '
        f'--font "{montserrat_font}" --range {TEXT_RANGES} '
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

    for size in args.sizes:
        run_font_conv(
            font_conv_cmd=args.font_conv_cmd,
            project_root=project_root,
            montserrat_font=montserrat_font,
            fontawesome_font=fontawesome_font,
            size=size,
        )

    print("[fonts] Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

