# ORBITRIX — decisions

What was decided, when, and why. Newest last.

## D1 — The idea (2026-11-09)

The brief: an FM-1 firmware unlike any other, deeply unorthodox in how it works and how it sounds, yet harmonically
beautiful. The look: the user's two ORBITRIX drawings (a sound-design screen and a sequencer screen).

Decided: **rhythm and harmony are the same ratios.** Four rings step at small ratios to a 16th (1:1, 5:4, 3:2, 7:4 ...),
and each sings an interval above the sun that is (in the flagship system) the same ratio. The unorthodox part is the
rhythm (polyrhythms of 4 against 5, 6 and 7, cycles that meet only every few bars). The beautiful part is the pitch:
every note is on the sun's harmonic series, so the whole fuses into one chord.

## D2 — The voice: orbit synthesis (2026-11-09)

The voice is a planet on a Kepler ellipse, heard as its position: `M = E − e sin E`, solved by a 33 × 513 table of
E − M (bilinear in M and e). The x view is `cos E + e/2` (zero mean over time). The y view is `sin E`, not the
geometric `sqrt(1 − e²) sin E`: normalised to a sine's RMS the factor goes, and over time `<sin² E> = 1/2` for any e.

- The spectrum (measured, `gen_tables.py`): at e = 0.3 the harmonics fall ~13 dB each, at e = 0.96 only ~3 dB after
  the second. It is warm and smooth, beautiful but mild.
- So **SLING** was added: the velocity view (`−sin E · dE/dM`, `cos E · dE/dM`, dE/dM from a second table, exact, no
  finite difference and none of its noise at low notes). At high e it is a bright pulse at the near point. Its gains
  hold its peak to 2 within a row; between rows they are interpolated and the velocity clamped to ±3.
- **Stereo from geometry:** L and R are two views of the same orbit (ANG ± SPR/2). L + R is another view, so mono
  never cancels.
- **The aliasing cap:** for each MIDI note, the largest e whose harmonics above −60 dB stay under 24 kHz (aliases
  above 20 kHz), for position and velocity, blended by SLING. Measured: F6 at ECC 99 + SLING 99 has its inharmonic
  content 66.7 dB down.
- PIXEL is exempt from the cap: aliasing is its sound.

## D3 — Harmony: gravity (2026-11-09)

The tunings are JUST (5- and 7-limit 12), OTONAL (harmonics 8–15), UTONAL, PENTA and ET. The keys retune to the sun
(SOLAR), or play its harmonics 1–27 (HARMONIC). GRAVITY 0..50 blends from equal temperament to fully just; above 50 a
note falls into tune on a damped spring (10 Hz, ζ 0.5 just above 50 … 3 Hz, ζ 0.15 at 99), from the last note or a
little off. The rings glide the same way between notes when GRAVITY is above 50, or when the sound is LEGATO.

## D4 — The orrery's clock (2026-11-09)

2520 ticks a 16th (lcm 1..10): every ratio p:q with p ≤ 10 is a whole number of ticks a step, so the rings meet exactly,
forever, with no drift. A ring's step fires at sub-block (32-frame, 0.73 ms) resolution. A conjunction is two rings
stepping on the same tick (their unswung ticks equal). The clock always runs (LFOs and the pad stay on it); PLAY
restarts it and only then do the rings step.

Each ring has one voice (four voices), and the keys have four. That makes eight, never more: the CPU never meets a
ninth.

## D5 — MUTATE in the main loop (2026-11-09)

The interrupt counts each ring's laps; the main loop mutates the pattern when a lap count changes (one writer for the
pattern: the main loop). A hit moves to a free neighbour or a degree steps; the number of hits stays (tested).

## D6 — The budget (2026-11-09)

Counted on the chip's own code (JieLi clang -Os, objdump): the voice loop is 245 instructions a frame with everything
on, ~110 plain; the FX loop ~500 a frame (the filter 2 × 101 when it is on). NEBULA runs at half the rate (22 kHz: a
damped tail has nothing above 11 kHz to lose); that saved ~10 %. Estimate: typical systems ~30 % of the chip, the worst
case (8 voices, everything on, the filter driven) under 50 %. Still to measure on the device (the hint line's CPU %)
or in fm1-emu.

## D7 — Identity (2026-11-09)

`FM-1_967XY` (967 was free in the Firmware Hub's catalogue on 2026-11-09), 96700 a dev build; the store magic
`ORBX`; the SMK-37 owner tag `ORBTRX`; the update loader's screen says ORBITRIX in lime (#C6FF1A).
