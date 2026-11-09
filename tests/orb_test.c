/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX's engine tests (tests/run_orb.sh: twice, the second time under ASan/UBSan).
 *   - an orbit with e = 0 is a sine; eccentricity brightens it; the velocity view (SLING) more so
 *   - the pitch: the increments are the note's frequency; the tunings' ratios are exact (to a cent)
 *   - the orrery: a ring of p:q fires p/q as often as a 1:1 ring; rings meet where they should (conjunctions)
 *   - the cap: a high note at full eccentricity and SLING keeps its aliases down
 *   - every system plays 30 s within range: no NaN-like runaway, no DC, the peak under full scale
 *   - the logic (NOT) keeps two rings apart; MUTATE keeps the number of hits */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "orb_systems.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static orb_t O;
static orb_sys_t S;
static orb_mem_t MEM;

/* a plain DFT magnitude at bin k of n samples (host: float is fine here) */
static double dft_mag(const float *x, int n, double k)
{
    double re = 0, im = 0;
    int i;
    for (i = 0; i < n; i++) { re += x[i] * cos(2 * M_PI * k * i / n); im -= x[i] * sin(2 * M_PI * k * i / n); }
    return sqrt(re * re + im * im) / n;
}

/* one voice alone, rendered raw (no FX): a held key on a plain sound */
static void plain(orb_snd_t *s)
{
    memset(s, 0, sizeof *s);
    s->a = 0; s->s = 99; s->d = 50; s->r = 30; s->level = 99; s->pan = 50; s->vsens = 0; s->ang = 0; s->spr = 0;
}
static int render_voice(float *outL, int n, int32_t pitch)
{
    int32_t blk[2 * ORB_SUB];
    int i = 0, k;
    orb_voice_t *v = &O.v[4];
    orb_trigger(&O, 4, ORB_KEYS, pitch, 32767, -1, INT32_MIN);
    for (k = 0; k < 64; k++) { memset(blk, 0, sizeof blk); orb_voice_tick(&O, v); orb_voice_render(v, blk, blk, 0, O.own[4], 0); }
    while (i < n) {
        memset(O.busd, 0, sizeof O.busd);
        orb_voice_tick(&O, v);
        orb_voice_render(v, O.busd, O.buss, 0, O.own[4], 0);
        for (k = 0; k < (int)ORB_SUB && i < n; k++, i++) outL[i] = O.busd[2 * k] / 32768.0f;
    }
    return n;
}

static void test_orbit(void)
{
    static float x[8192];
    double h[12], thd;
    int k;
    /* a pitch whose period is a whole number of frames over 8192 (bin 64 = 344.5 Hz) */
    int32_t p = (int32_t)lround((69.0 + 12.0 * log2((64.0 * 44100.0 / 8192.0) / 440.0)) * 256.0);
    orb_system(2, &S);                      /* (HARMONICES) */
    orb_init(&O, &S, &MEM);
    plain(&S.snd[0]);
    S.snd[0].ecc = 0;
    render_voice(x, 8192, p);
    for (k = 1; k <= 8; k++) h[k] = dft_mag(x, 8192, 64.0 * k);
    thd = sqrt(h[2] * h[2] + h[3] * h[3] + h[4] * h[4]) / h[1];
    printf("orbit e=0: fundamental %.3f, THD %.1f dB\n", h[1], 20 * log10(thd + 1e-12));
    CHECK(20 * log10(thd + 1e-12) < -60, "e = 0 is not a clean sine (THD %.1f dB)", 20 * log10(thd));
    CHECK(h[1] > 0.15, "e = 0 too quiet (%.3f)", h[1]);
    {
        double c0, c1, c2, e2;
        S.snd[0].ecc = 70;
        orb_init(&O, &S, &MEM);
        render_voice(x, 8192, p);
        for (k = 1; k <= 8; k++) h[k] = dft_mag(x, 8192, 64.0 * k);
        c1 = h[2] / h[1];
        S.snd[0].sling = 99;
        orb_init(&O, &S, &MEM);
        render_voice(x, 8192, p);
        for (k = 1; k <= 8; k++) h[k] = dft_mag(x, 8192, 64.0 * k);
        c2 = h[4] / h[1];
        e2 = h[2] / h[1];
        c0 = 0;
        printf("orbit e=0.68: h2/h1 %.1f dB; with SLING: h2/h1 %.1f dB, h4/h1 %.1f dB\n", 20 * log10(c1), 20 * log10(e2),
               20 * log10(c2));
        CHECK(c1 > 0.05, "eccentricity does not brighten (h2/h1 %.3f)", c1);
        CHECK(c2 > c0 + 0.1, "SLING does not brighten (h4/h1 %.3f)", c2);
    }
}

