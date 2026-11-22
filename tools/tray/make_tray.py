#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ujudhammo
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#
# DhammaKey - a DhammaWare project
# Homepage: https://dhammaware.org
#
# Copyright (C) 2026 Ujudhammo.
#
# TERMS OF USE AND NONCOMMERCIAL COMMITMENT:
# This software is offered as dana (a free gift) to serve the community, to
# support study and research, and to help spread the Dhamma. You may use, read,
# study, modify and contribute improvements to this source code for
# noncommercial purposes.
#
# PROHIBITED: copying, packaging, redistributing or using this source code or
# any derivative work for commercial purposes, for sale, or for any fee.
#
# The full, legally binding terms are in the LICENSE file at the project root.
# Third-party components keep their own licenses; see THIRD-PARTY-NOTICES.txt.
#
# THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. IN NO EVENT
# SHALL THE AUTHOR BE LIABLE FOR ANY DAMAGES ARISING FROM ITS USE.

"""Sinh bộ tray icon DhammaKey (chữ Latinh + chữ bản địa).

Chữ được chuyển thành nét vẽ (path) bằng fontTools, nên file SVG/PNG/ICO
không cần cài font. Muốn dùng font khác (vd. Noto Sans Myanmar) thì sửa
bảng FONTS rồi chạy lại:  python3 make_tray.py <thư-mục-ra>
Cần: fontTools, Pillow, rsvg-convert, ImageMagick (convert).
"""
import os, subprocess, sys
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.transformPen import TransformPen

OUT = sys.argv[1] if len(sys.argv) > 1 else "out"

F = "/usr/share/fonts/opentype"
FONTS = {
    "inter": (f"{F}/inter/Inter-Bold.otf", None),
    "deva":  ("/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf", None),
    "thai":  (f"{F}/tlwg/Loma-Bold.otf", None),
    "jp":    (f"{F}/noto/NotoSansCJK-Bold.ttc", 0),
    "kr":    (f"{F}/noto/NotoSansCJK-Bold.ttc", 1),
    "sc":    (f"{F}/noto/NotoSansCJK-Bold.ttc", 2),
    "mya":   (os.path.join(os.path.dirname(os.path.abspath(__file__)), "fonts", "NotoSansMyanmar-Bold.ttf"), None),
}

# Chữ Miến က: nay dùng Noto Sans Myanmar Bold (fonts/). Bản vẽ tay giữ lại làm dự phòng (font "draw").
MYANMAR_KA = ("M 9 52 L 9 33 C 9 4, 50 4, 50 33 L 50 52 "
              "M 50 33 C 50 4, 91 4, 91 33 L 91 40 C 91 56, 72 58, 68 47")

# mã: (Latinh, font Latinh, bản địa, font bản địa, tên)
MODES = [
    ("vi",       "V", "inter", "Vi", "inter", "Tiếng Việt"),
    ("pali",     "P", "inter", "Pā", "inter", "Pali"),
    ("sanskrit", "S", "inter", "अ",  "deva",  "Sanskrit"),
    ("myanmar",  "M", "inter", "က",  "mya",   "Miến Điện"),
    ("thai",     "T", "inter", "ก",  "thai",  "Thái"),
    ("han",      "H", "inter", "中", "sc",    "Hán / Trung"),
    ("korean",   "K", "inter", "한", "kr",    "Hàn"),
    ("japanese", "J", "inter", "あ", "jp",    "Nhật"),
    ("hannom",   "N", "inter", "喃", "sc",    "Hán Nôm"),
    ("off",      "E", "inter", "EN", "inter", "Tắt (gõ tiếng Anh)"),
    ("conflict", "V", "inter", "Vi", "inter", "Xung đột bộ gõ khác"),
]

MANGOSTEEN, RICE, PURPLE, RED = "#4B1E3F", "#F8F4EE", "#8C5A7C", "#B3261E"

_font_cache = {}
def font(key):
    if key not in _font_cache:
        path, idx = FONTS[key]
        _font_cache[key] = TTFont(path, fontNumber=idx or 0)
    return _font_cache[key]

def text_path(text, key):
    """Trả về (d, xmin, ymin, xmax, ymax) theo đơn vị font, trục y hướng xuống."""
    f = font(key); gs = f.getGlyphSet(); cmap = f.getBestCmap()
    pen = SVGPathPen(gs); bp = BoundsPen(gs); x = 0
    for ch in text:
        g = cmap[ord(ch)]
        t = (1, 0, 0, -1, x, 0)
        gs[g].draw(TransformPen(pen, t)); gs[g].draw(TransformPen(bp, t))
        x += gs[g].width
    return (pen.getCommands(),) + bp.bounds

