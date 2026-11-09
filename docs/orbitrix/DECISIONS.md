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

## D5 — MUTATE in the main loop (2026-11-09; replaced by D14)

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

## D8 — Even more unorthodox, at times (2026-11-09)

The user, after v0.1: 75 % happy; the sound should be more unorthodox at times. Decided: four phenomena from the same
sky, mostly used at moments (modulated), so the instrument stays beautiful at rest:

- **TIDE** (FM between the voices): its sidebands stay on the sun's harmonic series because the rings are its ratios.
  A block (32 frames) late, from the voices' mono sum less each one's own share; the phase product is in uint32 (a
  phase wraps: no overflow to fear, and the index can grow into chaos when turned far up).
- **INSPIRAL**: a merger's chirp, one table (`ORB_CHIRP`, the 2-octave shape) scaled to any depth; it lands exactly
  (tested: 1.000 of the note), and the merge starts the eccentricity envelope.
- **BEAM**: AM by a narrow pulse at the moon's phase.
- **HORIZON**: granular redshift, stretch, freeze and RETRO on the mix, after SPACE and before the filter map.
  Triangle grains staggered by half (they sum to one; Hann cost 29 instructions more a grain); its R is the wet 151
  frames later (a second read a grain cost more).

Three systems show them: GW150914, ALPHA CEN, SGR A*. HARMONICES, SPIRAL DRIFT, ROCHE, PULSAR and NEBULA got a touch
each (CONJ → TIDE, BEAM on the snare, LFO 2 → HORIZON ...).

## D9 — The budget, again (2026-11-09)

With D8 the absolute worst case is ~60 % of the chip (counted, not measured). Rather than take features away, the
interrupt measures itself and sheds the quietest voice (a ~10 ms fade) when a callback takes over 80 % of its time.
Typical systems stay well under half.

## D10 — The cat in the splash (2026-11-09)

The user's drawing: a black cat as a planet in a violet coat with a yellow ring, stars round it. It was given in the
chat only, so `tools/gen_splash.py` redraws it on the drawing's own grid (~24 px a cell of 1254), from shapes measured
off it; given as `assets/cat_source.png`, the picture itself is sampled instead. On the splash it is 3x, with
plus-stars twinkling as in the drawing, three planets going round it (behind it on the far side), a glint along the
ring, a shooting star and a blink.

## D11 — The legend of Astralica (2026-11-09)

The user: "Big ass cat Astralica takes over the universe to make her cat music ring through the multiverses." Decided:
a story at power-on, before the splash, ~14 s, five scenes (the silent universe; then came Astralica; she grew, and
grew: one big ass cat; she ate the planets and made their orbits sing; now her song rings through the multiverses).
The words are the user's, the tone kept.

- It is scored by the engine on its own system (`orb_intro_system`): a drone on the keys, a chirp on A each time a
  planet goes in (INSPIRAL, timed to land as it vanishes), HORIZON rising as the zoom streams the stars, the orrery's
  B C D, then the keys turn into her meow.
- Two engine calls came of it, useful beyond the intro: `orb_part_on` (a ring's sound played directly, on its voice)
  and `orb_glide` (a held key's note moves to another pitch on gravity's spring). The meow is three of those: mi-, AA,
  ow (G5 -> B5 -> E5), with the eccentricity envelope opening and closing it, the ping-pong delay sending it on.
- PLAY skips it (D12); it can be switched off (KEYS, SEL row: INTRO, kept in the settings). On the host it
  plays only with ORB_INTRO=1 (the scripts are timed from the splash); `tools/make_film.sh` films it.

## D12 — The glitches, and PRESS PLAY TO SKIP (2026-11-09)

The user saw a couple of glitches in the legend's film, and wanted "press play to skip". Found and fixed:

- **Pictures:** the planets blinked (the test of behind and in front of her was the wrong way round: those behind
  vanished, those in front were drawn twice); her size jumped where the zoom turned back (2232 -> 1800) and her anchor
  jumped twice; the stars changed pattern when the streaks began; the cats in the bubbles were drawn at under a pixel a
  cell, so cells fell out (now each pixel takes its cell). Every change of scene is now continuous (checked frame by
  frame).
- **Sound:** a RETRO grain in HORIZON could lap the write head at the longest lag and read fresh sound (two clicks at
  8.0 s; its upper lag now counts the grain going back); HORIZON's mix jumped when its depth did (it glides now, ~10
  ms); NEBULA's lines jumped to a new size (they glide, a frame a tick); the drone changed timbre in its release when the
  keys became the meow (it goes earlier now); the story's echoes changed pitch when the system to play came in (it comes
  after the splash, and the echoes die out during it). In the engine at large: the moon's radius, the dust, the beam,
  the tide and the projection's angles changed in steps a tick (RND -> MOON, CONJ -> TIDE clicked): they ramp over the
  tick's frames now, as the amplitude did.