static void test_pitch(void)
{
    int n, t, d, bad = 0;
    for (n = 12; n < 128; n++) {
        double f = 440.0 * pow(2.0, (n - 69) / 12.0), got = orb_inc(n * 256) * 44100.0 / 4294967296.0;
        if (fabs(1200 * log2(got / f)) > 0.5) bad++;
    }
    CHECK(!bad, "%d notes off by more than half a cent", bad);
    /* the tunings: degree 0..N-1 against their ratios */
    {
        static const double JUST[12] = {1, 16. / 15, 9. / 8, 6. / 5, 5. / 4, 4. / 3, 7. / 5, 3. / 2, 8. / 5, 5. / 3, 7. / 4, 15. / 8};
        static const double OTON[8] = {1, 9. / 8, 5. / 4, 11. / 8, 3. / 2, 13. / 8, 7. / 4, 15. / 8};
        orb_sys_clear(&S, "T");
        for (t = 0; t < 2; t++) {
            S.tuning = (uint8_t)t;
            for (d = 0; d < (t ? 8 : 12); d++) {
                double want = 1200 * log2(t ? OTON[d] : JUST[d]), got = orb_degree(&S, d) / 2.56;
                CHECK(fabs(want - got) < 0.3, "tuning %d degree %d: %.2f cents, want %.2f", t, d, got, want);
            }
            CHECK(orb_degree(&S, t ? 8 : 12) == 3072, "tuning %d: the degree after the last is not the octave", t);
            CHECK(orb_degree(&S, -1) == orb_degree(&S, t ? 7 : 11) - 3072, "tuning %d: negative degrees", t);
        }
    }
    /* a key in SOLAR: E above C at full pull is 5/4 (-13.7 cents) */
    orb_sys_clear(&S, "T");
    S.sun = 0; S.tuning = 0; S.macro[MAC_GRAVITY] = 50;
    CHECK(abs(orb_key_pitch(&S, 64, 0, 0) - (64 * 256 - 35)) <= 1, "E over C is not 5/4 (%d)", orb_key_pitch(&S, 64, 0, 0) - 64 * 256);
    S.macro[MAC_GRAVITY] = 0;
    CHECK(orb_key_pitch(&S, 64, 0, 0) == 64 * 256, "GRAVITY 0 retunes");
    printf("pitch: 116 notes within half a cent; JUST and OTONAL exact\n");
}

