#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFilter
except Exception as exc:  # pragma: no cover
    raise SystemExit("Pillow is required. Install with: pip install Pillow") from exc


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets" / "icons"
WIN_DIR = ASSETS / "windows"
LINUX_DIR = ASSETS / "linux"
MAC_ICONSET = ASSETS / "macos.iconset"

SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]
MAC_SIZES = [16, 32, 64, 128, 256, 512, 1024]


def ensure_dirs() -> None:
    for d in [WIN_DIR, LINUX_DIR, MAC_ICONSET]:
        d.mkdir(parents=True, exist_ok=True)


def _palette(dark: bool) -> tuple[tuple[int, int, int, int], ...]:
    if dark:
        return (
            (24, 36, 58, 255),   # ring
            (13, 20, 36, 255),   # core
            (18, 133, 255, 255), # arc
            (156, 211, 255, 255) # dot
        )
    return (
        (237, 244, 255, 255),
        (16, 28, 48, 255),
        (0, 122, 255, 255),
        (96, 182, 255, 255),
    )


def icon_image(size: int, dark: bool) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    ring, core, arc, dot = _palette(dark)

    ring_outer = int(size * 0.92)
    ring_inner = int(size * 0.44)
    ox = (size - ring_outer) // 2
    oy = (size - ring_outer) // 2
    ix = (size - ring_inner) // 2
    iy = (size - ring_inner) // 2

    d.ellipse([ox, oy, ox + ring_outer, oy + ring_outer], fill=ring)
    d.ellipse([ix, iy, ix + ring_inner, iy + ring_inner], fill=core)

    stroke = max(1, size // 12)
    arc_box = [
        ox + stroke,
        oy + stroke,
        ox + ring_outer - stroke,
        oy + ring_outer - stroke,
    ]
    d.arc(arc_box, start=210, end=340, fill=arc, width=stroke)

    dot_r = max(2, size // 11)
    dot_x = int(size * 0.73)
    dot_y = int(size * 0.30)
    d.ellipse([dot_x - dot_r, dot_y - dot_r, dot_x + dot_r, dot_y + dot_r], fill=dot)

    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    gd.ellipse([dot_x - dot_r * 2, dot_y - dot_r * 2, dot_x + dot_r * 2, dot_y + dot_r * 2], fill=(80, 170, 255, 60))
    glow = glow.filter(ImageFilter.GaussianBlur(max(1, size // 32)))
    img.alpha_composite(glow)
    return img


def icon_image_small(size: int, dark: bool) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    ring, core, arc, dot = _palette(dark)

    pad = max(1, size // 12)
    d.ellipse([pad, pad, size - pad - 1, size - pad - 1], fill=ring)
    inner = max(2, size // 4)
    d.ellipse([inner, inner, size - inner - 1, size - inner - 1], fill=core)

    stroke = max(1, size // 7)
    d.arc([pad + 1, pad + 1, size - pad - 2, size - pad - 2], start=220, end=338, fill=arc, width=stroke)

    dot_r = max(1, size // 8)
    x = int(size * 0.70)
    y = int(size * 0.30)
    d.ellipse([x - dot_r, y - dot_r, x + dot_r, y + dot_r], fill=dot)
    return img


def icon_for_size(size: int, dark: bool) -> Image.Image:
    if size <= 32:
        return icon_image_small(size, dark)
    return icon_image(size, dark)


def save_windows_assets() -> None:
    frames = [icon_for_size(s, dark=True) for s in SIZES]
    ico_path = WIN_DIR / "clipwheel.ico"
    frames[-1].save(ico_path, format="ICO", sizes=[(s, s) for s in SIZES], append_images=frames[:-1])

    for s in SIZES:
        icon_for_size(s, dark=True).save(WIN_DIR / f"clipwheel-{s}.png", "PNG")
        icon_for_size(s, dark=False).save(WIN_DIR / f"clipwheel-light-{s}.png", "PNG")

    icon_for_size(256, dark=True).save(ROOT / "clipwheel.png", "PNG")
    frames[-1].save(ROOT / "clipwheel.ico", format="ICO", sizes=[(s, s) for s in SIZES], append_images=frames[:-1])


def save_linux_assets() -> None:
    for s in [32, 48, 64, 128, 256, 512]:
        icon_for_size(s, dark=True).save(LINUX_DIR / f"clipwheel-{s}.png", "PNG")
        icon_for_size(s, dark=False).save(LINUX_DIR / f"clipwheel-light-{s}.png", "PNG")

    svg = """<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 256 256'>
<circle cx='128' cy='128' r='118' fill='#18243a'/>
<circle cx='128' cy='128' r='56' fill='#0d1424'/>
<path d='M45 165 A95 95 0 0 1 224 90' fill='none' stroke='#1285ff' stroke-width='18' stroke-linecap='round'/>
<circle cx='188' cy='78' r='12' fill='#9cd3ff'/>
</svg>"""
    (LINUX_DIR / "clipwheel.svg").write_text(svg, encoding="utf-8")

    desktop = """[Desktop Entry]
Type=Application
Name=ClipWheel
Comment=Radial clipboard manager
Exec=clipwheel
Icon=clipwheel
Terminal=false
Categories=Utility;Productivity;
"""
    (LINUX_DIR / "clipwheel.desktop").write_text(desktop, encoding="utf-8")


def save_macos_assets() -> None:
    for s in MAC_SIZES:
        base = icon_for_size(s, dark=True)
        name = f"icon_{s}x{s}.png"
        base.save(MAC_ICONSET / name, "PNG")
        if s <= 512:
            base.resize((s * 2, s * 2), Image.LANCZOS).save(MAC_ICONSET / f"icon_{s}x{s}@2x.png", "PNG")

    icns_path = ASSETS / "clipwheel.icns"
    try:
        icon_for_size(1024, dark=True).save(icns_path, format="ICNS")
    except Exception:
        pass

    script = """#!/usr/bin/env bash
set -euo pipefail
iconutil -c icns macos.iconset -o clipwheel.icns
echo "Generated clipwheel.icns"
"""
    script_path = ASSETS / "build-macos-icns.sh"
    script_path.write_text(script, encoding="utf-8")


def save_packaging_files() -> None:
    pkg = ROOT / "packaging" / "windows"
    pkg.mkdir(parents=True, exist_ok=True)
    iss = r"""; Inno Setup script for ClipWheel
[Setup]
AppName=ClipWheel
AppVersion=2.1.0
DefaultDirName={autopf}\ClipWheel
DefaultGroupName=ClipWheel
OutputBaseFilename=ClipWheel-Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=..\..\assets\icons\windows\clipwheel.ico

[Files]
Source: "..\..\clipwheel.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\clipwheel.ini"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\assets\icons\windows\clipwheel.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\ClipWheel"; Filename: "{app}\clipwheel.exe"; IconFilename: "{app}\clipwheel.ico"
Name: "{commondesktop}\ClipWheel"; Filename: "{app}\clipwheel.exe"; IconFilename: "{app}\clipwheel.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional icons:"

[Registry]
Root: HKCR; Subkey: ".cwh"; ValueType: string; ValueName: ""; ValueData: "ClipWheel.Snippet"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "ClipWheel.Snippet\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\clipwheel.ico,0"
"""
    (pkg / "clipwheel.iss").write_text(iss, encoding="utf-8")


def main() -> None:
    ensure_dirs()
    save_windows_assets()
    save_linux_assets()
    save_macos_assets()
    save_packaging_files()
    print("Brand assets generated in assets/icons and packaging/windows.")


if __name__ == "__main__":
    main()
