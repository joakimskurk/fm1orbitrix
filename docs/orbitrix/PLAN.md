# ORBITRIX — the plan

**Status:** v0.1 plan, 2026-11-09. Platform: Dinghy 0.5 (docs/DINGHY.md, docs/API.md, docs/RULES.md).
**The brief (the user's):** an FM-1 firmware unlike anything else: deeply unorthodox, and sounding as unorthodox, but
harmonically beautiful. The look: the two ORBITRIX screens the user drew (deep indigo, violet frames, lime, cyan,
magenta and yellow, pixel stars, rings and spirals).

## 1. The idea in one line

**An orbital groove engine: every rhythm is an interval, and every interval a rhythm.** Four planets circle a sun.
How fast each one goes against the others is a small ratio (1:1, 3:2, 5:4, 7:4 ...), and the same ratio, heard as
pitch, is a pure interval above the sun. A polyrhythm of 4 against 5 against 6 against 7 *is* the chord 4:5:6:7.

The unorthodox part and the beautiful part are the same thing seen from two sides. The rhythms are strange (quintuplets
against septuplets, cycles that only meet every few bars), but every pitch sits on the sun's harmonic series, so it
all fuses into one chord without a single beat.

## 2. The sound: orbit synthesis

The voice is a planet on an ellipse, and the sound is where it is.

| What | How | What you hear |
| --- | --- | --- |
| **ECC**, eccentricity 0..0.96 | Kepler's equation `M = E - e sin E`, solved in a table (`KEP`, 33 x 513) | 0: a pure sine. Up: the planet slows far out and whips through the near point: brighter, buzzier, a pulse at the top. Its spectrum is Bessel functions, the same as FM (Bessel found them solving this equation) |
| **ANG**, the projection angle | out = x cos θ + y sin θ | x and y have different spectra: turning ANG moves the timbre between them |
| **SPR**, spread | L at θ, R at θ + spread | stereo from geometry: two views of one motion. L + R is again a projection, so it never cancels in mono |
| **PREC**, precession | θ turns slowly by itself (free or on the beat) | a slow timbral swirl, the stereo image rotating |
| **MOON** and its **RATIO** | a moon circles the planet: x, y + r (cos h·M, sin h·M) | an epicycle. Whole h: harmonics. h = φ, √2, 2.76 ...: bells and metal |
| **DUST** and **COLOR** | noise in x and y, low- or high-passed | breath, hiss, the snare's wire, comets |
| **PIXEL** | x and y on a coarse grid | an 8-bit planet: stepped, aliased, on purpose |
| **FALL**, **FALL T** | the pitch falls from above onto the note | kicks and toms (an infall), or a soft swoop |

Each voice: amplitude ADSR, an eccentricity envelope (amount, attack, decay: the "FM index" envelope, here the orbit
stretching and relaxing), velocity, pan, a send to SPACE.

Aliasing: per note, a table (`ECAP`, from the measured spectra) caps the eccentricity so its harmonics stay under
24 kHz (aliases land above 20 kHz). PIXEL is exempt: that is its sound.

## 3. Harmony: gravity

Everything is tuned to the **sun** (a pitch class; the rings stand on it).

- **TUNING**: JUST (5- and 7-limit: 1/1 16/15 9/8 6/5 5/4 4/3 7/5 3/2 8/5 5/3 7/4 15/8), OTONAL (the overtone scale,
  harmonics 8–15: 1/1 9/8 5/4 11/8 3/2 13/8 7/4 15/8), UTONAL (its mirror, dark), PENTA (1/1 9/8 5/4 3/2 5/3), and
  ET (no retuning).
- The rings play scale **degrees**; their pitches are exact ratios to the sun.
- The keys (SOLAR mode) play the ordinary keyboard, each key retuned to the ratio of its interval to the sun: chords
  ring without beating. In HARMONIC mode the 16 white keys are harmonics 1–16 of the sun, the 11 black ones 17–27.
- **GRAVITY** (macro): 0 is equal temperament as played; 50 is fully just; above 50 a note *falls* into tune: it
  starts where the last one was (or a little off) and spirals in on a damped spring, overshooting and settling, like a
  voice finding the pitch. Rings glide the same way between notes when they are legato.

## 4. Rhythm: the Orrery

Four rings, A–D. Each ring:

| | |
| --- | --- |
| **RATIO** p:q | its speed against the base step (a 16th): 1:4 … 1:1 … 7:4 … 4:1. All ratios meet exactly: the clock counts 2520 ticks a 16th (2520 = lcm 1..10), so a ring's step is an integer number of ticks |
| **LEN** 1–64 | steps on the ring |
| **HITS**, **ROT** | an even (Euclidean) spread of hits, rotated; steps can be set by hand on the white keys |
| **DEG** | the degree it sings (A: the sun, B: the fifth, C: the third, D: the seventh in the default system) |
| **DIR** | FWD, REV, PING, SPIRAL (forwards, and the pattern turns one step each lap: rhythmic precession), RND |
| **LOGIC** | FREE, AND x, NOT x, XOR x (against ring x's current step), CONJ (only together with another ring) |
| **PROB**, **GATE** | per ring |

**Conjunctions:** when two or more rings fire on the same tick, the hits are accented, and the CONJ modulation source
flares. The long cycle's downbeats (where the orbits meet) are heard as accents by themselves.

Global: tempo, SWING, **MUTATE** (at each lap, steps move and degrees step to their neighbours), **HUM** (timing and
velocity humanised), **VAR** (degree variation on new hits).

Each ring plays its own sound (one voice each: a ring is monophonic, so conjunctions are chords across rings). The
keys play the KEYS sound on four voices. Eight voices in all, always: the CPU never meets a ninth.

## 5. Movement

- **Macros** (0 = the system as programmed): **CHAOS** (the orbits wobble: eccentricity and angle random walks, the
  moons drift off whole ratios, rings mutate more), **SPECTRAL** (brighter: eccentricity, moon, the filter), **GRAVITY**
  (§3), **FRACT** (pixel grid and ratchets: hits split into 2–4), **ORBIT** (the pad's radius and speed, precession,
  spread).
- **ORBIT pad**: a point circling (X, Y) at radius RAD, at RATE (on the beat). Its X and Y are modulation sources.
- **Two LFOs** (sine, triangle, saw, square, S&H), free or locked to the orrery's clock.
- **Mod matrix**, 8 slots: LFO1 LFO2 ORB-X ORB-Y RND STEP CONJ VEL MODW onto ECC ANG SPR MOON DUST PIXEL PITCH CUT RES
  MODE SEND FDBK RAD PREC LEVEL, −99..+99.

## 6. FX

- **FILTER MAP**: a stereo state-variable filter on the master, its mode morphing LP → BP → HP, CUT, RES and DRIVE.
  Bypassed (costs nothing) when it is open and clean.
- **SPACE**: a stereo delay timed in beat ratios (ping-pong, feedback, tone) into **NEBULA**, a four-line feedback
  delay network with size, decay and damping. Each sound has its own send.
- The end: DC blocker, a cubic soft clip.

## 7. The screens

Deep indigo, violet frames with rounded corners, a lime title pill, the ring colours cyan, magenta, yellow, violet,
pixel stars. Top bar: the page, the system, the tempo and the sun. The bottom: K1–K4 as four boxes with knob arcs.

| Button | Page(s) | K1–K4 (SEL held: the second row) |
| --- | --- | --- |
| HOME | ORRERY → MACROS | hits A–D (rotation A–D) · CHAOS SPECTRAL GRAVITY ORBIT (FRACT on ALGORITHM) |
| SEQ | RING → TIME | RATIO LEN HITS ROT (DEG DIR PROB LOGIC) · BPM SWING MUTATE HUM (VAR, GATE …) |
| EDIT | ORBIT → BODY | ECC ANG SPR PREC (MOON RATIO DUST PIXEL) · FALL FALL-T COLOR LEVEL (PAN OCT SEND PHASE) |
| ENV | ENV | A D S R (ECC AMT, ECC A, ECC D, VEL) |
| LFO | MOD → LFO | SRC DST AMT (SELECT: the slot) · RATE SHAPE for LFO 1 and 2 |
| FX | FILTER MAP → SPACE | CUT RES MODE DRIVE · TIME FDBK SIZE MIX (TONE DECAY PING SEND) |
| GLO | ORBIT PAD | X Y RAD RATE |
| ARP | KEYS | MODE TUNING SUN OCT |

SELECT turns the focus (KEYS, A, B, C, D) on every page; PRESETS the system; ALGORITHM is the page's extra; PLAY starts
the orrery; REC writes the keys into the focused ring; SAVE stores the system (8 places of your own). On RING, the
white keys are the steps (OCT± the page of 16); a held step key and K1 set its degree.

## 8. Budget and rules

The audio ISR: 256 frames in 5.8 ms; target under half. Eight sub-blocks of 32 frames: events, the clock, the
modulation and the envelopes at 1378 Hz; per sample only phases, table reads, multiplies and adds.

- Per voice per sample: ~100 instructions (Kepler bilinear lookup, two sines, the moon's two, dust, projection).
  8 voices ~800, the master ~150. On the host: `DINGHY_PERF=1` (sloopDX's engines fit at 1000–1500).
- No float, no 64-bit division, no flash in the ISR (tables are `const` in XIP), no loop whose length a parameter
  sets. In a feedback loop round toward zero.
- Memory: the delay 2 x 32768 int16 (128 KB) and NEBULA (~40 KB) in the pool, the canvas 240 x 120.

## 9. Systems (built-in presets)

Named after real resonances in the sky:

| | Rings (speed) | Character |
| --- | --- | --- |
| HARMONICES | 1:1 5:4 3:2 7:4 | the flagship: 4:5:6:7 as rhythm and chord, soft planets |
| GALILEAN | 1:1 1:2 1:4 2:1 | Io, Europa, Ganymede's 1:2:4: the straight groove, kick, bass, pad, dust hats |
| TRAPPIST | 1:1 3:2 5:3 8:5 | a resonant chain: glassy plucks that mutate |
| KIRKWOOD | 1:3 1:1 5:6 2:3 | the asteroid gaps against Jupiter (3, 5/2, 2): pixel percussion |
| SPIRAL DRIFT | 1:1 3:2 5:4 7:5, 16 24 20 28 | the drawing's pattern: SPIRAL direction, PROB 74, MUTATE 42 |
| PULSAR | 1:1 1:1 2:1 1:2 | drums: an infall kick, dust snare on NOT, pixel hats |
| NEBULA | 1:3 1:2 2:3 3:4 | slow, UTONAL, moons, a big NEBULA |
| ROCHE | 5:4 4:3 7:5 3:2 | CHAOS: bells off whole ratios, tearing |
| LAGRANGE | 1:1 1:2 3:2 2:1 | HARMONIC keys over a calm orbit |
| ACCRETION | 1:2 1:1 3:4 3:2 | GRAVITY high: everything falls into tune |

## 10. Order of work

1. Tables (`tools/gen_tables.py`) and the engine (`firmware/src/orb_engine.h`), heard on the host
   (`tools/orb_render.c` → `build/audio/*.wav`), tested (`tests/orb_test.c`, again under ASan/UBSan).
2. The app (`firmware/src/orbitrix.c`): pages, controls, systems, storage, MIDI (notes, CC 1, clock and transport).
3. The FM-1 build (`./build.sh`), sizes, the host perf count. Then the FM-1 itself, with the user.