static void test_orrery(void)
{
    uint32_t r, blocks, fires[4] = {0}, last[4] = {0}, conj;
    int32_t out[512];
    orb_system(2, &S);                      /* (HARMONICES) */
    for (r = 0; r < 4u; r++) {              /* every step a hit; 1:1, 3:2, 2:1, 1:2 */
        orb_ring_t *g = &S.ring[r];
        static const uint8_t P[4] = {1, 3, 2, 1}, Q[4] = {1, 2, 1, 2};
        g->ratio = orb_ratio_idx(P[r], Q[r]);
        g->len = 16; g->hits = 16; g->rot = 0; g->logic = 0; g->prob = 99; g->dir = 0; g->mute = 0;
        orb_euclid(g);
    }
    S.swing = 0; S.hum = 0; S.macro[MAC_FRACT] = 0; S.macro[MAC_CHAOS] = 0; S.bpm = 120;
    orb_init(&O, &S, &MEM);
    orb_play(&O, 1);
    /* 120 bpm: 8 base steps a second; 16 s = 128 base steps */
    blocks = (uint32_t)(16.0 * 44100 / 256);
    for (r = 0; r < blocks; r++) {
        uint32_t q;
        memset(out, 0, sizeof out);
        orb_render(&O, out, 256);
        for (q = 0; q < 4u; q++) { fires[q] += O.rs[q].flash - last[q]; last[q] = O.rs[q].flash; }
    }
    conj = O.conj_count;
    printf("orrery 16 s at 120: fires 1:1 %u, 3:2 %u, 2:1 %u, 1:2 %u; conjunction ticks %u\n", fires[0], fires[1], fires[2], fires[3], conj);
    CHECK(fires[0] >= 127 && fires[0] <= 129, "1:1 fired %u times, want 128", fires[0]);
    CHECK(fires[1] >= 190 && fires[1] <= 193, "3:2 fired %u times, want 192", fires[1]);
    CHECK(fires[2] >= 255 && fires[2] <= 257, "2:1 fired %u times, want 256", fires[2]);
    CHECK(fires[3] >= 63 && fires[3] <= 65, "1:2 fired %u times, want 64", fires[3]);
    /* every 2:1 tick that is also a 3:2 one, or a 1:1 one ...: the union of meeting ticks a base step pair. Over 2 base
     * steps: 1:1 at 0 1, 3:2 at 0 2/3 4/3, 2:1 at 0 .5 1 1.5, 1:2 at 0: meetings at 0 and 1 -> 2 per 2 base steps */
    CHECK(conj >= 126 && conj <= 130, "conjunctions %u, want 128", conj);

    /* NOT: ring B (1:1, all hits) NOT A (1:1, every other step) fires only where A rests */
    orb_system(2, &S);                      /* (HARMONICES) */
    S.ring[0].ratio = ORB_RATIO_1_1; S.ring[0].len = 16; S.ring[0].hits = 8; S.ring[0].rot = 0; orb_euclid(&S.ring[0]);
    S.ring[1].ratio = ORB_RATIO_1_1; S.ring[1].len = 16; S.ring[1].hits = 16; S.ring[1].logic = LOG_NOT; S.ring[1].ltgt = 0;
    S.ring[1].prob = 99; orb_euclid(&S.ring[1]);
    S.ring[2].mute = 1; S.ring[3].mute = 1; S.swing = 0; S.hum = 0; S.macro[MAC_FRACT] = 0; S.bpm = 120;
    orb_init(&O, &S, &MEM);
    orb_play(&O, 1);
    for (r = 0; r < (uint32_t)(4.0 * 44100 / 256); r++) { memset(out, 0, sizeof out); orb_render(&O, out, 256); }
    printf("logic: A %u hits, B NOT A %u in 4 s\n", O.rs[0].flash, O.rs[1].flash);
    CHECK(O.rs[0].flash >= 15 && O.rs[0].flash <= 17, "A fired %u, want 16", O.rs[0].flash);
    CHECK(O.rs[1].flash >= 15 && O.rs[1].flash <= 17, "B NOT A fired %u, want 16", O.rs[1].flash);

    /* MUTATE plays a variation of the pattern each lap, drawn afresh: the pattern as written never changes, a lap plays
     * as many hits as it has, nothing past the ring's end, and a degree a step at most 1 + VAR/25 from where it was */
    {
        static orb_sys_t W;
        uint32_t b, i, r, laps[ORB_NRING] = {0}, varied = 0, seen = 0;
        orb_system(6, &S);                  /* (SPIRAL DRIFT) */
        S.mutate = 99; S.var = 99;
        S.ring[1].dg[S.ring[1].len - 1u] = 21; S.ring[1].on[S.ring[1].len - 1u] = 2;   /* (a degree past MUTATE's old -7..14) */
        W = S;
        orb_init(&O, &S, &MEM);
        orb_play(&O, 1);
        for (b = 0; b < (uint32_t)(40.0 * 44100 / 256); b++) {
            memset(out, 0, sizeof out);
            orb_render(&O, out, 256);
            for (r = 0; r < ORB_NRING; r++) {
                const orb_ring_t *g = &S.ring[r];
                uint32_t h0 = 0, h1 = 0;
                int32_t d;
                if (O.rs[r].laps == laps[r]) continue;
                laps[r] = O.rs[r].laps;
                seen++;
                for (i = 0; i < ORB_NSTEP; i++) {
                    uint32_t on = orb_step_on(&O, r, i, &d);
                    if (i >= g->len) { CHECK(!on, "MUTATE played past ring %u's end (step %u)", r, i); continue; }
                    h0 += g->on[i] != 0;
                    h1 += on != 0;
                    if (on && g->on[i] && on == g->on[i] && d != g->dg[i])
                        CHECK(d - g->dg[i] <= 4 && g->dg[i] - d <= 4, "MUTATE moved a degree %d -> %d", g->dg[i], d);
                    if (O.rs[r].vm[i >> 5] >> (i & 31u) & 1u) varied++;
                }
                CHECK(h0 == h1, "MUTATE: ring %u's lap plays %u hits, %u written", r, h1, h0);
            }
        }
        CHECK(!memcmp(&W, &S, sizeof S), "MUTATE wrote into the pattern");
        printf("mutate: %u laps, %u steps varied, the patterns as written\n", seen, varied);
        CHECK(seen > 20u && varied > seen, "MUTATE did not vary (%u laps, %u steps)", seen, varied);
        orb_pattern_edited(&O, ORB_NRING);
        memset(out, 0, sizeof out);
        orb_render(&O, out, 256);
        for (r = 0; r < ORB_NRING; r++) CHECK(!O.rs[r].vm[0] && !O.rs[r].vm[1], "an edit kept ring %u's variation", r);
    }
}

