#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""The legend of Astralica for the page: the firmware's own legend (orbitrix.c, ORB_LEGEND: the host only), filmed on
the host at 25 frames a second, as an animation with a transparent background (the night is the page's), and its
sound beside it.

  python3 tools/make_legend.py        -> build/film/legend.webp (animated, lossless, looping), legend.m4a (its sound),
                                         legend.png (a still: the plop)

Needs a C compiler, Pillow (with WebP) and numpy; ffmpeg for the sound.
"""
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
FILM = ROOT / "build" / "film"
END_MS = 17200                                  # I_END in orbitrix.c: the legend, without the splash after it
FPS = 25
STILL_MS = 11550                                # the yellow plop


def main():
    FILM.mkdir(parents=True, exist_ok=True)
    (ROOT / "build" / "host").mkdir(parents=True, exist_ok=True)
    for f in FILM.glob("l-*.ppm"):
        f.unlink()
    exe = ROOT / "build" / "host" / "orbitrix-legend"
    subprocess.run(["cc", "-O2", "-Wall", "-Wno-unused-function", "-DDINGHY_HOST", '-DAPP_VER="legend"',
                    "-Ifirmware/hal", "-Ifirmware/src", "-Ifirmware/gen", "-o", str(exe), "firmware/src/orbitrix.c"],
                   cwd=ROOT, check=True)
    script = FILM / "legend-script.txt"
    script.write_text(f"# the legend alone (tools/make_legend.py)\n{END_MS} end\n")
    subprocess.run([str(exe), str(FILM / "l"), str(script), str(FPS)], cwd=ROOT, check=True,
                   env=dict(os.environ, ORB_INTRO="1"), stdout=subprocess.DEVNULL)
    frames = sorted(FILM.glob("l-*.ppm"))[: END_MS * FPS // 1000]
    if not frames:
        sys.exit("no frames: the host wrote none")
    first = np.asarray(Image.open(frames[0]).convert("RGB")).reshape(-1, 3)
    cols, counts = np.unique(first, axis=0, return_counts=True)
    bg = cols[counts.argmax()]                  # the night: what is everywhere at the start
    out = []
    for f in frames:
        a = np.asarray(Image.open(f).convert("RGB"))
        alpha = np.where(np.all(a == bg, axis=2), 0, 255).astype(np.uint8)
        out.append(Image.fromarray(np.dstack([a, alpha]), "RGBA"))
    webp = FILM / "legend.webp"
    try:
        out[0].save(webp, save_all=True, append_images=out[1:], duration=1000 // FPS, loop=0, lossless=True, method=6)
    except (OSError, ValueError, KeyError):     # (a Pillow without animated WebP: ffmpeg's libwebp, from PNGs)
        for i, im in enumerate(out):
            im.save(FILM / f"l-{i:05d}.png")
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", str(FPS), "-i", str(FILM / "l-%05d.png"),
                        "-c:v", "libwebp_anim", "-lossless", "1", "-loop", "0", str(webp)], check=True)
        for f in FILM.glob("l-*.png"):
            f.unlink()
    out[STILL_MS * FPS // 1000].save(FILM / "legend.png")
    for f in frames:
        f.unlink()
    for f in FILM.glob("l-*.ppm"):
        f.unlink()
    m4a = FILM / "legend.m4a"
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(FILM / "l.wav"), "-t", f"{END_MS / 1000:.2f}",
                    "-af", f"afade=t=out:st={END_MS / 1000 - 0.4:.2f}:d=0.4", "-c:a", "aac", "-b:a", "160k", str(m4a)],
                   check=True)
    print(f"legend -> {webp} ({len(out)} frames, {webp.stat().st_size // 1024} KB, the night transparent), "
          f"{m4a.name} ({m4a.stat().st_size // 1024} KB), legend.png")


if __name__ == "__main__":
    main()
