# Working on ORBITRIX

- Bare-metal FM-1 firmware on Dinghy 0.5. The design is `docs/orbitrix/PLAN.md`; what was decided, and why, is
  `docs/orbitrix/DECISIONS.md` (add to it, dated). The user's brief: deeply unorthodox, sounding as unorthodox, and
  harmonically beautiful. The look is the user's two ORBITRIX drawings: deep indigo, violet frames with cut corners, a
  lime title pill, the ring colours cyan, magenta, yellow, violet (A–D), lime for KEYS, pixel stars.
- The code: `firmware/src/orb_engine.h` (the engine, no platform, shared with the host tools), `orb_systems.h` (the
  sound library, 30 sounds by name, and the sixteen systems: KITTEN and MILKY WAY, to start from, first), `orbitrix.c` (the app), `orb_splash.h` (the cat, from `tools/gen_splash.py`: never edit it).
  The laws live in `tools/gen_tables.py`: change them there and rerun it, never edit `orb_tables.h`.
- After every engine change: `tests/run_orb.sh` (tests, the same under ASan/UBSan, renders, spectrograms, bench). After
  an app change: `tests/run_host.sh tests/scripts/pages.txt`, `edit.txt`, `cosmos.txt`, `splash.txt` and `groove.txt`
  (KITTEN made into a groove: STEPS' black keys, IDEA, UNDO, SOUND, REC), and look at the screens;
  `tools/readme_screens.sh` remakes the README's. Then `./build.sh` (stops at a `FAIL` line: never read only its `ok` lines) and, with the packages built (SMK-37 first,
  the FM-1 last), `tests/run_tests.sh`.
- The legend of Astralica (`intro_*()` in orbitrix.c, its system `orb_intro_system`) is the page's, not the FM-1's: it
  is compiled on the host only (`ORB_LEGEND`), plays with `ORB_INTRO=1`, and `tools/make_legend.py` films it for the
  page as an animation on a transparent night with its sound (`build/film/legend.webp`, `.m4a`); `tools/make_film.sh`
  makes an mp4 of it. The FM-1 and the page's FM-1 start with the splash.
- Hear it and see it: the user listens to `build/audio/`. What can be measured, measure it (`tools/orb_look.py`:
  spectrogram, centroid, energy above 16 kHz, the largest jump between frames, clicks: keep the legend and the
  systems at none but a ring's own re-attacks).
- The budget (docs/RULES.md): the audio ISR under half of its 5.8 ms. The voice loop on the chip is ~110
  instructions a frame plain, ~310 with SLING, MOON, DUST, PIXEL, TIDE, BEAM and the send; the FX ~700 a frame with
  the filter, NEBULA and HORIZON on, ~+250 (x86, callgrind) with the background radiation, BLOOM, the low cut and TIRED
  LIGHT all on as well (nothing while they are off). Over 80 % the interrupt sheds the quietest voice (`orb_shed`). Count with
  callgrind on the host (`-Os`) or on the chip's own code (JieLi clang `-Os -c`, objdump). The app shows `CPU nn%` on
  the hint line.
- Audio code: no float, no 64-bit division, no flash access in `orb_render`, no loop whose length a parameter sets,
  fixed point in range (UBSan finds a signed overflow, and it is a bug on the FM-1 too). In a feedback loop round toward
  zero (`orb_rz`). A constant that looks like a hardware address (0x02000000, 0x1Exxxxx ...) fails the build's
  register check: write it as a shift.
- Ramps a frame are built so they never pass their target (`orb_rz(... , 5)`): no clamp a frame.
- The page (`web/`, published by CI from `main` to GitHub Pages): the firmware compiled to WebAssembly
  (`web/app/build.sh`, `web_host.c`), `worklet.js`, `play.js`, `index.html`, `make_site.py`. The app runs as passes
  (`app_start`, `boot_pass`, `app_pass`): keep every loop in it a pass, never a wait. Try the page in Chromium
  (Playwright, headless) after a change: Start, PLAY skips the splash, the screen and the lights.
- Only edit Dinghy's own files (`platform.c`, `gfx.c`, `hal/`, `loader/`, `tools/build.py`) when the task needs it,
  and list the departure in README.md.
- Name no product or other firmware as a model or an inspiration, anywhere in the repository. Dinghy, Felucca, SLOOP
  and the SMK-37 layer stay credited: that is the code's licence.
- Never install on an FM-1, push, or tag a release without asking.