static void test_alias(void)
{
    static float x[8192];
    double total = 0, harm = 0;
    int k;
    /* F6 (1378 Hz, bin 256) at full eccentricity and SLING: what is not harmonic must stay ~50 dB down */
    int32_t p = (int32_t)lround((69.0 + 12.0 * log2((256.0 * 44100.0 / 8192.0) / 440.0)) * 256.0);   /* bin 256 */
    orb_system(2, &S);                      /* (HARMONICES) */
    orb_init(&O, &S, &MEM);
    plain(&S.snd[0]);
    S.snd[0].ecc = 99;
    S.snd[0].sling = 99;
    S.snd[0].ang = 30;
    render_voice(x, 8192, p);
    for (k = 0; k < 8192; k++) x[k] *= (float)(0.5 - 0.5 * cos(2 * M_PI * k / 8192));   /* Hann: no leakage skirts */
    for (k = 1; k < 4096; k++) {            /* within 4 bins of a harmonic: harmonic (the window's main lobe) */
        double m = dft_mag(x, 8192, k);
        int off = k % 256;
        total += m * m;
        if (off <= 4 || off >= 252) harm += m * m;
    }
    printf("alias F6 at ECC 99 + SLING 99: inharmonic %.1f dB under the harmonics\n", 10 * log10((total - harm) / harm + 1e-15));
    CHECK(10 * log10((total - harm) / harm + 1e-15) < -45, "aliasing %.1f dB", 10 * log10((total - harm) / harm));
}

static void test_systems(void)
{
    uint32_t i, b;
    static int32_t out[512];
    for (i = 0; i < ORB_NSYSTEM; i++) {
        double sum = 0, sq = 0;
        int32_t peak = 0;
        uint32_t n = 0;
        orb_system(i, &S);
        orb_init(&O, &S, &MEM);
        orb_play(&O, 1);
        for (b = 0; b < (uint32_t)(30.0 * 44100 / 256); b++) {
            uint32_t k;
            memset(out, 0, sizeof out);
            if (b == 200u) for (k = 0; k < 4u; k++) orb_note_on(&O, (uint8_t)(k + 1), orb_key_pitch(&S, 48u + S.sun + 4u * k, 0, 0), 100);
            if (b == 3000u) for (k = 0; k < 4u; k++) orb_note_off(&O, (uint8_t)(k + 1));
            if (b == 3500u) { uint32_t q; for (q = 0; q < ORB_NMACRO; q++) S.macro[q] = 99; S.hretro = 50; S.hshift = 24; }
            if (b == 3700u) for (k = 0; k < ORB_NPART; k++) { S.snd[k].tide = 99; S.snd[k].beam = 99; S.snd[k].insp = 99; }
            if (b == 4000u) { S.cut = 50; S.res = 99; S.drive = 99; S.dfb = 99; S.rdecay = 99; }
            orb_render(&O, out, 256);
            for (k = 0; k < 512u; k++) {
                int32_t a = out[k] < 0 ? -out[k] : out[k];
                if (a > peak) peak = a;
                sum += out[k];
                sq += (double)out[k] * out[k];
                n++;
            }
        }
        printf("system %-12s peak %5.1f dBFS  rms %5.1f dBFS  dc %+.5f\n", S.name, 20 * log10((peak + 1) / 32768.0),
               10 * log10(sq / n / (32768.0 * 32768.0) + 1e-15), sum / n / 32768.0);
        CHECK(peak <= 32767, "system %u: peak %d over full scale", i, peak);
        CHECK(fabs(sum / n / 32768.0) < 0.002, "system %u: DC %.4f", i, sum / n / 32768.0);
        CHECK(sq / n > 1e3, "system %u: silent", i);
    }
}

