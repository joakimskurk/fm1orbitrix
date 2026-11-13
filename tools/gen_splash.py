#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""The splash's cat: a black cat as a planet in a violet coat, a yellow ring round it (the user's drawing), as pixel art
on a grid of cells, for firmware/src/orb_splash.h (palette indices, drawn 3x on the FM-1) and assets/cat.png.

  python3 tools/gen_splash.py

If assets/cat_source.png is there (the user's own picture), its cells are sampled and matched to the palette. If not,
the cat is drawn here from shapes measured off that picture (1254 px a side, a cell ~24 px). The drawing's stars are
left out: the splash twinkles its own around the cat.
"""
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets" / "cat_source.png"
OUT_H = ROOT / "firmware" / "src" / "orb_splash.h"
OUT_PNG = ROOT / "assets" / "cat.png"
N = 52                                          # cells a side over the 1254 px picture
U = 1254 / N

# the palette (index 0: nothing). Eyes are their own index so they can blink
PAL = [None, (7, 6, 24), (217, 194, 255), (124, 71, 240), (69, 18, 194), (9, 11, 34), (248, 242, 230),
       (200, 168, 255), (255, 242, 26), (248, 242, 230)]
BG, LILAC, PURPLE, DEEP, FACE, WHITE, WHISK, YELLOW, EYE = 1, 2, 3, 4, 5, 6, 7, 8, 9


def cell(px, py):
    """the cell a picture point is in"""
    return int(px / U), int(py / U)


def inside(poly, x, y):
    c = False
    n = len(poly)
    for i in range(n):
        x1, y1 = poly[i]
        x2, y2 = poly[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            c = not c
    return c


def draw():
    g = [[0] * N for _ in range(N)]
    cx = lambda i: (i + 0.5) * U                 # a cell's centre in the picture
    # the head and the body: the silhouette, stepped as the drawing is
    body = [(430, 190), (497, 190), (497, 228), (530, 228), (530, 265), (565, 265), (565, 340), (705, 340), (705, 305),
            (740, 305), (740, 265), (775, 265), (775, 228), (810, 228), (810, 188), (880, 188), (880, 388), (915, 388),
            (915, 462), (955, 462), (955, 690), (920, 690), (920, 745), (870, 745), (870, 880), (805, 880), (805, 915),
            (745, 915), (745, 950), (690, 950), (690, 985), (660, 985), (660, 1045), (612, 1045), (612, 985), (565, 985),
            (565, 950), (505, 950), (505, 915), (445, 915), (445, 880), (385, 880), (385, 745), (330, 745), (330, 690),
            (290, 690), (290, 470), (330, 470), (330, 390), (365, 390), (365, 310), (398, 310), (398, 230), (430, 230)]
    face = [(432, 268), (470, 268), (470, 300), (500, 300), (500, 340), (530, 340), (530, 398), (730, 398), (730, 340),
            (760, 340), (760, 300), (800, 300), (800, 262), (845, 262), (845, 400), (880, 400), (880, 600), (870, 600),
            (870, 690), (760, 690), (760, 715), (490, 715), (490, 690), (370, 690), (370, 470), (370, 330), (400, 330),
            (400, 300), (432, 300)]
    ring_c, ring_o, ring_i = (625, 700), (572, 178), (470, 100)
    ring_ci = (625, 676)

    def ring(px, py):
        ox = ((px - ring_c[0]) / ring_o[0]) ** 2 + ((py - ring_c[1]) / ring_o[1]) ** 2
        ix = ((px - ring_ci[0]) / ring_i[0]) ** 2 + ((py - ring_ci[1]) / ring_i[1]) ** 2
        return ox <= 1.0 and ix > 1.0

    for j in range(N):
        for i in range(N):
            px, py = cx(i), cx(j)
            if ring(px, py) and py < ring_c[1]:
                g[j][i] = YELLOW                # the ring behind the cat
            if inside(body, px, py):
                g[j][i] = PURPLE
    # the light from the upper left: a lilac rim where the cell up or left is outside
    rim = [[False] * N for _ in range(N)]
    for j in range(N):
        for i in range(N):
            if g[j][i] == PURPLE and cx(j) < 760:
                up = g[j - 1][i] if j else 0
                left = g[j][i - 1] if i else 0
                if up != PURPLE or left != PURPLE:
                    rim[j][i] = True
    for j in range(N):
        for i in range(N):
            if rim[j][i]:
                g[j][i] = LILAC
    for j in range(N):                          # the face, black
        for i in range(N):
            if inside(face, cx(i), cx(j)):
                g[j][i] = FACE
    # the chin: a lilac band under the face, the corners first
    for j in range(N):
        for i in range(N):
            px, py = cx(i), cx(j)
            if g[j][i] == PURPLE and 680 <= py < 770 and 380 <= px < 875:
                g[j][i] = LILAC
            if g[j][i] == FACE and py >= 680 and (px < 490 or px >= 760):
                g[j][i] = LILAC
    # the deep violet at the heart of the lower body
    for j in range(N):
        for i in range(N):
            px, py = cx(i), cx(j)
            if g[j][i] == PURPLE and py >= 860 and abs(px - 640) < (960 - py) * 0.7:
                g[j][i] = DEEP
    # eyes, nose, whiskers
    def rect(x0, y0, x1, y1, c):              # the cells whose centre is in it (at least one)
        hit = [(i, j) for j in range(N) for i in range(N) if x0 <= cx(i) < x1 and y0 <= cx(j) < y1]
        if not hit:
            hit = [cell((x0 + x1) / 2, (y0 + y1) / 2)]
        for i, j in hit:
            g[j][i] = c
    rect(482, 500, 530, 600, EYE)
    rect(722, 500, 770, 600, EYE)
    rect(588, 553, 662, 578, WHITE)
    rect(612, 578, 636, 603, WHITE)
    rect(375, 566, 447, 590, WHISK)
    rect(375, 614, 447, 638, WHISK)
    rect(802, 566, 874, 590, WHISK)
    rect(802, 614, 874, 638, WHISK)
    for j in range(N):                          # the ring in front, last
        for i in range(N):
            px, py = cx(i), cx(j)
            if ring(px, py) and py >= ring_c[1] - 20:
                g[j][i] = YELLOW
    return g


def sample(path):
    im = Image.open(path).convert("RGB").resize((1254, 1254), Image.NEAREST)
    g = [[0] * N for _ in range(N)]
    for j in range(N):
        for i in range(N):
            c = im.getpixel((int((i + 0.5) * U), int((j + 0.5) * U)))
            best = min(range(1, len(PAL)), key=lambda k: sum((a - b) ** 2 for a, b in zip(PAL[k], c)))
            g[j][i] = 0 if best == BG else best
    return g


def main():
    g = sample(SRC) if SRC.exists() else draw()
    for j in range(N):                          # the background is nothing
        for i in range(N):
            if g[j][i] == BG:
                g[j][i] = 0
    ys = [j for j in range(N) if any(g[j])]
    xs = [i for i in range(N) if any(g[j][i] for j in range(N))]
    y0, y1, x0, x1 = ys[0], ys[-1], xs[0], xs[-1]
    w, h = x1 - x0 + 1, y1 - y0 + 1
    crop = [row[x0:x1 + 1] for row in g[y0:y1 + 1]]
    img = Image.new("RGBA", (w * 8, h * 8), (0, 0, 0, 0))   # (transparent round her: any page's night shows through)
    for j in range(h):
        for i in range(w):
            if crop[j][i]:
                for yy in range(8):
                    for xx in range(8):
                        img.putpixel((i * 8 + xx, j * 8 + yy), PAL[crop[j][i]] + (255,))
    img.save(OUT_PNG)

    def rgb565(c):
        return ((c[0] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[2] >> 3)
    lines = ["/* SPDX-License-Identifier: GPL-3.0-only */", "/* Copyright (C) 2026 joakimskurk */",
             "/* Generated by tools/gen_splash.py: the splash's cat-planet as palette indices (0: nothing). Do not edit. */",
             "#ifndef ORB_SPLASH_H", "#define ORB_SPLASH_H", "#include <stdint.h>",
             f"#define CAT_W {w}u", f"#define CAT_H {h}u",
             "enum { CAT_NONE, CAT_BG, CAT_LILAC, CAT_PURPLE, CAT_DEEP, CAT_FACE, CAT_WHITE, CAT_WHISK, CAT_YELLOW, CAT_EYE };",
             "static const uint16_t CAT_PAL[%d] = {0, %s};" % (len(PAL), ", ".join(f"0x{rgb565(c):04X}u" for c in PAL[1:])),
             f"static const uint8_t CAT[CAT_H][CAT_W] = {{"]
    for row in crop:
        lines.append("    {" + ",".join(str(v) for v in row) + "},")
    lines += ["};", "#endif", ""]
    OUT_H.write_text("\n".join(lines))
    print(f"{OUT_H}: {w} x {h} cells ({'sampled from ' + SRC.name if SRC.exists() else 'drawn'}); {OUT_PNG}")


if __name__ == "__main__":
    main()