- **A click detector** in `tools/orb_look.py`: a lone spike in the second difference (4 times anything else within
  20 ms: a sharp waveform, high ECC or PIXEL, has an edge every period and is not lone), where the level does not rise
  (not an attack). The legend and its skip: none. What it still flags in the systems (a few in 16 s) are a ring
  retriggering its own sounding voice (a 1 ms re-attack): left as a pluck's nature.
- **PRESS PLAY TO SKIP** at the top while it plays, PLAY blinking; only PLAY skips (a key pressed by chance does not),
  and it skips the splash too, straight to the instrument.

## D13 — The page, with ORBITRIX to play on it (2026-11-09)

The user: a page, with a version on it that can be tried. Decided, as RoboSeq's and FM-1 Clock's pages do: the
firmware itself, `orbitrix.c` unchanged, compiled to WebAssembly on a web FM-1 (`web/app/web_host.c`, no C library),
its sound made in an AudioWorklet (`worklet.js`), whose quanta run the main loop for the milliseconds they are worth:
the sound is the time base, as the audio interrupt is on the FM-1.

- For that the app runs as passes: `app_start`, `boot_pass` (the story, the splash), `app_pass` (the main loop's
  body). On the FM-1 and the host they run in a loop, as before (the screens checked pixel for pixel: unchanged but
  for PLAY now skipping the splash); on the page `app_main` is one pass a call.