def glyph_svg(text, key, fill, box_w, box_h, cx, cy, size):
    if key == "draw":  # chữ Miến vẽ tay: nét stroke, hộp 100x60
        s = min(box_w / 100, box_h / 60) * 1.0
        sw = 14 if size >= 24 else 17          # nét dày hơn ở cỡ nhỏ
        return (f'<path d="{MYANMAR_KA}" fill="none" stroke="{fill}" stroke-width="{sw}" '
                f'stroke-linecap="round" stroke-linejoin="round" '
                f'transform="translate({cx - 50*s:.3f},{cy - 30*s:.3f}) scale({s:.4f})"/>')
    d, x0, y0, x1, y1 = text_path(text, key)
    w, h = x1 - x0, y1 - y0
    s = min(box_w / w, box_h / h)
    tx, ty = cx - (x0 + w / 2) * s, cy - (y0 + h / 2) * s
    return f'<path d="{d}" fill="{fill}" transform="translate({tx:.3f},{ty:.3f}) scale({s:.5f})"/>'

def glyph_box(text, key):
    """Hộp chữ trong lưới 32 px: chữ đơn cao hơn, hai chữ rộng hơn.
    Bản 0.3.4: chữ to thêm khoảng 15% cho dễ nhìn ở khay hệ thống."""
    if key == "draw":           return 26, 17
    if key == "mya":            return 26, 18
    if len(text) >= 2:          return 27, 17
    if key in ("sc", "kr", "jp"): return 23, 23
    if key == "thai":           return 22, 21
    if key == "deva":           return 24, 22
    return 22, 22               # V P S M ...

def icon_svg(text, key, mode, theme, size):
    """theme: light | dark | template (đen-trong suốt cho macOS / Linux symbolic)."""
    off, conflict = mode == "off", mode == "conflict"
    if theme == "template":
        key_fill, ink, edge = "#000", "#000", "#000"
    elif theme == "dark":
        key_fill, ink, edge = RICE, MANGOSTEEN, RICE
    else:
        key_fill, ink, edge = MANGOSTEEN, RICE, MANGOSTEEN
    if conflict and theme != "template":
        key_fill, ink = RED, "#FFFFFF"
    bw, bh = glyph_box(text, key)
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 32 32">']
    if off:
        parts.append(f'<rect x="2" y="2" width="28" height="28" rx="6" fill="none" stroke="{edge}" stroke-width="2.6"/>')
        parts.append(glyph_svg(text, key, edge, bw - 4, bh - 3, 16, 16, size))
    elif theme == "template":
        # phím đặc, chữ khoét rỗng (mask) để macOS tự tô theo thanh menu
        parts.append('<defs><mask id="m"><rect width="32" height="32" fill="#fff"/>'
                     + glyph_svg(text, key, "#000", bw, bh, 16, 16, size) + '</mask></defs>')
        parts.append('<rect x="1" y="1" width="30" height="30" rx="6.5" fill="#000" mask="url(#m)"/>')
    else:
        parts.append(f'<rect x="1" y="1" width="30" height="30" rx="6.5" fill="{key_fill}"/>')
        parts.append(glyph_svg(text, key, ink, bw, bh, 16, 16, size))
    parts.append('</svg>')
    return "".join(parts)

def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh: fh.write(data)

def render(svg_path, png_path, px):
    os.makedirs(os.path.dirname(png_path), exist_ok=True)
    subprocess.run(["rsvg-convert", "-w", str(px), "-h", str(px), "-o", png_path, svg_path], check=True)

def main():
    for setname, ti, ki in (("latin", 1, 2), ("native", 3, 4)):
        for m in MODES:
            mode, text, key = m[0], m[ti], m[ki]
            base = f"dk-{mode}"
            # Windows .ico: 16/20/24/32, nền sáng và nền tối
            for theme in ("light", "dark"):
                pngs = []
                for px in (16, 20, 24, 32, 48, 64):
                    svg = f"{OUT}/src/{setname}/{base}-{theme}-{px}.svg"
                    write(svg, icon_svg(text, key, mode, theme, px))
                    png = f"{OUT}/png/{setname}/{base}-{theme}-{px}.png"
                    render(svg, png, px)
                    if px <= 32: pngs.append(png)
                ico = f"{OUT}/windows/{setname}/{base}-{theme}.ico"
                os.makedirs(os.path.dirname(ico), exist_ok=True)
                subprocess.run(["convert", *pngs, ico], check=True)
            # macOS: ảnh template 18 px và @2x
            svg = f"{OUT}/src/{setname}/{base}-template.svg"
            write(svg, icon_svg(text, key, mode, "template", 18))
            render(svg, f"{OUT}/macos/{setname}/{base}Template.png", 18)
            render(svg, f"{OUT}/macos/{setname}/{base}Template@2x.png", 36)
            # Linux: SVG màu + -symbolic
            write(f"{OUT}/linux/{setname}/{base}.svg", icon_svg(text, key, mode, "light", 32))
            write(f"{OUT}/linux/{setname}/{base}-symbolic.svg", icon_svg(text, key, mode, "template", 16))

if __name__ == "__main__":
    main()
