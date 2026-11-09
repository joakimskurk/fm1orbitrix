#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""ORBITRIX's site (GitHub Pages, https://joakimskurk.github.io/fm1orbitrix/): its page, ORBITRIX to play in the
browser, the legend, the systems to hear, the installer.

  index.html                  web/index.html with fm1pkg.js and fm1ota.js (SLOOP's, unchanged) and the metadata
                              inlined: the installer (Web MIDI, Chrome or Edge), the demos
  app/                        ORBITRIX to play: orbitrix.wasm (web/app/build.sh: the firmware's own code on
                              web/app/web_host.c), worklet.js (the audio thread), play.js (the FM-1 drawn)
  firmware/orbitrix-VER.fwsc  the package the page installs (its SHA-256 in the page: a damaged download is never
                              offered)
  film/legend.webp, .m4a      the legend of Astralica: an animation on a transparent night, and its sound
                              (tools/make_legend.py: the firmware's legend, filmed on the host)
  audio/                      the twenty systems (tests/run_orb.sh's tour: the engine on the host), AAC, and each
                              one's spectrum as a picture (WebP), low to high; audio/songs/ the little songs
                              (orb_render song: the systems with chords, arranged)
  screens/                    the cat, the icon, the link preview, the pages' screens, the legend's poster
  .nojekyll                   (served as it is)

  sh tests/run_orb.sh          (first: the tour)
  python3 tools/make_legend.py (the legend)
  sh web/app/build.sh          (ORBITRIX for the browser)
  python3 web/make_site.py build/orbitrix.fwsc VERSION OUT_DIR

The package must be ORBITRIX's (its identity FM-1_967..) with its update loader in it (which the installer talks to).
The audio is encoded by ffmpeg. After RoboSeq's and Star Distroyer's web/make_site.py, after SLOOP's (Copyright (C)
2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments; isod89).
"""
import hashlib
import json
import re
import shutil
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
APP = {"build/web/orbitrix.wasm": "orbitrix.wasm", "web/app/worklet.js": "worklet.js", "web/app/play.js": "play.js"}
SCREENS = {"orbit": "screen_orbit", "keys": "screen_keys", "orrery": "screen_orrery", "horizon": "screen_horizon",
           "steps": "screen_steps", "sound": "screen_sound"}
LEGEND = ["build/film/legend.webp", "build/film/legend.m4a", "build/film/legend.png"]   # tools/make_legend.py
DEMOS = [   # build/audio/tour/NN_name.wav, in the systems' order
    ("KITTEN", "A beat to start from, on the grid: a kick, a clap, hats, a purring bass. Yours to change."),
    ("MILKY WAY", "A lullaby: a music box twinkling at 3:2 over pitched boops, a soft kick, a purring bass; the keys mew."),
    ("HARMONICES", "4, 5, 6 and 7 hits a bar on the chord 4:5:6:7, in the overtone scale: the idea itself."),
    ("GALILEAN", "Io, Europa and Ganymede's 1:2:4: four on the floor, a bass, a pad, hats that keep out of the kick's way."),
    ("TRAPPIST", "A resonant chain, 1:1 3:2 5:3 8:5: glassy plucks that mutate."),
    ("KIRKWOOD", "The asteroid gaps against Jupiter (3, 5/2, 2): pixel percussion over a pentatonic blip melody."),
    ("SPIRAL DRIFT", "16, 24, 20 and 28 steps at 1:1 3:2 5:4 7:5, all turning as they go; they tug at each other where they meet."),
    ("PULSAR", "Drums: an infall kick, a dust snare with a beam, pixel hats, a bass."),
    ("NEBULA", "Slow and wide, UTONAL, moons, a deep nebula; notes fall into tune, and now and then into the horizon."),
    ("ROCHE", "Torn apart: bells off whole ratios, dust, chaos, the tide."),
    ("LAGRANGE", "The keys are the sun's harmonics over a calm orbit."),
    ("ACCRETION", "Gravity high: every note falls into tune, the rings glide."),
    ("GW150914", "The first merger heard: notes chirp up from octaves below and ring down; every second bar it slides a little into the horizon."),
    ("ALPHA CEN", "A triple star: the rings pull hard on each other, the three-body problem, tuned."),
    ("SGR A*", "The black hole at the centre: when the pad's point dips, everything slides in, redshifted and partly backwards."),
    ("AFTERGLOW", "The oldest light under everything: chords from the overtone series, a blooming nebula, echoes that tire a fourth lower each time."),
    ("PURRSEC", "A cat café at 84, swung: a soft thump, a brushed pat, the purr walking the royal road under its chords; the keys a toy piano."),
    ("STARDUST", "A nursery of stars on the overtone series: stars at 3:2, glass at 5:4, a sub on the chords' roots, a long halo."),
    ("MOON NAP", "A lullaby on the subharmonics: a paw rocking the chord, a music box, a slow sub, a thirty-second halo; the keys mew."),
    ("ZOOMIES", "A kitten's mad minute at 124: four on the floor, a squeak chasing at 3:2, a growl leaping octaves, a chord a bar."),
]
SONGS = [   # build/audio/songs/NN_name.wav (orb_render song): the systems with chords, arranged
    ("PURRSEC", "Its chords in a halo, then the purr, the beat, the hats; a break where the toy piano sings the chords' own notes; the tail."),
    ("STARDUST", "The overtone chords alone, the sub, the half-time thump, the stars and the glass; the bell sings in the break."),
    ("MOON NAP", "No drums: the chords, the sub, the paw rocking, the music box and the stars; her mew in the break; thirty seconds of halo."),
    ("ZOOMIES", "The chords low, the growl, four on the floor and the clap, the squeak chasing; the pixel lead sings in the break."),
    ("MILKY WAY", "The lullaby arranged: the pentatonic chords, the bass, the soft kick, the boops and the music box; she mews in the break."),
    ("AFTERGLOW", "The oldest light: chords on the overtone series in a long halo, the sub, glass at 5:4, a music box at 7:4, tired echoes."),
]
SPEC_W, SPEC_H, SPEC_N = 1200, 160, 8192
SPEC_LO, SPEC_HI, SPEC_DB = 40.0, 16000.0, 75.0
NIGHT = [(0.00, (10, 6, 26)), (0.30, (40, 22, 100)), (0.55, (112, 66, 232)), (0.75, (255, 79, 168)),
         (0.90, (198, 255, 26)), (1.00, (255, 255, 255))]


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def read_wav(path):
    with wave.open(str(path)) as f:
        ch, fs = f.getnchannels(), f.getframerate()
        x = np.frombuffer(f.readframes(f.getnframes()), dtype="<i2").astype(np.float32)
    return x.reshape(-1, ch).mean(axis=1) / 32768.0, fs


def spectrum(x, fs, pic):
    """the piece's spectrum, left to right: a column a slice, the louder the brighter"""
    win = np.hanning(SPEC_N).astype(np.float32)
    x = np.concatenate([x, np.zeros(SPEC_N, np.float32)])
    hop = (len(x) - 2 * SPEC_N) / (SPEC_W - 1)
    freqs = np.fft.rfftfreq(SPEC_N, 1.0 / fs)
    edges = np.geomspace(SPEC_LO, SPEC_HI, SPEC_H + 1)
    cut = np.append(np.searchsorted(freqs, edges[:-1]), np.searchsorted(freqs, edges[-1]))
    db = np.empty((SPEC_H, SPEC_W), np.float32)
    for c in range(SPEC_W):
        s = int(c * hop)
        m = np.abs(np.fft.rfft(x[s:s + SPEC_N] * win))
        db[:, c] = 20.0 * np.log10(np.maximum.reduceat(m, cut)[:-1] + 1e-9)
    ref = np.percentile(db, 99.8)
    t = np.clip((db - (ref - SPEC_DB)) / SPEC_DB, 0.0, 1.0)[::-1] ** 1.25
    rgb = np.zeros(t.shape + (3,), np.float32)
    for (t0, c0), (t1, c1) in zip(NIGHT, NIGHT[1:]):
        k = ((t >= t0) & (t <= t1))[..., None]
        f = ((t - t0) / (t1 - t0))[..., None]
        rgb = np.where(k, np.array(c0) + f * (np.array(c1) - np.array(c0)), rgb)
    Image.fromarray(rgb.astype(np.uint8), "RGB").save(pic, quality=84, method=6)


def encode(wav, m4a):
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", str(wav), "-c:a", "aac", "-b:a", "160k", "-movflags",
                    "+faststart", str(m4a)], check=True)