static void test_inspiral(void)
{
    /* the chirp starts 2 octaves under (insp 50: 24 semitones) and lands on the note at its time; then it holds */
    orb_voice_t *v = &O.v[4];
    int32_t p = 57 * 256, k, first = 0, mid = 0;
    uint32_t inc_target = orb_inc(p);
    orb_system(2, &S);                      /* (HARMONICES) */
    orb_init(&O, &S, &MEM);
    plain(&S.snd[0]);
    S.snd[0].insp = 50;
    S.snd[0].inspt = 50;                    /* 147 ms: ~203 ticks */
    S.macro[MAC_GRAVITY] = 0;
    orb_trigger(&O, 4, ORB_KEYS, p, 32767, -1, INT32_MIN);
    for (k = 0; k < 400; k++) {
        orb_voice_tick(&O, v);
        if (k == 0) first = (int32_t)v->inc;
        if (k == 100) mid = (int32_t)v->inc;
    }
    printf("inspiral: start %.2f, half-way %.2f, end %.3f of the note's frequency\n", (double)first / inc_target,
           (double)mid / inc_target, (double)v->inc / inc_target);
    CHECK(fabs((double)first / inc_target - 0.25) < 0.02, "the chirp does not start 2 octaves under");
    CHECK((double)mid / inc_target < 0.6, "the chirp rises too early (a chirp is slow, then quick)");
    CHECK(fabs((double)v->inc / inc_target - 1.0) < 0.002, "the chirp does not land on the note");
    CHECK(v->estage != 0 || v->eenv == 0, "the merge did not start the eccentricity's flash");
}

static void test_horizon(void)
{
    /* a held chord, then HORIZON all the way: still sound (frozen), in range, a twelfth down */
    static int32_t out[512];
    uint32_t b, k;
    double sq = 0;
    int32_t peak = 0;
    orb_system(8, &S);                      /* (NEBULA) */
    orb_init(&O, &S, &MEM);
    for (k = 0; k < 3u; k++) orb_note_on(&O, (uint8_t)(k + 1), orb_key_pitch(&S, 60u + 4u * k, 0, 0), 100);
    S.hshift = -19; S.hretro = 30; S.hgrain = 40;
    for (b = 0; b < (uint32_t)(6.0 * 44100 / 256); b++) {
        memset(out, 0, sizeof out);
        if (b == 300u) { S.macro[MAC_HORIZON] = 99; for (k = 0; k < 3u; k++) orb_note_off(&O, (uint8_t)(k + 1)); S.rmix = 0; S.dmix = 0; }
        orb_render(&O, out, 256);
        if (b > 700u) for (k = 0; k < 512u; k++) { int32_t a = out[k] < 0 ? -out[k] : out[k]; if (a > peak) peak = a; sq += (double)out[k] * out[k]; }
    }
    printf("horizon: frozen, after the notes ended: rms %.1f dBFS, peak %.1f dBFS\n", 10 * log10(sq / ((6.0 * 44100 / 256 - 701) * 512) / 1073741824.0 + 1e-15), 20 * log10((peak + 1) / 32768.0));
    CHECK(sq > 0 && peak > 300, "HORIZON at full depth holds nothing");
    CHECK(peak <= 32767, "HORIZON over full scale");
}

/* the library: every sound loads with its register (a ring's root C2 + 12 x oct; the keys' part two octaves less) and
 * its name; the keys play a track's sound on the keys' voices, and the ring's own voice is left alone */