- The web FM-1 adds to RoboSeq's: buttons held and let go (SEL is a layer), the keys' two light layers (lit, and the
  sun's glow), MIDI straight into the firmware (velocity, CC 1, a clock), and the store: the settings and U1..U8,
  kept in the browser. On the page SEL latches (a mouse cannot hold it and turn a knob), and Shift holds it.
- The engine's memory is plain memory there (no `.pool` section: its zeros would be in the file): 313 KB, 109 KB as
  served.
- The page: Astralica at the top, the FM-1 to play, the legend's film, the thirteen systems with their spectra, how
  it works, the installer (SLOOP's protocol, as the other pages carry it). Tried in headless Chromium: Start, the
  legend plays, PLAY skips it, PLAY runs the orrery, a key lights, PRESETS turns the system; no errors.

## D14 — MUTATE varies the pattern, it never writes it (2026-11-09)

The user: "mange av parameterne resetter seg ved endt sekvens" (many of the parameters reset at the end of the
sequence). Traced on the host, every change to the system pass by pass, while it played, stopped and started again: no
knob's value changes by itself; the one thing that writes the system at the end of a lap was MUTATE (D5). It rewrote
the ring's steps and degrees there, on top of its own last changes, so what was programmed (steps tapped in, a step's
degree, REC) wandered off lap by lap, into what was saved too, and a degree set outside −7..14 was pulled back inside.

Decided: MUTATE plays a variation of the pattern, and never writes it. At each ring's lap the engine draws the next
lap's variation afresh from the pattern as written (as many changes as before: 1 to a quarter of the ring; a hit to a
free neighbour, a degree by up to 1 + VAR/25, within the degrees' whole −14..21), into the ring's state, not the system.
The pattern is what is seen and edited on RING, and saved; the orrery shows what the lap plays, MUTATE's steps hollow. A
lap after PLAY, and the rest of a lap after an edit to that ring (a step, its degree, LEN HITS ROT, REC, a system),
play the pattern as written. MUTATE 0 is the pattern exactly, every lap. The draws run in the interrupt at the lap (16
at most, a few hundred instructions a lap) with their own random state, so the rest's draws are left as they were.

Tested: 76 laps at MUTATE 99 (`tests/orb_test.c`): every lap plays as many hits as are written, nothing past the end, a
degree at most 1 + VAR/25 away, and the system byte for byte as it was. On the host and the page: steps tapped in, a
degree, 15 to 25 s of playing, STOP and PLAY: the pattern as it was. (`tests/scripts/edit.txt` began in the splash
since D12's skip: it is 0.3 s later now, and edits the RING page again.)

What still comes back at a lap by design: a modulation from STEP (a ramp over ring A's lap, TRAPPIST's ANG) and a
synced SAW LFO (GW150914's HORIZON) fall back to their start there.

## D15 — Music in a minute: STEPS, SOUND, IDEA, and Astralica at home (2026-11-09)

The user: "we're onto something, but I wish I could make music with this. It is hard to navigate, and I wish for
something cute, since the cat is very cute. A deep round for musicality; change the interface if we must; take from
the drawings. The firmwares that score best are the ones you can quickly make something nice with, as the best
grooveboxes." What stood in the way: fifteen pages under physics names; the focus on a turned knob; no way to pick a
sound but to build it from ECC and SLING; steps sixteen at a time with nothing for the black keys; nothing to undo; a
first system of quintuplets against septuplets.

Decided, the engine and its strangeness kept whole, a friendly way in on top:

- **KITTEN**, system 0 (first at power-on, and after the legend): a beat to start from, on the grid: a kick, a clap's
  backbeat, hats in 8ths, a purring bass on the root, its fifth and 7/4, the keys a soft paw; nothing mutates. The
  other thirteen follow (system numbers +1; the settings got a version byte, so an older layout is not misread).
- **The library**: 25 sounds by name (BOOM THUMP TOM PAT CLAP TICK TSSS ZAP · PURR GROWL SUB SWOOP · PLUCK PAW GLASS
  BELL STAR TIDAL · PIXEL LEAD FLUTE MEOW · CLOUD DRONE COMET), in nine kinds, each with an icon. Rendered one by one
  (`orb_render lib`, `build/audio/lib/`) and balanced: peaks −5 to −19 dBFS, drums by nature lower in RMS; the hats were
  buried (−25 and −34) and went up, also in the systems that use them. Each has a register (the octave as a ring plays
  it); a sound keeps its own level when picked, the track keeps its pan. A sound knows its number (`orb_snd_t.lib`, a
  spare byte), so the screens can name it and IDEA knows what it is.
- **SOUND**, EDIT's first page (then ORBIT, BODY, COSMOS: from the friendly to the deep): K1 the sound by name (heard at
  once when stopped), TONE (ECC), DECAY, LEVEL; SEL: OCT, PAN, SPACE, WOBBLE (the moon).
- **The keys play the focus** (the chip, now on every page): K is the keys' own sound, A–D that track's, played on the
  keys' four voices (a note-on with a part: the sequence goes on, untouched, on the rings' own) and two octaves down,
  where its ring plays it. REC writes the note heard (from the pitch played, not the key, so HARMONIC keys and MIDI
  record right too); REC with the keys in focus records into A.
- **STEPS** (was RING): the white keys the steps, as before; the black ones each with a job, drawn over its key on the
  screen: the lower F# G# A# C# pick track A–D, D# an **IDEA**, the upper four mute A–D (lit while it plays, out a
  moment at each hit), D# **CLEAR**, the top F# **UNDO**. The hits are paws.
- **IDEA**: a pattern as the track's kind of sound wants it, its LEN and SPEED kept: a kick on the beat (4, 3 or 5 a
  bar), a snare's backbeat (and now and then a ghost before it), hats in 8ths, offbeats or 16ths, accents between, a
  bass mostly on the root (its octave, now and then another tone), a melody walking the chord's tones (the tuning's
  consonant degrees: the major pentatonic in JUST and ET, harmonics 8 10 12 14 in OTONAL ...), a pad's few long notes.
