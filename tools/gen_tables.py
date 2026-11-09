#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
"""ORBITRIX's laws, as tables: firmware/src/orb_tables.h. Change a law here and run it again; never edit the header.

  python3 tools/gen_tables.py            (needs numpy)

What is here (docs/orbitrix/PLAN.md):
  ORB_SIN      one sine period, 1024 points + 1, Q15
  ORB_KEP      Kepler's equation: E - M for 33 eccentricities (0 .. 0.96) x 512 mean anomalies (+1), in 2^-16 turns
  ORB_KEPW     dE/dM = 1 / (1 - e cos E) on the same grid, Q10: the planet's speed (SLING: the velocity view)
  ORB_GX/GY    per eccentricity row: the gains (Q12) that give x = cos E + e/2 and y = sin E the RMS of a sine (the
               geometric y is sqrt(1-e^2) sin E: normalised, the factor goes; GY is 1 by Kepler: <sin^2 E>_M = 1/2)
  ORB_GVX/GVY  the same for the velocity, vx = -sin E dE/dM and vy = cos E dE/dM, held to a peak of 2
  ORB_ECAP     per MIDI note: the highest eccentricity (row units, Q11) whose harmonics stay under 24 kHz (-60 dB);
  ORB_ECAPV    the same for the velocity view
  ORB_EXP2     the oscillator increment of C10 (MIDI 132) times 2^(i/768), i = 0..768: inc for any pitch, by shifts
  ORB_ATK      envelope attack: the Q24 step a control tick (32 frames) for 100 times (1 ms .. 20 s)
  ORB_DEC      envelope decay/release: the Q16 share of the way a tick, for the same 100 times (time = to -60 dB)
  ORB_GCUT     the filter's g = tan(pi fc / fs), Q24, for 129 cutoffs 20 Hz .. 18 kHz
  ORB_LFO      LFO increments a control tick for 100 rates (0.02 .. 20 Hz)
  ORB_SCALE    the tunings' degrees in 1/256 semitone (JUST, OTONAL, UTONAL, PENTA, ET), ORB_SCALE_N their sizes
  ORB_KEYDEV   per tuning, the retuning of each semitone above the sun, 1/256 semitone (SOLAR keys)
  ORB_HARM     harmonics 1..32 of the sun in 1/256 semitone above it (HARMONIC keys)
  ORB_MRATIO   the moon's ratios (Q8) and their names
  ORB_RATIO    the rings' speed ratios p:q and names
"""
import math
from pathlib import Path

import numpy as np

OUT = Path(__file__).resolve().parents[1] / "firmware" / "src" / "orb_tables.h"
FS = 44100.0
TICK = 32.0 / FS                    # the control tick: 32 frames
NE = 33                             # eccentricity rows: e = 0.96 * r / 32
E_TOP = 0.96
NM = 512                            # mean anomaly columns (+1 guard)


def kepler_E(M, e):
    """E for mean anomaly M (radians, array) and eccentricity e: Newton, from a start that converges for e < 1."""
    E = M + e * np.sin(M) if e < 0.8 else np.full_like(M, math.pi)
    for _ in range(60):
        E = E - (E - e * np.sin(E) - M) / (1.0 - e * np.cos(E))
    return E


def osc_xy(e, n=8192):
    """x, y of the orbit (as the engine has them, before the gains) over one period of uniform M."""
    M = 2 * math.pi * np.arange(n) / n
    E = kepler_E(M, e)
    return np.cos(E) + e / 2.0, np.sin(E)          # (y: sin E, not b sin E: its RMS over M is 1/sqrt 2 for any e)


def osc_v(e, n=8192):
    """the velocity of the orbit (d/dM), as the engine has it before the gains."""
    M = 2 * math.pi * np.arange(n) / n
    E = kepler_E(M, e)
    w = 1.0 / (1.0 - e * np.cos(E))
    return -np.sin(E) * w, np.cos(E) * w           # (vy without b, as y: the gain carries the level)


def top_harmonic(sig):
    spec = np.abs(np.fft.rfft(sig))
    spec[0] = 0
    top = spec.max()
    k = len(spec) - 1
    while k > 1 and spec[k] < top * 1e-3:
        k -= 1
    return k


