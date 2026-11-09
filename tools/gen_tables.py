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
  ORB_CHIRP    INSPIRAL's law: the share of the drop left at each 64th of the way (a binary's chirp, t^-3/8)
  ORB_SEMI     2^(s/12), Q16, s = -24..24: HORIZON's redshift
  ORB_BGP_V    the background radiation's chords (bg_chords): per tuning, eight progressions of up to eight chords, a
               bass and seven voices each (1/256 semitone over the sun in its octave 3, -32768 a rest), checked (in the
               tuning, every interval pure) and voice-led here; ORB_BGP_N their lengths, ORB_BGP_C, ORB_BGC_* their names
  ORB_PADW     its voice's wave: a soft saw, 8 harmonics and 4 (for the high notes: nothing folds over at 22 kHz)
  ORB_SILK     SILK's low-pass, a pole's coefficient (Q15) for 0..99: 9 kHz .. 400 Hz
"""
import itertools
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


# ---- BACKGROUND RADIATION: its chords. Eight progressions for each tuning, every chord in the tuning's own notes (so
# nothing in it clashes with the rings), every interval inside a chord a pure one (no wolf: in JUST, D and A or D and F
# never sound together), voiced here: a bass, seven voices above it, open (no thirds low down, no seconds, no minor
# ninths), each chord led into the next by the least movement (the notes it shares are held), round the cycle too
BG_PROGS = ["SUN", "TIDES", "ORBIT", "DUSK", "COMET", "ECLIPSE", "LULLABY", "VOYAGE"]
BG_NV = 8
BG_TOP = 3000                       # the highest voice, cents over the sun (in its octave 3)
REST = 99999.0                      # (a voice that rests in this chord)


def _red(r):
    while r >= 2:
        r /= 2
    while r < 1:
        r *= 2
    return r


def _cents(r):
    return 1200.0 * math.log2(float(r))


def bg_chords(scales):
    from fractions import Fraction as F
    J = scales["JUST"]
    # a chord: (root, its name, bass, the tones above in stacking order); root and bass in semitones over the sun (the
    # name's), the tones as ratios over the sun
    def semi(tun, root, suf, bass, ups):
        if tun == "ET":
            r = lambda k: 2 ** ((k % 12) / 12.0)
        else:
            if tun == "PENTA":
                assert all(k % 12 in (0, 2, 4, 7, 9) for k in [bass] + ups), (suf, ups)
            r = lambda k: J[k % 12]
        return (root, suf, bass % 12, r(bass), [r(k) for k in ups])

    def ratio(root, suf, bass_semi, bass, ups):
        return (root, suf, bass_semi, F(bass), [F(u) for u in ups])

    def diatonic(tun):
        c = lambda *a: semi(tun, *a)
        return {
            "Iadd9": c(0, "add9", 0, [0, 4, 7, 2]), "I6": c(0, "6", 0, [0, 4, 7, 9]),
            "Isus2": c(0, "sus2", 0, [0, 2, 7]), "Imaj9": c(0, "maj9", 0, [0, 4, 7, 11, 2]),
            "IVadd9": c(5, "add9", 5, [5, 9, 0, 7]), "IVmaj7": c(5, "maj7", 5, [5, 9, 0, 4]),
            "IVmaj9": c(5, "maj9", 5, [5, 9, 0, 4, 7]), "vi7": c(9, "m7", 9, [9, 0, 4, 7]),
            "vi9": c(9, "m9", 9, [9, 0, 4, 7, 11]), "iii7": c(4, "m7", 4, [4, 7, 11, 2]),
            "V6": c(7, "6", 7, [7, 11, 2, 4]), "bVImaj7": c(8, "maj7", 8, [8, 0, 3, 7]),
            "iv9": c(5, "m(add9)", 5, [5, 8, 0, 7]), "IV/I": c(5, "", 0, [5, 9, 0]), "V/I": c(7, "", 0, [7, 11, 2]),
        }

    def penta(tun):
        c = lambda *a: semi(tun, *a)
        return {
            "Iadd9": c(0, "add9", 0, [0, 4, 7, 2]), "I6": c(0, "6", 0, [0, 4, 7, 9]), "Isus2": c(0, "sus2", 0, [0, 2, 7]),
            "vi7": c(9, "m7", 9, [9, 0, 4, 7]), "iii7": c(4, "m7", 4, [4, 7, 2]), "I6/V": c(0, "6", 7, [7, 9, 0, 4]),
            "Iadd9/V": c(0, "add9", 7, [7, 0, 2, 4]), "iii7/II": c(4, "m7", 2, [2, 4, 7]),
        }

    H = lambda h: _red(F(h))
    otonal = {   # all on the sun's harmonic series
        "Oadd9": ratio(0, "add9", 0, H(4), [H(5), H(6), H(9)]),
        "Oh7": ratio(0, "h7", 0, H(4), [H(5), H(6), H(7)]),
        "Oh9": ratio(0, "h9", 0, H(4), [H(5), H(6), H(7), H(9)]),
        "Omaj7": ratio(0, "maj7", 0, H(8), [H(10), H(12), H(15)]),
        "Omaj9": ratio(0, "maj9", 0, H(8), [H(10), H(12), H(15), H(18)]),
        "Oiii7": ratio(4, "m7", 4, H(10), [H(12), H(15), H(18)]),
        "OV6": ratio(7, "6", 7, H(12), [H(15), H(18), H(20)]),
        "Osm": ratio(7, "sm", 7, H(6), [H(7), H(9)]),
        "Ou": ratio(4, "sm", 4, H(5), [H(6), H(7)]),
        "O7911": ratio(10, "h", 10, H(7), [H(9), H(11)]),
        "OV/I": ratio(7, "", 0, H(8), [H(12), H(15), H(18)]),
    }
    utonal = {   # all under the sun: its subharmonic series (the sun at the top of its own minor)
        "Ufm": ratio(5, "m", 5, F(4, 3), [F(8, 5), F(1), F(4, 3)]),
        "Ufm6": ratio(5, "m6", 5, F(4, 3), [F(8, 5), F(1), F(8, 7)]),
        "Udb": ratio(1, "", 1, F(16, 15), [F(4, 3), F(8, 5)]),
        "Udbmaj7": ratio(1, "maj7", 1, F(16, 15), [F(4, 3), F(8, 5), F(1)]),
        "Ubbm": ratio(10, "m", 10, F(16, 9), [F(16, 15), F(4, 3)]),
        "Ubbm7": ratio(10, "m7", 10, F(16, 9), [F(16, 15), F(4, 3), F(8, 5)]),
        "Uab6": ratio(5, "m", 8, F(8, 5), [F(1), F(4, 3)]),
        "Ubbm/F": ratio(10, "m", 5, F(4, 3), [F(16, 9), F(16, 15)]),
        "Udb/F": ratio(1, "", 5, F(4, 3), [F(16, 15), F(8, 5)]),
    }
    D = ["Iadd9"], ["Iadd9", "IVmaj7"], ["Iadd9", "vi7", "IVadd9", "V6"], ["vi9", "IVmaj7", "Iadd9", "iii7"], \
        ["IVmaj7", "V6", "iii7", "vi9"], ["Iadd9", "bVImaj7", "IVmaj7", "iv9"], ["Iadd9", "IV/I", "Iadd9", "V/I"], \
        ["Iadd9", "iii7", "vi9", "IVmaj9", "Iadd9", "V6", "IVadd9", "iv9"]
    P = ["Iadd9"], ["Iadd9", "vi7"], ["Iadd9", "iii7", "vi7", "I6/V"], ["vi7", "I6/V", "Iadd9", "iii7"], \
        ["I6/V", "iii7", "vi7", "Iadd9"], ["vi7", "iii7/II", "I6/V", "Iadd9"], ["Iadd9", "I6", "Isus2", "I6"], \
        ["Iadd9", "iii7", "vi7", "iii7/II", "I6/V", "Iadd9/V", "vi7", "I6"]
    O = ["Oadd9"], ["Oh9", "OV6"], ["Omaj9", "Oiii7", "OV6", "Oh9"], ["Oiii7", "Osm", "OV6", "Oadd9"], \
        ["OV6", "Oiii7", "Omaj7", "Oh9"], ["Oh9", "Ou", "Osm", "O7911"], ["Oadd9", "OV/I", "Omaj7", "Oh7"], \
        ["Oh9", "Oiii7", "OV6", "Osm", "O7911", "Ou", "Omaj9", "Oadd9"]
    U = ["Ufm"], ["Ufm", "Udbmaj7"], ["Ufm", "Udbmaj7", "Ubbm7", "Ufm6"], ["Ubbm7", "Udbmaj7", "Uab6", "Ufm"], \
        ["Udbmaj7", "Ubbm7", "Ufm", "Uab6"], ["Ufm6", "Udb", "Ubbm", "Ufm"], ["Ufm", "Ubbm/F", "Ufm", "Udb/F"], \
        ["Ufm", "Udb", "Ubbm7", "Ufm6", "Udbmaj7", "Ubbm", "Uab6", "Ufm"]
    plan = {"JUST": (diatonic("JUST"), D), "OTONAL": (otonal, O), "UTONAL": (utonal, U), "PENTA": (penta("PENTA"), P),
            "ET": (diatonic("ET"), D)}

    def pure(a, b):                 # an interval of small numbers (both ways under 17): no wolf, no tritone
        x = _red(a / b)
        y = _red(b / a)
        return (x.numerator <= 16 and x.denominator <= 16) or (y.numerator <= 16 and y.denominator <= 16)

    def label(bass, ups):           # its ratios as whole numbers, each tone stacked above the last: 4:5:6:9; or, if
        out, last = [F(1)], F(1)    # they are too big, as a subharmonic chord (u12:10:8:7: 1/12 1/10 1/8 1/7)
        for u in ups:
            r = _red(u / bass)
            if r == 1:
                continue
            while r <= last:
                r *= 2
            out.append(r)
            last = r

        def whole(rs):
            l = 1
            for r in rs:
                l = l * r.denominator // math.gcd(l, r.denominator)
            n = [int(r * l) for r in rs]
            g = 0
            for v in n:
                g = math.gcd(g, v)
            return [v // g for v in n]
        n = whole(out)
        if max(n) < 100:
            return ":".join(str(v) for v in n)
        n = whole([1 / r for r in out])
        return "u" + ":".join(str(v) for v in n) if max(n) < 100 else ""

    def voicings(bass_pc, ups_pc):  # every good voicing: (its cost, the bass and the seven above, REST for none)
        pcs = sorted({round(u % 1200.0, 3) for u in ups_pc})
        root = round(bass_pc % 1200.0, 3)
        fifth = round((bass_pc + 702.0) % 1200.0, 3)
        out = []
        for top, gap in ((BG_TOP, 260.0), (BG_TOP + 400, 260.0), (BG_TOP + 400, 200.0)):
            for b in (bass_pc + 1200.0 * k for k in range(-2, 2)):
                if not -500.0 <= b <= 600.0:     # (the bass about the sun: over the rings' bass, not under it)
                    continue
                cands = sorted({round(u + 1200.0 * k, 3) for u in set(pcs) | {root} for k in range(-1, 4)
                                if b + 480.0 <= u + 1200.0 * k <= top and u + 1200.0 * k >= 150.0})
                for nv in (BG_NV - 1, BG_NV - 2, BG_NV - 3):
                    for combo in itertools.combinations(cands, nv):
                        if not set(pcs) <= {round(x % 1200.0, 3) for x in combo}:
                            continue
                        ok, prev = True, b
                        for i, x in enumerate(combo):   # (a second only high up, where it shimmers: above G4)
                            g = x - prev
                            need = 480.0 if i == 0 or prev < 500.0 else gap if prev < 1850.0 else 180.0
                            if g < need - 1e-6 or g > 1300.0:
                                ok = False
                                break
                            prev = x
                        if not ok:
                            continue
                        allp = (b,) + combo
                        for i in range(len(allp)):
                            for j in range(i + 1, len(allp)):
                                d = allp[j] - allp[i]
                                if d > 1250.0 and 50.0 < d % 1200.0 < 150.0:   # (a minor ninth, or wider)
                                    ok = False
                        if not ok:
                            continue
                        cost = (b / 100.0) ** 2 * 0.05 + (BG_NV - 1 - nv) * 2.5
                        for i, x in enumerate(combo):
                            cost += ((x - (600.0 + 350.0 * i)) / 100.0) ** 2 * 0.1
                        for pc in set(pcs) | {root}:
                            n = sum(1 for x in combo if round(x % 1200.0, 3) == pc)
                            x = n - (0 if pc == root and pc not in pcs else 1)   # (its copies past the first)
                            if x > 0:   # (the root and fifth double best, once; a third or seventh hardly)
                                cost += x * 1.5 + (x - 1) * 2.0 if pc in (root, fifth) else x * 4.0
                        out.append((cost, allp + (REST,) * (BG_NV - 1 - nv)))
            if out:
                break
        assert out, "no voicing"
        out.sort()
        return out[:32]

    def lead(opts):                 # the cycle with the least movement (a held note costs nothing): Viterbi from each start
        A = [np.array([v for _, v in o]) for o in opts]
        S = [np.array([c for c, _ in o]) for o in opts]
        if len(A) == 1:
            return [A[0][0]]
        wv = np.array([0.6] + [1.0] * (BG_NV - 1))

        def vl(x, y):               # (to or from a rest: as a fade, 3)
            a, b = x[:, None, :], y[None, :, :]
            d = np.abs(a - b) / 100.0
            r = (a >= REST) | (b >= REST)
            d = np.where(r, np.where((a >= REST) & (b >= REST), 0.0, 3.0), d)
            return (d * wv).sum(axis=2) - 1.5 * ((d < 0.01) & ~r).sum(axis=2)
        best = None
        for s0 in range(len(A[0])):
            dp, backs, prev = np.array([S[0][s0]]), [], A[0][s0:s0 + 1]
            for i in range(1, len(A)):
                m = dp[:, None] + vl(prev, A[i]) + S[i][None, :]
                backs.append(m.argmin(axis=0))
                dp = m.min(axis=0)
                prev = A[i]
            tot = dp + vl(A[-1], A[0][s0:s0 + 1])[:, 0]
            j = int(tot.argmin())
            if best is None or tot[j] < best[0]:
                js = [0] * len(A)
                js[-1] = j
                for i in range(len(A) - 1, 0, -1):
                    js[i - 1] = int(backs[i - 1][js[i]])
                js[0] = s0
                best = (float(tot[j]), js)
        return [A[i][j] for i, j in enumerate(best[1])]

    return plan, pure, label, voicings, lead


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

    # ---- BACKGROUND RADIATION: its chords (bg_chords: per tuning, eight progressions of up to eight chords, eight
    # voices each, 1/256 semitone over the sun in its octave 3), what each chord is called; the voice's wave (a soft
    # saw: 8 harmonics, and 4 for the high notes, so nothing folds over at NEBULA's half rate); SILK's low-pass
    plan, pure, label, voicings, lead = bg_chords(scales)
    cat, cat_ix, bgn, bgc, bgv = [], {}, [], [], []
    for n in names:
        chords, progs = plan[n]
        assert len(progs) == len(BG_PROGS)
        scale = None if n == "ET" else set(scales[n])
        rn, rc, rv = [], [], []
        for prog in progs:
            assert 1 <= len(prog) <= 8
            opts, ids = [], []
            for cn in prog:
                root, suf, bsemi, bass, ups = chords[cn]
                lab = ""
                if scale is not None:
                    tones = [bass] + ups
                    for t in tones:
                        assert _red(t) in scale, (n, cn, t)
                    for i in range(len(tones)):
                        for j in range(i + 1, len(tones)):
                            assert _red(tones[i] / tones[j]) == 1 or pure(tones[i], tones[j]), (n, cn, tones[i], tones[j])
                    lab = label(bass, ups)
                key = (root, suf, bsemi, lab)
                if key not in cat_ix:
                    cat_ix[key] = len(cat)
                    cat.append(key)
                ids.append(cat_ix[key])
                opts.append(voicings(_cents(bass) % 1200.0, [_cents(u) % 1200.0 for u in ups]))
            vs = lead(opts)
            rn.append(len(prog))
            rc.append(ids + [255] * (8 - len(ids)))
            v = [[-32768 if c >= REST else int(round(c * 2.56)) for c in x] for x in vs]
            rv.append(v + [v[0]] * (8 - len(v)))
        bgn.append(rn)
        bgc.append(rc)
        bgv.append(rv)
    parts.append("static const char *const ORB_BGPROG_NAME[%d] = {%s};\n" % (len(BG_PROGS), ", ".join(f'"{n}"' for n in BG_PROGS)))
    parts.append("static const uint8_t ORB_BGP_N[%d][%d] = {%s};\n" % (len(names), len(BG_PROGS),
                 ", ".join("{" + ", ".join(str(v) for v in r) + "}" for r in bgn)))
    lines = ["static const uint8_t ORB_BGP_C[%d][%d][8] = {" % (len(names), len(BG_PROGS))]
    for n, r in zip(names, bgc):
        lines.append("    {" + ", ".join("{" + ", ".join(str(v) for v in c) + "}" for c in r) + "},   /* " + n + " */")
    lines.append("};\n")
    parts.append("\n".join(lines))
    lines = ["static const int16_t ORB_BGP_V[%d][%d][8][%d] = {" % (len(names), len(BG_PROGS), BG_NV)]
    for n, r in zip(names, bgv):
        lines.append("    {   /* " + n + " */")
        for pi, pr in enumerate(r):
            lines.append("        {" + ", ".join("{" + ", ".join(str(v) for v in c) + "}" for c in pr) + "},   /* " + BG_PROGS[pi] + " */")
        lines.append("    },")
    lines.append("};\n")
    parts.append("\n".join(lines))
    parts.append(c_array("ORB_BGC_ROOT", "uint8_t", [k[0] for k in cat]))
    parts.append(c_array("ORB_BGC_BASS", "uint8_t", [k[2] for k in cat]))
    parts.append("static const char *const ORB_BGC_SUF[%d] = {%s};\n" % (len(cat), ", ".join(f'"{k[1]}"' for k in cat)))
    parts.append("static const char *const ORB_BGC_RATIO[%d] = {%s};\n" % (len(cat), ", ".join(f'"{k[3]}"' for k in cat)))
    amps = [1.0 / h ** 1.45 for h in range(1, 9)]
    waves = []
    for nh in (8, 4):
        w = [sum(amps[h - 1] * math.sin(2 * math.pi * h * i / 1024) for h in range(1, nh + 1)) for i in range(1025)]
        pk = max(abs(x) for x in w)
        waves.append([int(round(30000 * x / pk)) for x in w])
    lines = ["static const int16_t ORB_PADW[2][1025] = {"]
    for w in waves:
        lines.append("    {" + ", ".join(str(v) for v in w) + "},")
    lines.append("};\n")
    parts.append("\n".join(lines))
    silk = [int(round(32767 * (1 - math.exp(-2 * math.pi * 9000.0 * (400.0 / 9000.0) ** (v / 99.0) / 22050.0)))) for v in range(100)]
    parts.append(c_array("ORB_SILK", "int16_t", silk))
    bg_info = (len(BG_PROGS), len(cat), bgv)

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

    # ---- INSPIRAL: a binary's chirp. Its frequency goes as (t_c - t)^(-3/8); the note starts 2 octaves under and
    # lands at t = 1. S(u) is the share of the drop still to go at u = t / T (1 at the start, 0 at the merge); other
    # depths scale the same shape
    K = 2 ** (8 * 2 / 3.0)
    tc = K / (K - 1)
    chirp = [int(round(32767 * math.log((tc - u) / (tc - 1)) / math.log(tc / (tc - 1)))) for u in (i / 64.0 for i in range(65))]
    chirp[64] = 0
    parts.append(c_array("ORB_CHIRP", "int16_t", chirp))
    # ---- HORIZON: 2^(s/12) in Q16 for s = -24..24 semitones (the grains' rate: redshift, blueshift)
    parts.append(c_array("ORB_SEMI", "uint32_t", (f"{int(round(65536 * 2 ** (k / 12.0)))}u" for k in range(-24, 25)), 8))

    head = ("/* SPDX-License-Identifier: GPL-3.0-only */\n"
            "/* Generated by tools/gen_tables.py. Do not edit: change the laws there and run it. */\n"
            "#ifndef ORB_TABLES_H\n#define ORB_TABLES_H\n#include <stdint.h>\n\n"
            f"#define ORB_NE {NE}\n#define ORB_NM {NM}\n#define ORB_NTUNING {len(names)}\n"
            f"#define ORB_NMOON {len(moons)}\n#define ORB_NRATIO {len(ratios)}\n"
            f"#define ORB_NBGPROG {bg_info[0]}\n#define ORB_NBGC {bg_info[1]}\n"
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
    for n, r in zip(names, bg_info[2]):
        print("BG", n, "ORBIT:", " | ".join(" ".join(str(round(v / 256, 1)) for v in c) for c in r[2][:4]))


if __name__ == "__main__":
    main()
