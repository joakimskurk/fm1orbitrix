#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""ORBITRIX's header: Astralica on her night, her four kittens on their rings, the title in its lime pill (with ears),
in the firmware's own fonts (firmware/gen/felucca_font.h) and colours, drawn in pixels at half size and doubled.

  python3 tools/make_header.py               -> docs/readme/header.png (1280 x 640: the README's top, and the
                                                repository's social preview: Settings, General, Social preview)
  python3 tools/make_header.py OUT W H [VER]  another size (web/make_site.py: the page's link preview, 1200 x 630)
"""
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BG, INK, FRAME, FDIM = (10, 6, 26), (16, 8, 34), (112, 66, 232), (52, 32, 116)
LIME, YEL, CYAN, MAG, VIO = (198, 255, 26), (255, 232, 26), (38, 224, 255), (255, 79, 168), (164, 116, 255)
TXT, GREY, DIMT = (240, 232, 255), (128, 108, 184), (76, 60, 120)


def font(name):
    """a font of the firmware's: {char: (advance, rows of 4-bit alpha)}, its height and its left pad"""
    src = (ROOT / "firmware" / "gen" / "felucca_font.h").read_text()

    def arr(n):
        body = re.search(r"%s\[\d+\] = \{(.*?)\};" % n, src, re.S).group(1)
        return [int(v, 0) for v in body.replace("\n", " ").split(",") if v.strip()]
    h, pad, first, last = (int(v) for v in re.search(r"felucca_font_t %s = \{ (\d+), (\d+), (\d+), (\d+)," % name, src).groups())
    data, off, adv, bw = arr(name + "_DATA"), arr(name + "_OFF"), arr(name + "_ADV"), arr(name + "_BW")
    glyphs = {}
    for i in range(last - first + 1):
        w, bpr, o = bw[i], (bw[i] + 1) // 2, off[i]
        rows = []
        for y in range(h):
            rows.append([(data[o + y * bpr + x // 2] >> (0 if x & 1 else 4)) & 15 for x in range(w)])
        glyphs[chr(first + i)] = (adv[i], rows)
    return glyphs, h, pad


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = np.zeros((h, w, 3), dtype=np.float64)

    def pset(self, x, y, c, a=1.0):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y, x] = self.px[y, x] * (1 - a) + np.array(c) * a

    def rect(self, x, y, w, h, c, a=1.0):
        for yy in range(int(y), int(y + h)):
            for xx in range(int(x), int(x + w)):
                self.pset(xx, yy, c, a)

    def text(self, x, y, f, s, c):
        glyphs, h, pad = f
        for ch in s:
            adv, rows = glyphs.get(ch, glyphs[" "])
            for gy, row in enumerate(rows):
                for gx, v in enumerate(row):
                    if v:
                        self.pset(x - pad + gx, y + gy, c, v / 15.0)
            x += adv
        return x

    def text_w(self, f, s):
        return sum(f[0].get(ch, f[0][" "])[0] for ch in s)


def pill(cv, x, y, w, h, c, cut=6):
    """a pill with its corners cut, as the screen's title"""
    for yy in range(h):
        d = max(0, cut - yy, cut - (h - 1 - yy))
        cv.rect(x + d, y + yy, w - 2 * d, 1, c)


def tri(cv, x0, x1, y_base, apex_x, apex_y, c):
    """a filled triangle: its base from x0 to x1 on y_base, its point at (apex_x, apex_y), above"""
    for y in range(apex_y, y_base + 1):
        f = (y - apex_y) / max(1, y_base - apex_y)
        a, b = apex_x + (x0 - apex_x) * f, apex_x + (x1 - apex_x) * f
        for x in range(int(math.floor(a)), int(math.ceil(b)) + 1):
            cv.pset(x, y, c)


def ears(cv, x, y, c, gap):
    """two cat ears standing on a pill's top edge (y), the left one's foot at x, the other gap further"""
    for k in (0, gap):
        tri(cv, x + k, x + k + 12, y, x + k + (3 if k == 0 else 9), y - 10, c)


def kitten(cv, x, y, c, r=6):
    """a kitten planet: a round body, two ears, two eyes"""
    x, y = int(round(x)), int(round(y))
    for yy in range(-r, r + 1):
        for xx in range(-r, r + 1):
            if xx * xx + yy * yy <= r * r + 1:
                cv.pset(x + xx, y + yy, c)
    tri(cv, x - r, x - 1, y - r + 2, x - r, y - r - 3, c)
    tri(cv, x + 1, x + r, y - r + 2, x + r, y - r - 3, c)
    cv.pset(x - 2, y - 1, INK)
    cv.pset(x + 2, y - 1, INK)
    cv.pset(x - 2, y, INK)
    cv.pset(x + 2, y, INK)


def plus_star(cv, x, y, c, arm):
    cv.rect(x - arm, y, 2 * arm + 1, 1, c, 0.55)
    cv.rect(x, y - arm, 1, 2 * arm + 1, c, 0.55)
    cv.pset(x, y, (255, 255, 255))


def draw(W, H, ver=""):
    s = 2                                    # (pixels: drawn at half size, doubled)
    w, h = W // s, H // s
    cv = Canvas(w, h)
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy = w * 0.30, h * 0.53
    g = np.exp(-(((xx - cx) / (w * 0.30)) ** 2 + ((yy - cy) / (h * 0.42)) ** 2))   # (her glow)
    for k in range(3):
        cv.px[..., k] = BG[k] + (np.array((44, 22, 110))[k] - BG[k]) * g * 0.55 + (yy / h) * (20 - BG[k] * 0.2) * 0.4
    rng = np.random.default_rng(37)
    pal = [TXT, VIO, LIME, MAG, CYAN, YEL]
    for _ in range(int(w * h / 300)):
        x, y = int(rng.integers(0, w)), int(rng.integers(0, h))
        c = pal[int(rng.integers(0, len(pal)))] if rng.random() < 0.35 else TXT
        cv.pset(x, y, c, float(rng.uniform(0.25, 0.9)))
    for x, y, arm, c in ((0.08, 0.16, 3, TXT), (0.52, 0.12, 2, VIO), (0.95, 0.80, 3, TXT), (0.60, 0.88, 2, MAG),
                         (0.90, 0.14, 2, CYAN), (0.05, 0.86, 2, VIO), (0.47, 0.50, 2, TXT)):
        plus_star(cv, int(x * w), int(y * h), c, arm)
    # her four rings (A..D: cyan, magenta, yellow, violet), dotted, tilted a little; a kitten on each
    rings = [(CYAN, 96, 30, 0.35), (MAG, 120, 40, 2.1), (YEL, 144, 50, 3.9), (VIO, 168, 60, 5.3)]
    tilt = -0.10
    for c, a, b, ph in rings:
        n = int(a * 0.9)
        for k in range(n):
            t = 2 * math.pi * k / n
            x, y = a * math.cos(t), b * math.sin(t)
            cv.pset(cx + x * math.cos(tilt) - y * math.sin(tilt), cy + x * math.sin(tilt) + y * math.cos(tilt), c, 0.85)
    cat = Image.open(ROOT / "assets" / "cat.png").convert("RGBA")
    cat = np.asarray(cat.resize((cat.width // 2, cat.height // 2), Image.NEAREST)).astype(np.float64)
    ox, oy = int(cx - cat.shape[1] / 2), int(cy - cat.shape[0] / 2 - 6)
    for y in range(cat.shape[0]):
        for x in range(cat.shape[1]):
            al = cat[y, x, 3] / 255.0
            if al > 0:
                cv.pset(ox + x, oy + y, tuple(cat[y, x, :3]), al)
    for c, a, b, ph in rings:                # (the kittens in front of her)
        x, y = a * math.cos(ph), b * math.sin(ph)
        kitten(cv, cx + x * math.cos(tilt) - y * math.sin(tilt), cy + x * math.sin(tilt) + y * math.cos(tilt), c)
    # the frame: violet, its corners cut
    m, cut = 8, 10
    for k in range(m + cut, w - m - cut):
        cv.pset(k, m, FRAME)
        cv.pset(k, h - 1 - m, FRAME)
    for k in range(m + cut, h - m - cut):
        cv.pset(m, k, FRAME)
        cv.pset(w - 1 - m, k, FRAME)
    for k in range(cut):
        for px, py in ((m + cut - k, m + k), (w - 1 - m - cut + k, m + k), (m + cut - k, h - 1 - m - k),
                       (w - 1 - m - cut + k, h - 1 - m - k)):
            cv.pset(px, py, FRAME)
    # the title: ORBITRIX in its lime pill, with ears; what it is under it
    fl, fs = font("FONT_L"), font("FONT_S")
    tx0 = int(w * 0.585)
    tw = cv.text_w(fl, "ORBITRIX")
    py = int(h * 0.30)
    pill(cv, tx0, py, tw + 20, 40, LIME)
    ears(cv, tx0 + 10, py, LIME, tw - 10)
    cv.text(tx0 + 10, py + 4, fl, "ORBITRIX", INK)
    y = py + 54
    cv.text(tx0, y, fs, "An orbital groove engine", TXT)
    cv.text(tx0, y + 16, fs, "for the M-VAVE FM-1", GREY)
    y += 44
    cv.text(tx0, y, fs, "Every rhythm an interval,", VIO)
    cv.text(tx0, y + 16, fs, "every interval a rhythm.", VIO)
    y += 44
    x = tx0
    for c in (CYAN, MAG, YEL, VIO, LIME):    # the rings' colours, a dot each, and what it is
        cv.rect(x, y + 5, 6, 6, c)
        x += 10
    cv.text(x + 4, y, fs, "astral cuteness", MAG)
    if ver:
        cv.text(w - m - 8 - cv.text_w(fs, ver), h - m - 22, fs, ver, DIMT)
    img = Image.fromarray(np.clip(cv.px, 0, 255).astype(np.uint8), "RGB")
    return img.resize((W, H), Image.NEAREST)


def main():
    if len(sys.argv) >= 4:
        draw(int(sys.argv[2]), int(sys.argv[3]), sys.argv[4] if len(sys.argv) > 4 else "").save(sys.argv[1])
        print(f"header -> {sys.argv[1]}")
        return
    out = ROOT / "docs" / "readme" / "header.png"
    draw(1280, 640).save(out, optimize=True)
    print(f"header -> {out} ({out.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
