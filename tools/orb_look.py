#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""Look at a render: a spectrogram (log frequency), the waveform envelope, and numbers (peak, RMS, DC, the
spectral centroid, the largest jump between frames: a click, the share of energy above 16 kHz).

  python3 tools/orb_look.py build/audio/x.wav [more.wav ...]     -> build/look/x.png, and a line each
"""
import sys
import wave
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def load(p):
    with wave.open(str(p)) as w:
        n, ch, fs = w.getnframes(), w.getnchannels(), w.getframerate()
        a = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float64) / 32768.0
    return a.reshape(-1, ch), fs


def colour(v):                                  # 0..1 -> indigo .. violet .. magenta .. lime
    stops = [(0.0, (8, 4, 24)), (0.35, (70, 30, 170)), (0.6, (230, 60, 170)), (0.8, (255, 210, 40)), (1.0, (200, 255, 40))]
    v = float(np.clip(v, 0, 1))
    for (a, ca), (b, cb) in zip(stops, stops[1:]):
        if v <= b:
            t = (v - a) / (b - a)
            return tuple(int(ca[i] + (cb[i] - ca[i]) * t) for i in range(3))
    return stops[-1][1]


def look(path, out_dir):
    x, fs = load(path)
    mono = x.mean(axis=1)
    n = 4096
    hop = max(256, len(mono) // 900)
    win = np.hanning(n)
    frames = []
    for i in range(0, max(1, len(mono) - n), hop):
        frames.append(np.abs(np.fft.rfft(mono[i:i + n] * win)))
    S = np.array(frames).T + 1e-9
    freqs = np.fft.rfftfreq(n, 1 / fs)
    H, W = 300, S.shape[1]
    img = Image.new("RGB", (W, H + 80), (8, 4, 24))
    px = img.load()
    lo, hi = np.log(30), np.log(20000)
    rows = np.exp(lo + (hi - lo) * (1 - np.arange(H) / (H - 1)))
    idx = np.clip(np.searchsorted(freqs, rows), 1, len(freqs) - 1)
    db = 20 * np.log10(S / S.max())
    for c in range(W):
        col = db[idx, c]
        for r in range(H):
            px[c, r] = colour((col[r] + 90) / 90)
    d = ImageDraw.Draw(img)
    for f in (100, 1000, 10000):
        y = int((1 - (np.log(f) - lo) / (hi - lo)) * (H - 1))
        d.line([(0, y), (12, y)], fill=(200, 255, 40))
        d.text((14, y - 6), f"{f}", fill=(200, 255, 40))
    env = [np.abs(mono[i:i + hop]).max() if i < len(mono) else 0 for i in range(0, W * hop, hop)]
    for c, v in enumerate(env[:W]):
        h = int(v * 70)
        d.line([(c, H + 75 - h), (c, H + 75)], fill=(38, 230, 255))
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / (Path(path).stem + ".png")
    img.save(out)
    pk = np.abs(x).max()
    rms = np.sqrt((x * x).mean())
    spec = (np.abs(np.fft.rfft(mono)) ** 2)
    f = np.fft.rfftfreq(len(mono), 1 / fs)
    cen = (spec * f).sum() / spec.sum()
    hf = spec[f > 16000].sum() / spec.sum()
    jump = np.abs(np.diff(x, axis=0)).max()
    print(f"{Path(path).name:34s} peak {20*np.log10(pk+1e-9):6.1f}  rms {20*np.log10(rms+1e-9):6.1f}  dc {x.mean():+.4f}"
          f"  centroid {cen:6.0f} Hz  >16k {10*np.log10(hf+1e-12):6.1f} dB  max step {jump:.3f}  -> {out}")


if __name__ == "__main__":
    for p in sys.argv[1:]:
        look(p, Path("build/look"))
