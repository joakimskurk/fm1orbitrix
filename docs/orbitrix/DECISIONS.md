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

## D17 — The legend, continued: her tummy, and the ORBITRIX (2026-11-09)

The user: "She ate all the planets and transformed them in her tummy, and when she pooped them out, they formed the
orbitrix." Decided: two scenes in place of "she ate the planets and made their orbits sing", the words the user's:

- **Four planets** now (the tracks' colours, cyan magenta yellow violet), eaten at 5.2, 5.9, 6.6 and 7.3 s, each with
  its chirp (1/1, 3/2, the octave, 7/4).
- **"SHE ATE ALL THE PLANETS / IN HER TUMMY THEY CHANGED"** (8.2 s): she zooms back out as before; her tummy glows (a
  dithered rainbow), the four turn in it and become twinkles, she wobbles with rumbles either side, her eyes content
  (^ ^); her tummy gurgles: four bubbles (BLOOP) on A, low.
- **"AND WHEN SHE POOPED THEM OUT / THEY FORMED THE ORBITRIX"** (10.2 s): plop, plop, plop, plop, 0.4 s apart: a
  little cosmic swirl comes out under her in each track's colour (eyes, a shine, a blush; she gives a push), plop! and
  a puff of stardust, and it turns into a kitten that hops out to its ring. Each ring grows from its kitten as it
  arrives, and sings: the orrery starts with the first, B C D join as theirs arrive. The plops are BOOP on A, each its
  ring's degree (the 4:5:6:7). "THE ORBITRIX" in lime.
- Then the multiverses as before. 17.2 s in all (was 14.4); PLAY skips it, as ever.

The kittens are the ones that go round her on HOME (D16): the story says where they came from. Measured: the legend
has no click (`tools/orb_look.py`), its scenes −2.4 to −6 dBFS peak; the film (`tools/make_film.sh`) and the README's
strip (`tests/scripts/legend.txt`) are remade.

## D18 — The legend is the page's (2026-11-09)

The user: "I don't think we should have this as the splash, but as an animation on the git page", with a transparent
background. Decided:

- **The FM-1 starts with the splash** and goes to the instrument (PLAY skips the splash, as before). The legend's code
  is compiled on the host only (`ORB_LEGEND`), so it costs the FM-1 nothing (and the page's FM-1 27 KB less); the
  INTRO setting went with it (the settings' version is 4: an older one is read as new).
- **The page shows it** as an animation: `tools/make_legend.py` builds the host firmware, plays the legend
  (ORB_INTRO=1) and takes its 430 frames at 25 a second, makes the night transparent (the colour everywhere at the
  start: the stars, Astralica, the kittens and the words stay), and writes a lossless looping WebP (306 KB, 240 x 240,
  scaled pixel for pixel by the page), a still, and its sound (AAC). PRESS PLAY TO SKIP went (it was the FM-1's), and
  the line over the words. It loops silent; "From the start, with its sound" restarts it with its sound.
- CI makes it in place of the film (`make_film.sh` stays, for films of the host).

## D19 — Under it all: the background radiation, a nebula that blooms, tired light (2026-11-09)

The user: it is not astral yet; a huuuge pad must lie under it, not clashing with itself, fed extra into the reverb,
which must be longer (to save CPU, its wet fed back into itself), and low-cut. Then: the pad is rather "background
radiation"; bring in astral ideas, vague theories too; make redshift into an FX; read up. Read: the CMB's acoustic peaks
form a harmonic series; baryons make the odd peaks taller (a "baryometer"); photon diffusion damps the high ones (Silk
damping); its temperature is 2.725 K with fluctuations about one in 100 000; sonifications of it fall in pitch as the
universe expands (Cramer's "Sound of the Big Bang"); Zwicky's tired light (1929) had light lose energy with distance;
the CMB cold spot and the "axis of evil" are its unexplained anomalies. Decided:

- **BACKGROUND RADIATION** (GLO's first page): eight partials on the sun's harmonic series (1..8; PENTA and ET without
  the 7th; UTONAL on the sun's third below: 1, 4/3, 2, 8/3, 4, 16/3), so it cannot beat with itself (tested: all its
  energy on the harmonics, its fluctuations on). SILK, BARYON, ANISO, TEMP (an octave each way, shown in kelvin: Wien's
  law), LOW CUT (each partial's own second-order response, no filter a frame; and a pole on NEBULA's input), GLOW into
  NEBULA. It swells in on PLAY (~1.5 s), fades on STOP, glides when the sun moves (~0.4 s). Made at NEBULA's half rate,
  plain table sines (a pad has no harmonics of its own to alias); its shape's divisions one partial a tick. The page
  draws a map of the first light (warm and cold, drifting, its grain), Astralica at its middle, the peaks below.
- **NEBULA longer**: BLOOM lifts its lines' feedback towards 0.985 and puts 0.01 of its wet back through the
  diffusers (denser); the two never pass 0.995 together (stable whatever the rest: tested at everything full, 117 dB
  down 50 s later, never past full scale). And the lines run twice as hot, the gain rounded once (it was twice, two bits
  lost), with a soft knee above three quarters: the tail used to be cut ~58 dB down by the rounding, now its last 20 dB
  are heard, in every system (their levels as before). A moved sun clears it a moment (its gain to 0.55, back in 0.6 s)
  when the background or BLOOM is on, so the old chord's tail never clashes with the new.
- **TIRED LIGHT** (HORIZON, second row): the echo's feedback read slower than it was written (two taps a 46 ms window
  apart, triangles), so each echo is a just interval lower than the last (15/16 .. 1/2, shown as z): tested, the second
  echo of an A4 at 0.674 of the first (2/3). Through the echo's tone it also dims.
- **AFTERGLOW**, the sixteenth system, the last of the sky's: all of it. MILKY WAY has the background radiation under it.
- The cost, all on (x86, callgrind -Os): ~+260 instructions a frame; nothing while off. The FM-1's image is 192 KB.
- Kept for later, from the reading: the cold spot, the axis of evil (an alignment where none should be), Hawking
  radiation (a black hole's glow, hotter as it shrinks), dark matter's flat rotation curves, the gravitational-wave
  background pulsars hear, lensing's time delays (the Einstein cross's images hours apart).

## D20 — The background radiation, ambient (2026-11-09)

The user, hearing D19's pad: "an incredibly nasal drone; I imagined something much more ambient." It was: eight steady
sines on harmonics 1..8 crowd the thirds into the middle octave (a formant: nasal), BARYON lifted the odd ones (hollow,
a clarinet's), and nothing moved. Decided (the user: "bring in ideas you have"):

- **An open chord** on one harmonic series (of the sun two octaves down: 2 3 4 6 8 9 12 16, C2 G2 C3 G3 C4 D4 G4 C5 on
  C; UTONAL of the sun's third below): fifths, octaves and a ninth over four octaves, no crowded thirds; still nothing in
  it can clash (tested: 0.999 of its energy on the chord, its drift on). A gentle hump, the middle voices fullest; SILK
  softer to begin with.
- **DRIFT** (was ANISO): each voice breathes on its own (a new depth every ~3 s, one voice in turn, ~1.5 s to get
  there), at 99 coming and going entirely, so the chord turns inside itself; and each wanders a few cents (+-3.4 at 99,
  slowly), a shimmer, not a beat you notice.
- **HISS** (in BARYON's place): the static the background was first heard as, in Penzias and Wilson's antenna (1965):
  noise between ~250 Hz and ~1.5 kHz, a different one each side, under its level.
- **Slower**: ~3 s in, ~3 s out. And an idea of my own: **cooling**: swelling in from silence it starts an octave up
  and comes down onto its note in ~2 s (the first light redshifting as the universe cools).
- MILKY WAY and AFTERGLOW: less of it dry, more into the nebula, more DRIFT, a little HISS.

## D21 — The background radiation as chords (2026-11-09)

The user, hearing D20: "We cannot have that kind of glide, and it sounds almost like an FM, since it resonates in
itself. I imagine a well-balanced carpet of beautiful chords that can lie behind when it fits." It did glide (the
cooling, the sun's moves, the drift's cents), and eight sines on one harmonic series fuse into one tone whose partials
beat and ring in the nebula: an FM's colour. Decided:

- **Chords**, not a series: eight progressions (SUN, TIDES, ORBIT, DUSK, COMET, ECLIPSE, LULLABY, VOYAGE), each written
  for each tuning in its own notes, so the rings never meet a note outside their scale: JUST pure chords (add9, m7,
  maj7, m9, 6, the borrowed bVImaj7 and iv(add9) in ECLIPSE; never D with A or F, the scale's wolves), OTONAL pieces of
  the sun's harmonic series, UTONAL of its subharmonics (F minor under C, D-flat, B-flat minor), PENTA its five notes
  only (and no D with A there either), ET the JUST progressions tempered. tools/gen_tables.py checks every chord: in
  the tuning, every interval in it of small numbers (both ways under 17), and fails the build otherwise.
- **Voiced and led ahead of time** (gen_tables.py, not the chip): a bass about the sun (over the rings' bass, not under
  it), up to seven voices over it, open (a fourth or more above the bass, no third under E3, no second under G4, no
  minor ninth anywhere), the root and fifth doubled rather than the third; then each progression's cycle with the least
  movement (Viterbi from every first voicing), a held note costing nothing. The engine reads eight pitches a chord.
- **No glide, ever**: a voice whose note changes fades out (~0.2 s), takes the new note when it is silent and swells in
  (~0.7 s); the lowest first, each ~20 ms after the last, so the chord blooms upwards; a note the next chord shares is
  held. Tested: in every tuning, VOYAGE a chord a bar: a note changes only in silence, a sounding voice's steps never
  move, a held note never dips, every chord on its bar (an eighth early, so the old notes have left as it comes).
- **A voice**: two oscillators a few cents apart (+-4 at WIDTH 99), A left and B right as WIDTH has it; a soft saw (8
  harmonics, 4 above ~1.3 kHz, so nothing folds over at the half rate), two poles of SILK, the low cut. No hiss, no
  drift, no cooling. CHANGE (1, 2, 4, 8 bars) and WIDTH take HISS's and DRIFT's places; BLOOM moves to ALGORITHM.
- **When it fits**: SEL + GLO takes it out (it fades) and brings it back, its settings kept (bg_out, saved with the
  system). MILKY WAY: ORBIT, two bars a chord, a little more of it than before (LEVEL 45), less into the nebula
  (GLOW 45, BLOOM 30); AFTERGLOW: ORBIT on the overtone series (maj9, m7, 6, h9), BLOOM 55; KITTEN: SUN (its bass sings
  7/4, which only the sun's own chord takes). Every other system: LEVEL 0, ORBIT ready when it is turned up.
- The page shows the chord (its root large, its kind, its ratios: 4:5:6:9, u12:10:8:7 when subharmonic is shorter),
  the progression with the chord lit, and each voice as a star where its note is.
- The cost, all on (x86, callgrind -Os): ~+250 instructions a frame (D19: ~+260); nothing while off.

## D22 — HALO: the chords all wet (2026-11-09)

The user, hearing D21: "Fine now, but shouldn't they be 100 % wet in a reverb or something? I miss the big floating
soundscape." The chords were dry, with a little of them into NEBULA, which is the rings' space and sized for them.
Decided:

- **HALO**, the background radiation's own space (a galaxy's halo: vast, faint, all round it), only for it, all wet:
  no dry chord at all. Four allpasses (10 .. 31 ms) and four lines (primes, 161 .. 273 ms) in a Hadamard, at a quarter
  of the rate (11 kHz: the chords are soft already, and SILK before it), each line damped at 3 kHz (its tail darkens as
  it goes), two of them swimming (+-5 frames, ~0.12 and ~0.17 Hz: the tail never stands still, never rings). A pole at
  4 kHz before the rate goes down and two after it comes up: its images ~61 dB down. HALO 1..99: 3 s .. 40 s to
  -60 dB (gen_tables.py, each line's feedback), its output's gain set so it is as loud whatever its length (tested:
  as loud as the dry chords, its correlation with them 0.02: all wet). HALO 0: dry, as before (dry <-> wet glides in
  ~50 ms). A moved sun clears it a moment, as NEBULA.
- **It ends**: its one-poles are written so they land on their input exactly (orb_pole: the distance shrinks, rounded
  toward zero), so the tail gets to silence and the halo stops (tested: HALO 65 silent 19 s after STOP; before this,
  the poles stuck a bit away and it hummed at -90 dB for ever).
- HALO takes TEMP's knob (TEMP to the second row), GLOW goes to ALGORITHM, BLOOM to HORIZON's second row beside TIRED
  (both: the strange space). MILKY WAY HALO 65 (~16 s), AFTERGLOW 80 (~24 s), every other system 70 when its chords
  are turned up; GLOW lower (20, 30): the halo is their space now. The page draws a violet shell about the map, as full
  as the halo rings.
- Memory: 28 KB of the pool (331 776 B of 344 064). The cost (x86, callgrind -Os): HALO ~+60 instructions a frame;
  everything cosmic on ~+340; nothing while off.

## D23 — Astral cuteness: four systems, little songs, a header (2026-11-09)

The user: "Can we make some better-built patches and demos that are astral cuteness at its best? And a more fitting
header for the preview on the git landing page." Decided:

- **Four systems by hand**, the last four (16..19, so the others keep their places; SET_VER 5: the places of your own
  moved, the settings start again): PURRSEC (a cat cafe at 84, swung, JUST: the royal road), STARDUST (a nursery of
  stars at 100, OTONAL), MOON NAP (a lullaby at 66, UTONAL, no drums, a 30 s halo), ZOOMIES (a kitten's mad minute at
  124, JUST, a chord a bar). In each the bass **walks the chords' roots**: a ring as long as the progression (8 bars at
  1:2, 4 bars at 1:1, 8 at 1:4) whose sections are the chords, so the bass and the chords change together on the bar;
  the melodies keep to **the notes every chord of the progression holds** (STARDUST: 9/8 5/4 3/2 of the series; MOON
  NAP: the 4/3 and 8/5 under the sun), so nothing clashes whatever comes when. No MUTATE (it steps degrees off the
  chords): the life is in the probabilities, the moons where the orbits meet, the halo. Their basses without legato
  (a slide between roots is a glide). Written with orb_ring_pat (the steps as a string), levels set by measuring each
  ring alone (tools/orb_render.c: ORB_NOBG, ORB_SOLO).
- **Little songs** (orb_render song N): the system's parts by kind enter one after another: its chords alone in their
  halo, the bass, the beat, the melodies; a break (the beat and the bass out) where the keys sing the chords' own notes
  (a two-bar phrase, each note the chord's at that moment); all of it again; the beat out; STOP, and the tail to
  silence. For the six with chords (MILKY WAY, AFTERGLOW and the four), build/audio/songs, and on the page above the
  systems. On the page the systems are twenty.
- **The header**: tools/make_header.py draws Astralica on her night with her four kittens on their dotted rings (A..D's
  colours), the title in its lime pill with cat ears, what it is and "astral cuteness", in the firmware's own fonts
  (read from felucca_font.h) and colours, in pixels doubled: docs/readme/header.png (1280 x 640: the README's top, and
  the size GitHub's social preview wants), and the page's link preview at 1200 x 630 from the same drawing.

## D24 — A set of its own kinds, FOLLOW, CUTE (2026-11-09)

The user: "STARDUST has a lot of disharmony, and the rest all sound alike; I want a better set of patches, and to make
cute things more intuitively." Asked, they chose a whole new set (KITTEN kept, the strangest of the sky's out or made
musical) and all four tools offered. Decided:

- **The disharmony**: STARDUST's progression held 15/8 and 7/4 (B and B-flat over C) in chords one after the other,
  and the halo holds each chord under the next: a semitone for twenty seconds. The same in other OTONAL and UTONAL
  progressions (8/7 against 16/15) and JUST's ECLIPSE and VOYAGE (the borrowed chords). Now every progression keeps to
  one scale's notes, and gen_tables.py checks it (a progression's notes together must never crowd: no gap under 90
  cents, never two under 150 in a row) and fails the build otherwise.
- **FOLLOW** (a ring's flag, STEPS' ALGORITHM): its degrees are the notes of the chord of the bar, not the tuning's:
  #0 the chord's bass (within six semitones of the sun), up through its notes (not its added ninth: so on any chord #1
  is the third, #2 the fifth), an octave a round. The chord is the background radiation's even when it is silent.
  MUTATE steps a FOLLOW ring along the chord. Turned on or off, a ring's notes go to the nearest of the other kind.
  Tested: in every tuning, a chord a bar, MUTATE on, every note fired is in the chord of its bar.
- **CHORD keys** (KM_CHORD): the white keys the chord's notes going up from C4, a black key the white one under it an
  octave up; tested in every tuning: every key in its chord. The KEYS page names them, the bass lit; the LEDs glow on
  the bass.
- **IDEA** writes FOLLOW patterns on pitched rings (the bass on #0 and its neighbours, a melody walking #1 .. #6).
- **CUTE** (HOME's third page; SEL+HOME): a whole patch in a mood, from banks written by hand: each mood's roles (a
  kick, a snare, a melody, a sparkle, a counter, a bass), their sounds to pick from, its tunings (only the consonant
  ones), chords, tempos, swing, space, halo; every pitched ring FOLLOWs, the keys CHORD. **MOOD** turns the patch playing
  to one (tempo, sounds by kind, space, chords; the patterns and their places kept). Tested: 24 made in the four moods,
  every note in its chord, none too quiet or over. The page draws her in the mood (asleep with z's, hearts, her eyes
  aglow among stars, her kittens racing).
- **The set**, sixteen, each its own genre, tempo, sounds and space: KITTEN, MILKY WAY, HARMONICES, PURRSEC (lo-fi),
  ZOOMIES (house), CATWALK (disco), SPIRAL (generative), LUNA NOVA (bossa nova), SGR A* (dub, one drop; its old slide
  into the horizon taken out: a glide), BOOP BOP (chiptune, dry), SNOWGLOBE (a music box in 12/8: rings at 3:4, twelve
  steps a bar), STARDUST (synth pop), ALPHA CEN (a gamelan: interlocking bells, TIDE their shimmer), MOON NAP,
  AFTERGLOW, PULSAR (minimal techno). Out: GALILEAN, TRAPPIST, KIRKWOOD, NEBULA, ROCHE, LAGRANGE, ACCRETION, GW150914
  (glides, chaos, or a twin of another). Every melody's register checked (the highest a star at E6 .. F7 in the music
  boxes), every part's level measured alone. SET_VER 6.