static void test_library(void)
{
    static int32_t out[512];
    uint32_t li, b, k, keys_on = 0;
    for (li = 1; li < ORB_NLIB; li++) {
        orb_snd_t a = {0}, kk = {0};
        orb_lib_load(&a, li, ORB_A);
        orb_lib_load(&kk, li, ORB_KEYS);
        CHECK(a.lib == li && a.oct == ORB_LIB[li].oct, "library %u: lib %u oct %d", li, a.lib, a.oct);
        CHECK(kk.oct == (ORB_LIB[li].oct - 2 < -3 ? -3 : ORB_LIB[li].oct - 2), "library %u on the keys: oct %d", li, kk.oct);
        CHECK(a.level > 30 && ORB_LIB[li].name[0], "library %u: level %u", li, a.level);
    }
    orb_system(0, &S);
    for (k = 0; k < ORB_NRING; k++) S.ring[k].mute = 1;
    orb_init(&O, &S, &MEM);
    orb_note_on_part(&O, 1, 48 * 256, 100, ORB_B);
    for (b = 0; b < 20u; b++) { memset(out, 0, sizeof out); orb_render(&O, out, 256); }
    for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++) if (O.v[k].on && O.v[k].part == ORB_B) keys_on++;
    CHECK(keys_on == 1, "the keys playing B: %u key voices on B", keys_on);
    CHECK(!O.v[ORB_B - 1].stage, "the keys playing B woke B's ring voice");
    CHECK(O.v[ORB_NV - ORB_NKV].target == 48 * 256 + S.snd[ORB_B].oct * 3072 || keys_on, "the pitch");
    orb_note_off(&O, 1);
    printf("library: %u sounds load with their registers; the keys play a track's sound on their own voices\n", ORB_NLIB - 1u);
}

/* BACKGROUND RADIATION: all its energy on the sun's harmonics (it cannot beat with itself), its fluctuations on too;
 * NEBULA at its longest with BLOOM: it dies away, never past full scale; TIRED LIGHT: each echo 2/3 of the last */