- **UNDO**, a level per ring, a gesture at a time (changes under 1.5 s apart: a run of knob detents is one), for IDEA,
  CLEAR, LEN HITS ROT on any page, taps, a step's note, REC. UNDO again is REDO.
- **HOME**: Astralica is the sun. She bobs on each beat, blinks, her eyes light where the orbits meet, and stopped she
  naps (zz). The lanes name each track's sound, its speed and level; a muted track naps too. K1–K4 the hits of A–D, SEL
  their levels (a mixer; the rotations are on STEPS). Her head sits by the sun's note in the top bar.
- **Where am I**: a button's pages show on the hint line as it is pressed, this one lit; a message shows at once.

Engine changes, small: the note-on with a part; the library tag; a chirp grows from a third only when it starts from
silence (a ringing voice kept its level). PAW lost SF_PHASE (a chord retriggering a ringing voice clicked). Measured:
the systems unchanged but for the hats' level (HARMONICES bit for bit); the bench unchanged. The click detector's
findings are as before (a ring's re-attacks, chord changes on the keys); KITTEN's two are a −64 dBFS kink as a key
voice's decay reaches its floor; SWOOP retriggered every 8th note swoops each time (its nature; it is for slow lines).
Tests: `test_library` (every sound loads with its register; the keys play a track on their own voices, the ring's left
alone); `tests/scripts/groove.txt` (KITTEN made into a groove); `tests/scripts/readme.txt` and `tools/readme_screens.sh`
(the README's screens).

## D16 — Astronomical cuteness (2026-11-09)

The user: "astronomical cuteness is what I want now". Decided: cute everywhere, without a control moving or a page
getting harder to read; the cat's world, not stickers on the instrument.

- **Astralica alive** (HOME): she bobs on the beat and blinks, as before; now a note rises from her each bar, her eyes
  close happily (^ ^: the eye's top row) at an IDEA, a SAVE, a system loaded and PLAY, she sings along in a bubble when
  the keys play (mew, nya, mrrp, prr, in turn), and she naps (zz) when stopped. Where the orbits meet, a twinkle.
- **Her kittens**: the planets are kitten heads in their tracks' colours (HOME, ORBIT, LFO, ORBIT PAD, COSMOS round her;
  TIME's playhead carries one). An accent is a heart (HOME, STEPS), a hit on STEPS a paw, and hearts rise from the
  track when a step is set or an IDEA comes. The little Astralica sits at the ORBIT's focus and in COSMOS's middle.
- **Ears and paws**: the title pill and the focus chip wear cat's ears, every gauge ends in a paw, the keys sounding on
  KEYS show a paw. The macros: CHAOS a ball of yarn (curved strands turning, its thread loose), SPECTRAL a rainbow,
  FRACT paws walking up, TIDE a fish bobbing on the tide, HORIZON a black hole with ears (and on its page the hole
  itself is cat-shaped), ORBIT a kitten going round, MUTATE a strand of yarn.
- **Words**: "AN IDEA, nya~", "NAPS zz" / "IS AWAKE", "ALL CLEAN", "SAVED, mrrp!", "AN EMPTY BOX".
- **Sounds**: BOOP (a soft blip falling onto its note), SQUEAK (a toy's chirp up), BLOOP (a bubble, from far under),
  TOY (a toy piano's tine and clang), MEW (her voice in short: it falls from above as her mouth opens and closes); the
  library is 30, rendered and balanced (peaks −8 to −14 dBFS for these), none with a click.
- **MILKY WAY**, system 1: a lullaby. A music box (STAR) twinkling at 3:2 over pitched boops, a soft kick on 1 and 3, a
  purring bass at half speed, PENTA (nothing can be wrong), swing 22, a long ping-pong echo and a wide nebula, CONJ →
  MOON; the keys mew (at a lower level: a chord of meows adds up). The sky's systems move up one; the settings' version
  is 3.

Measured: the engine unchanged (the tests' figures as before, the bench within its noise); MILKY WAY peaks −1.5 dBFS,
RMS −22; its one click finding is a note's onset inside the loud mix (−49 dBFS, no track alone has it).