def cap_table(nh):
    """per MIDI note 0..143: the largest e (row units Q11) with f * NH <= 24 kHz, between rows in log harmonics"""
    ecap = []
    for note in range(144):
        f = 440.0 * 2 ** ((note - 69) / 12.0)
        nmax = 24000.0 / f
        best = 0.0
        for r in range(NE):
            if nh[r] <= nmax:
                best = float(r)
            else:
                if r > 0:
                    a, b = math.log(nh[r - 1]), math.log(nh[r])
                    t = (math.log(nmax) - a) / (b - a) if b > a else 0.0
                    best = r - 1 + max(0.0, min(1.0, t))
                break
        ecap.append(min(65535, int(best * 2048)))
    return ecap


def c_array(name, ctype, vals, per_line=12):
    vals = list(vals)
    lines = []
    for i in range(0, len(vals), per_line):
        lines.append("    " + ", ".join(str(v) for v in vals[i:i + per_line]) + ",")
    return f"static const {ctype} {name}[{len(vals)}] = {{\n" + "\n".join(lines) + "\n};\n"


def main():
    parts = []

    # ---- the sine
    sin = [int(round(32767 * math.sin(2 * math.pi * i / 1024))) for i in range(1025)]
    parts.append(c_array("ORB_SIN", "int16_t", sin))

    # ---- Kepler: E - M in 2^-16 turns
    kep, kepw = [], []
    gx, gy, nh, gvx, gvy, nhv = [], [], [], [], [], []
    for r in range(NE):
        e = E_TOP * r / (NE - 1)
        M = 2 * math.pi * np.arange(NM + 1) / NM
        E = kepler_E(M, e)
        d = (E - M) / (2 * math.pi) * 65536.0
        row = [int(round(v)) for v in d]
        row[NM] = row[0]                # periodic
        assert max(abs(v) for v in row) < 32767
        kep.extend(row)
        w = 1.0 / (1.0 - e * np.cos(E))
        wrow = [int(round(v * 1024)) for v in w]
        assert max(wrow) < 32767
        kepw.extend(wrow)
        vx, vy = osc_v(e)
        g1 = min(0.7071 / np.sqrt(np.mean(vx * vx)), 2.0 / np.abs(vx).max())
        g2 = min(0.7071 / np.sqrt(np.mean(vy * vy)), 2.0 / np.abs(vy).max())
        gvx.append(int(round(4096 * g1)))
        gvy.append(int(round(4096 * g2)))
        nhv.append(max(top_harmonic(vx), top_harmonic(vy)))
        x, y = osc_xy(e)
        rx, ry = np.sqrt(np.mean(x * x)), np.sqrt(np.mean(y * y))
        gx.append(int(round(4096 * 0.7071 / rx)))
        gy.append(int(round(4096 * 0.7071 / ry)))
        # the highest harmonic with energy that matters: from the top, the first above -60 dB of the strongest
        spec = np.abs(np.fft.rfft(x * gx[-1] / 4096.0)) + np.abs(np.fft.rfft(y * gy[-1] / 4096.0))
        spec[0] = 0
        top = spec.max()
        k = len(spec) - 1
        while k > 1 and spec[k] < top * 1e-3:
            k -= 1
        nh.append(k)
    lines = ["static const int16_t ORB_KEP[%d][%d] = {" % (NE, NM + 1)]
    for r in range(NE):
        row = kep[r * (NM + 1):(r + 1) * (NM + 1)]
        lines.append("    {" + ", ".join(str(v) for v in row) + "},")
    lines.append("};\n")
    parts.append("\n".join(lines))
    lines = ["static const int16_t ORB_KEPW[%d][%d] = {" % (NE, NM + 1)]
    for r in range(NE):
        lines.append("    {" + ", ".join(str(v) for v in kepw[r * (NM + 1):(r + 1) * (NM + 1)]) + "},")
    lines.append("};\n")
    parts.append("\n".join(lines))
    parts.append(c_array("ORB_GVX", "int16_t", gvx))
    parts.append(c_array("ORB_GVY", "int16_t", gvy))
    parts.append(c_array("ORB_GX", "int16_t", gx))
    parts.append(c_array("ORB_GY", "int16_t", gy))
    parts.append(c_array("ORB_NH", "uint16_t", nh))

    ecap = cap_table(nh)
    ecapv = cap_table(nhv)
    parts.append(c_array("ORB_ECAP", "uint16_t", ecap))
    parts.append(c_array("ORB_ECAPV", "uint16_t", ecapv))

    # ---- pitch: inc of C10 (MIDI 132) * 2^(i/768)
    f132 = 440.0 * 2 ** ((132 - 69) / 12.0)
    exp2 = [min(0xFFFFFFFF, int(round(f132 * 2 ** (i / 768.0) * 2 ** 32 / FS))) for i in range(769)]
    parts.append(c_array("ORB_EXP2", "uint32_t", (f"{v}u" for v in exp2), 8))

    # ---- envelopes: 100 times, 1 ms .. 20 s
    times = [0.001 * 20000.0 ** (v / 99.0) for v in range(100)]
    atk = [min(1 << 24, int(round((1 << 24) * TICK / t))) for t in times]
    dec = []
    for t in times:
        tau = t / math.log(1000.0)
        dec.append(max(1, min(65535, int(round(65536 * (1 - math.exp(-TICK / tau)))))))
    parts.append(c_array("ORB_ATK", "uint32_t", (f"{v}u" for v in atk), 8))
    parts.append(c_array("ORB_DEC", "uint16_t", dec))
    ms = [int(round(t * 1000)) for t in times]
    parts.append(c_array("ORB_TIME_MS", "uint16_t", ms))

    # ---- the filter: g = tan(pi fc / fs), Q24, 129 cutoffs 20 Hz .. 18 kHz
    gcut = []
    for i in range(129):
        fc = 20.0 * (900.0 ** (i / 128.0))
        fc = min(fc, 0.45 * FS)
        gcut.append(int(round((1 << 24) * math.tan(math.pi * fc / FS))))
    parts.append(c_array("ORB_GCUT", "int32_t", gcut, 8))

    # ---- LFO rates: 0.02 .. 20 Hz, phase increment a control tick (2^32 a cycle)
    lfo = [int(round(0.02 * 1000.0 ** (v / 99.0) * TICK * 2 ** 32)) for v in range(100)]
    parts.append(c_array("ORB_LFO", "uint32_t", (f"{v}u" for v in lfo), 8))

    # ---- tunings
    def st256(ratio):
        return int(round(12 * math.log2(ratio) * 256))
    from fractions import Fraction as F
    scales = {
        "JUST": [F(1), F(16, 15), F(9, 8), F(6, 5), F(5, 4), F(4, 3), F(7, 5), F(3, 2), F(8, 5), F(5, 3), F(7, 4), F(15, 8)],
        "OTONAL": [F(1), F(9, 8), F(5, 4), F(11, 8), F(3, 2), F(13, 8), F(7, 4), F(15, 8)],
        "UTONAL": [F(1), F(16, 15), F(8, 7), F(16, 13), F(4, 3), F(16, 11), F(8, 5), F(16, 9)],
        "PENTA": [F(1), F(9, 8), F(5, 4), F(3, 2), F(5, 3)],
        "ET": [2 ** (k / 12.0) for k in range(12)],
    }
    names = list(scales)
    sc_rows, sc_n, keydev = [], [], []
    for n in names:
        degs = [st256(float(r)) for r in scales[n]]
        sc_n.append(len(degs))
        sc_rows.append(degs + [0] * (12 - len(degs)))
        # keys: each semitone above the sun to the nearest degree of this tuning; JUST for PENTA and ET's 0
        pool = degs if n not in ("PENTA",) else [st256(float(r)) for r in scales["JUST"]]
        if n == "OTONAL":               # all of harmonics 16..31: twelve semitones each find their own
            pool = [st256(h / 16.0) for h in range(16, 32)]
        if n == "UTONAL":
            pool = [st256(32.0 / h) for h in range(17, 33)] + [0]
        row = []
        for s in range(12):
            target = s * 256
            near = min(pool, key=lambda d: abs(d - target))
            row.append(near - target)
        keydev.append(row)
    lines = ["static const int16_t ORB_SCALE[%d][12] = {" % len(names)]
    for n, row in zip(names, sc_rows):
        lines.append("    {" + ", ".join(str(v) for v in row) + "},   /* " + n + " */")
    lines.append("};\n")
    parts.append("\n".join(lines))
    parts.append(c_array("ORB_SCALE_N", "uint8_t", sc_n))
    lines = ["static const int16_t ORB_KEYDEV[%d][12] = {" % len(names)]
    for n, row in zip(names, keydev):
        lines.append("    {" + ", ".join(str(v) for v in row) + "},   /* " + n + " */")
    lines.append("};\n")
    parts.append("\n".join(lines))
    parts.append("static const char *const ORB_TUNING_NAME[%d] = {%s};\n" % (len(names), ", ".join(f'"{n}"' for n in names)))
    parts.append(c_array("ORB_HARM", "int16_t", [st256(h) for h in range(1, 33)]))

    # ---- the moon's ratios
    moons = [("1", 1.0), ("2", 2.0), ("3", 3.0), ("4", 4.0), ("5", 5.0), ("6", 6.0), ("7", 7.0), ("8", 8.0),
             ("1/2", 0.5), ("3/2", 1.5), ("5/2", 2.5), ("PHI", 1.6180340), ("SQ2", 1.4142136), ("BEL", 2.7565),
             ("e", 2.7182818), ("PI", 3.1415927), ("SQ7", 2.6457513), ("5.4", 5.4)]
    parts.append(c_array("ORB_MRATIO", "uint16_t", [int(round(r * 256)) for _, r in moons]))
    parts.append("static const char *const ORB_MRATIO_NAME[%d] = {%s};\n" % (len(moons), ", ".join(f'"{n}"' for n, _ in moons)))

    # ---- the rings' speeds: p steps in the time of q base steps (a 16th); 2520 ticks a base step
    ratios = [(1, 4), (1, 3), (1, 2), (2, 3), (3, 4), (4, 5), (5, 6), (1, 1), (6, 5), (5, 4), (4, 3), (7, 5), (3, 2),
              (8, 5), (5, 3), (7, 4), (9, 5), (2, 1), (9, 4), (7, 3), (5, 2), (3, 1), (4, 1)]
    for p, q in ratios:
        assert (2520 * q) % p == 0
    parts.append(c_array("ORB_RATIO_P", "uint8_t", [p for p, _ in ratios]))
    parts.append(c_array("ORB_RATIO_Q", "uint8_t", [q for _, q in ratios]))
    parts.append(c_array("ORB_RATIO_TICKS", "uint16_t", [2520 * q // p for p, q in ratios]))
    parts.append("static const char *const ORB_RATIO_NAME[%d] = {%s};\n" % (len(ratios), ", ".join(f'"{p}:{q}"' for p, q in ratios)))

    head = ("/* SPDX-License-Identifier: GPL-3.0-only */\n"
            "/* Generated by tools/gen_tables.py. Do not edit: change the laws there and run it. */\n"
            "#ifndef ORB_TABLES_H\n#define ORB_TABLES_H\n#include <stdint.h>\n\n"
            f"#define ORB_NE {NE}\n#define ORB_NM {NM}\n#define ORB_NTUNING {len(names)}\n"
            f"#define ORB_NMOON {len(moons)}\n#define ORB_NRATIO {len(ratios)}\n"
            f"#define ORB_RATIO_1_1 {ratios.index((1, 1))}\n\n")
    OUT.write_text(head + "\n".join(parts) + "\n#endif\n")
    print(f"{OUT}: {OUT.stat().st_size} B")
    print("NH per row:", nh)
    print("NH velocity:", nhv)
    print("ECAP rows at A1 A2 A3 A4 A5:", [round(ecap[n] / 2048, 2) for n in (33, 45, 57, 69, 81)])
    print("ECAPV rows at A1 A2 A3 A4 A5:", [round(ecapv[n] / 2048, 2) for n in (33, 45, 57, 69, 81)])
    print("GVX:", gvx[::8], "GVY:", gvy[::8])
    print("GX:", gx[::8], "GY:", gy[::8])
    print("KEYDEV JUST (cents):", [round(v / 2.56, 1) for v in keydev[0]])


if __name__ == "__main__":
    main()