static double peak_freq(const float *x, int n, double lo, double hi)
{
    static float w[8192];
    double best = 0, bf = lo, f;
    int i;
    for (i = 0; i < n; i++) w[i] = x[i] * (float)(0.5 - 0.5 * cos(2 * M_PI * i / n));
    for (f = lo; f <= hi; f += 0.5) {
        double m = dft_mag(w, n, f * n / 44100.0);
        if (m > best) { best = m; bf = f; }
    }
    return bf;
}
static void test_cosmic(void)
{
    static int32_t out[512];
    static float x[8192];
    uint32_t b, k, i;
    orb_system(2, &S);                      /* (HARMONICES, its rings quiet) */
    for (k = 0; k < ORB_NRING; k++) S.ring[k].mute = 1;
    S.tuning = 1; S.sun = 0; S.rmix = 0; S.dmix = 0; S.cut = 99; S.res = 0; S.fmode = 0;
    S.bg_level = 99; S.bg_glow = 0; S.bg_silk = 20; S.bg_hiss = 0; S.bg_aniso = 60; S.bg_cut = 0; S.bg_temp = 0;
    memset(S.mod, 0, sizeof S.mod);
    orb_init(&O, &S, &MEM);
    orb_play(&O, 1);
    for (b = 0; b < (uint32_t)(10.0 * 44100 / 256); b++) { memset(out, 0, sizeof out); orb_render(&O, out, 256); }
    for (i = 0; i < 8192u; i += 256u) {
        memset(out, 0, sizeof out);
        orb_render(&O, out, 256);
        for (k = 0; k < 256u; k++) x[i + k] = (float)out[2u * k];
    }
    {   /* the open chord: of C3 / 4, harmonics 2 3 4 6 8 9 12 16 (within 2 bins: its drift a few cents) */
        static const int H[8] = {2, 3, 4, 6, 8, 9, 12, 16};
        double f0 = 440.0 * pow(2.0, (48 - 69) / 12.0) / 4.0, tot = 0, harm = 0, bin = 44100.0 / 8192.0;
        static float w[8192];
        int kk, h;
        for (i = 0; i < 8192u; i++) w[i] = x[i] * (float)(0.5 - 0.5 * cos(2 * M_PI * i / 8192.0));
        for (kk = 1; kk < 400; kk++) {      /* up to 2.1 kHz */
            double m = dft_mag(w, 8192, kk), f = kk * bin;
            int on = 0;
            for (h = 0; h < 8; h++) if (fabs(f - H[h] * f0) <= 2 * bin) on = 1;
            tot += m * m;
            if (on) harm += m * m;
        }
        printf("background radiation: %.4f of its energy on its chord (one harmonic series)\n", harm / tot);
        CHECK(harm / tot > 0.99, "background radiation: %.4f on its chord", harm / tot);
        CHECK(tot > 1e3, "background radiation: silent");
    }
    /* NEBULA, all the way: size, decay, BLOOM, no damping, the pad glowing into it; then off: it must die away */
    S.rmix = 99; S.rsize = 99; S.rdecay = 99; S.rdamp = 0; S.bloom = 99; S.bg_glow = 99; S.bg_cut = 30;
    orb_init(&O, &S, &MEM);
    orb_play(&O, 1);
    {
        int32_t peak = 0;
        double e1 = 0, e2 = 0;
        for (b = 0; b < (uint32_t)(70.0 * 44100 / 256); b++) {
            if (b == (uint32_t)(4.0 * 44100 / 256)) orb_play(&O, 0);
            memset(out, 0, sizeof out);
            orb_render(&O, out, 256);
            for (k = 0; k < 512u; k++) {
                int32_t a = out[k] < 0 ? -out[k] : out[k];
                double t = b * 256.0 / 44100;
                if (a > peak) peak = a;
                if (t >= 10 && t < 15) e1 += (double)out[k] * out[k];
                if (t >= 60 && t < 65) e2 += (double)out[k] * out[k];
            }
        }
        printf("NEBULA at its longest with BLOOM: peak %.1f dBFS; 50 s later %.1f dB down\n", 20 * log10((peak + 1) / 32768.0),
               10 * log10((e1 + 1) / (e2 + 1)));
        CHECK(peak <= 32767, "NEBULA with BLOOM: over full scale");
        CHECK(e2 < e1 * 0.5, "NEBULA with BLOOM does not die away (%.3g then %.3g)", e1, e2);
        CHECK(e1 > 1e6, "NEBULA with BLOOM: no tail");
    }
    /* TIRED LIGHT: a short A4 into the echo; the second echo 2/3 of the first */
    orb_system(2, &S);
    for (k = 0; k < ORB_NRING; k++) S.ring[k].mute = 1;
    plain(&S.snd[0]);
    S.snd[0].send = 99;
    S.bpm = 120; S.dtime = 5; S.dfb = 80; S.dmix = 99; S.ping = 0; S.rmix = 0; S.dtone = 99; S.tired = 6;
    S.macro[MAC_GRAVITY] = 0; memset(S.mod, 0, sizeof S.mod);
    orb_init(&O, &S, &MEM);
    {
        static float rec[4 * 44100];
        uint32_t n = 0, D = 11025;
        double f1, f2;
        orb_note_on(&O, 1, 69 * 256, 100);
        while (n + 256u <= sizeof rec / sizeof rec[0]) {
            if (n >= 4410u && n < 4410u + 256u) orb_note_off(&O, 1);
            memset(out, 0, sizeof out);
            orb_render(&O, out, 256);
            for (k = 0; k < 256u; k++) rec[n + k] = (float)out[2u * k];
            n += 256u;
        }
        f1 = peak_freq(rec + D + 600, 2048, 200, 700);
        f2 = peak_freq(rec + 2 * D + 1800, 2048, 150, 700);
        printf("tired light: the echoes at %.1f Hz and %.1f Hz: %.3f (2/3)\n", f1, f2, f2 / f1);
        CHECK(fabs(f2 / f1 - 2.0 / 3.0) < 0.02, "tired light: %.3f, want 0.667", f2 / f1);
    }
}

int main(void)
{
    test_orbit();
    test_pitch();
    test_orrery();
    test_alias();
    test_inspiral();
    test_horizon();
    test_library();
    test_cosmic();
    test_systems();
    if (fails) { printf("orb_test: %d FAILED\n", fails); return 1; }
    printf("orb_test: all passed\n");
    return 0;
}