def og_image(out):
    """the link preview, 1200 x 630: the README's header (tools/make_header.py), drawn at this size"""
    sys.path.insert(0, str(ROOT / "tools"))
    import make_header
    make_header.draw(1200, 630).save(out)


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not product.startswith("FM-1_967"):
        raise SystemExit(f"{pkg}: identity {product!r} is not ORBITRIX's (FM-1_967..)")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it: the installer could not write it")
    for src in list(APP) + LEGEND:
        if not (ROOT / src).exists():
            raise SystemExit(f"{src}: missing (web/app/build.sh, tools/make_legend.py)")
    tour = sorted((ROOT / "build" / "audio" / "tour").glob("[0-9][0-9]_*.wav"))
    if len(tour) != len(DEMOS):
        raise SystemExit(f"build/audio/tour: {len(tour)} renders, {len(DEMOS)} wanted (tests/run_orb.sh renders them)")
    songs = {w.stem.split("_", 1)[1].replace("_", " ").upper(): w
             for w in (ROOT / "build" / "audio" / "songs").glob("[0-9][0-9]_*.wav")}
    if set(songs) != {t for t, _ in SONGS}:
        raise SystemExit(f"build/audio/songs: {sorted(songs)}, wanted {[t for t, _ in SONGS]} (tests/run_orb.sh)")
    name = f"orbitrix-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    if out.exists():
        shutil.rmtree(out)
    for sub in ("firmware", "screens", "audio", "audio/songs", "app", "film"):
        (out / sub).mkdir(parents=True)
    shutil.copy(pkg, out / "firmware" / name)
    for src, dst in APP.items():
        shutil.copy(ROOT / src, out / "app" / dst)
    for src in LEGEND[:2]:
        shutil.copy(ROOT / src, out / "film" / Path(src).name)
    shutil.copy(ROOT / LEGEND[2], out / "screens" / "legend.png")
    shutil.copy(ROOT / "assets" / "cat.png", out / "screens" / "cat.png")
    shutil.copy(ROOT / "assets" / "logo.png", out / "screens" / "icon.png")
    for dst, src in SCREENS.items():
        shutil.copy(ROOT / "docs" / "readme" / f"{src}.png", out / "screens" / f"{dst}.png")
    og_image(out / "screens" / "og.png")
    demos = []
    for wav, (title, about) in zip(tour, DEMOS):
        x, fs = read_wav(wav)
        stem = wav.stem
        spectrum(x, fs, out / "audio" / f"{stem}.webp")
        encode(wav, out / "audio" / f"{stem}.m4a")
        demos.append({"id": stem, "title": title, "about": about, "src": f"audio/{stem}.m4a",
                      "spectrum": f"audio/{stem}.webp", "secs": round(len(x) / fs, 1)})
    song_list = []
    for title, about in SONGS:
        wav = songs[title]
        x, fs = read_wav(wav)
        stem = wav.stem
        spectrum(x, fs, out / "audio" / "songs" / f"{stem}.webp")
        encode(wav, out / "audio" / "songs" / f"{stem}.m4a")
        song_list.append({"id": "song_" + stem, "title": title, "about": about, "src": f"audio/songs/{stem}.m4a",
                          "spectrum": f"audio/songs/{stem}.webp", "secs": round(len(x) / fs, 1)})
    meta = json.dumps({"version": version, "product": product, "pkg": "firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest(), "size": len(raw)})
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    html = (HERE / "index.html").read_text(encoding="utf-8")
    for mark in ("/*LIB*/", "/*META*/", "/*DEMOS*/", "/*SONGS*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"web/index.html must hold {mark} once")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta).replace("/*DEMOS*/", json.dumps(demos))
    html = html.replace("/*SONGS*/", json.dumps(song_list))
    (out / "index.html").write_text(html, encoding="utf-8")
    (out / ".nojekyll").write_text("")
    size = sum(p.stat().st_size for p in out.rglob("*") if p.is_file())
    print(f"site: {out}: index.html ({len(html)} B), firmware/{name} ({len(raw)} B, {product}), {len(demos)} demos, {len(song_list)} songs, "
          f"ORBITRIX to play ({(out / 'app' / 'orbitrix.wasm').stat().st_size // 1024} KB), the legend, "
          f"{size // 1024} KB in all")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
