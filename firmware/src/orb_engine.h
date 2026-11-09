/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX's engine: orbit synthesis, the Orrery (four rings), gravity, the filter map, SPACE (docs/orbitrix/PLAN.md).
 * One header, no platform: the firmware (orbitrix.c) and the host tools (tools/orb_render.c, tests/orb_test.c)
 * include the same code.
 *
 * Rules (docs/RULES.md): integers only, no 64-bit division, no flash in orb_render (the tables are const, in XIP),
 * no loop whose length a parameter sets. A feedback loop rounds toward zero (orb_rz).
 *
 *   orb_init(o, sys, mem)          the engine on a system (orb_sys_t, the main loop's to edit) and its memory (the pool)
 *   orb_render(o, out, n)          n stereo frames (a multiple of 32) added to out: the audio interrupt
 *   orb_note_on / orb_note_off     the keys (from the main loop: an event queue, one writer each side)
 *   orb_play(o, on)                the orrery's clock
 *   orb_euclid, orb_mutate         the rings' patterns (main loop) */
#ifndef ORB_ENGINE_H
#define ORB_ENGINE_H
#include <stdint.h>
#include "orb_tables.h"

#define ORB_FS 44100
#define ORB_SUB 32u                         /* frames a control tick: events, clock, modulation at 1378 Hz */
#define ORB_NV 8u                           /* voices: 0..3 the rings A..D, 4..7 the keys */
#define ORB_NKV 4u
#define ORB_NPART 5u                        /* sounds: KEYS, A, B, C, D */
#define ORB_NRING 4u
#define ORB_NSTEP 64u
#define ORB_TPS 2520u                       /* clock ticks a base step (a 16th): lcm 1..10, so every ratio is exact */
#define ORB_NMOD 8u
#define ORB_DLEN 32768u                     /* the delay's memory a channel (0.74 s) */
#define ORB_RLEN 4096u                      /* a NEBULA line (a power of 2: a mask) */
#define ORB_APLEN 1024u
#define ORB_HLEN 32768u                     /* HORIZON's memory (mono, 0.74 s) */
#define ORB_NBG 8u                          /* BACKGROUND RADIATION: its partials (the acoustic peaks) */
#define ORB_NTIRED 8u                       /* TIRED LIGHT: OFF, and seven just intervals an echo */

enum { ORB_KEYS = 0, ORB_A, ORB_B, ORB_C, ORB_D };
enum { DIR_FWD, DIR_REV, DIR_PING, DIR_SPIRAL, DIR_RND, ORB_NDIR };
enum { LOG_FREE, LOG_AND, LOG_NOT, LOG_XOR, LOG_CONJ, ORB_NLOGIC };
enum { MAC_CHAOS, MAC_SPECTRAL, MAC_GRAVITY, MAC_FRACT, MAC_ORBIT, MAC_TIDE, MAC_HORIZON, ORB_NMACRO };
enum { KM_SOLAR, KM_HARMONIC };
enum { LS_SINE, LS_TRI, LS_SAW, LS_SQR, LS_SH, LS_DRIFT, ORB_NLSHAPE };
enum { SRC_OFF, SRC_LFO1, SRC_LFO2, SRC_ORBX, SRC_ORBY, SRC_RND, SRC_STEP, SRC_CONJ, SRC_VEL, SRC_MODW, SRC_AFT, ORB_NSRC };
enum { DST_ECC, DST_SLING, DST_ANG, DST_SPR, DST_MOON, DST_DUST, DST_PIXEL, DST_PITCH, DST_CUT, DST_RES, DST_MODE,
       DST_SEND, DST_FDBK, DST_RAD, DST_PREC, DST_LEVEL, DST_PROB, DST_TIDE, DST_HORIZON, DST_BEAM, ORB_NDST };
#define SF_PHASE 1u                         /* snd flags: the orbit restarts at the near point on a hit */
#define SF_LEGATO 2u                        /* a ring's notes glide into each other while it sounds */

static const char *const ORB_PART_NAME[ORB_NPART] = {"KEYS", "A", "B", "C", "D"};
static const char *const ORB_DIR_NAME[ORB_NDIR] = {"FWD", "REV", "PING", "SPIRAL", "RND"};
static const char *const ORB_LOGIC_NAME[ORB_NLOGIC] = {"FREE", "AND", "NOT", "XOR", "CONJ"};
static const char *const ORB_MACRO_NAME[ORB_NMACRO] = {"CHAOS", "SPECTRAL", "GRAVITY", "FRACT", "ORBIT", "TIDE",
                                                       "HORIZON"};
static const char *const ORB_LSHAPE_NAME[ORB_NLSHAPE] = {"SINE", "TRI", "SAW", "SQR", "S&H", "DRIFT"};
static const char *const ORB_SRC_NAME[ORB_NSRC] = {"OFF", "LFO1", "LFO2", "ORB X", "ORB Y", "RND", "STEP", "CONJ",
                                                   "VEL", "MODW", "AFT"};
static const char *const ORB_DST_NAME[ORB_NDST] = {"ECC", "SLING", "ANG", "SPR", "MOON", "DUST", "PIXEL", "PITCH",
                                                   "CUT", "RES", "MODE", "SEND", "FDBK", "RAD", "PREC", "LEVEL",
                                                   "PROB", "TIDE", "HORIZN", "BEAM"};
/* the delay's times, in beats (num / den of a quarter) */
#define ORB_NDT 12u
static const uint8_t ORB_DT_NUM[ORB_NDT] = {1, 1, 1, 3, 1, 1, 3, 2, 3, 1, 3, 2};
static const uint8_t ORB_DT_DEN[ORB_NDT] = {8, 6, 4, 8, 3, 2, 4, 3, 2, 1, 2, 1};   /* sorted by length */
static const char *const ORB_DT_NAME[ORB_NDT] = {"1/32", "1/16T", "1/16", "3/32", "1/8T", "1/8", "3/16", "1/4T",
                                                 "3/8", "1/4", "3/8D", "1/2"};
/* synced LFO / orbit pad periods, in base steps (16ths) */
#define ORB_NSYNC 10u
static const uint16_t ORB_SYNC_STEPS[ORB_NSYNC] = {256, 128, 64, 48, 32, 16, 12, 8, 4, 2};
/* TIRED LIGHT: an echo's ratio to the last (1 - ratio, Q16: how much slower the feedback reads) and its z */
static const uint16_t ORB_TIRED_RATE[ORB_NTIRED] = {0, 4096, 7282, 10923, 13107, 16384, 21845, 32768};
static const char *const ORB_TIRED_NAME[ORB_NTIRED] = {"OFF", "z 0.07", "z 0.13", "z 0.20", "z 0.25", "z 0.33", "z 0.50",
                                                       "z 1.00"};   /* (15/16 8/9 5/6 4/5 3/4 2/3 1/2) */
static const char *const ORB_SYNC_NAME[ORB_NSYNC] = {"16BR", "8BR", "4BR", "3BR", "2BR", "1BR", "3/4", "1/2", "1/4", "1/8"};

/* ---- what the main loop edits (a system) */
typedef struct {                            /* a sound: what a part plays. 0..99 unless said */
    uint8_t ecc, sling, ang, spr;           /* orbit: eccentricity, the velocity view, projection angle, L/R spread */
    uint8_t prec, moon, mratio, dust;       /* precession rate (0 = still), moon radius, its ratio (list), dust */
    uint8_t color, pixel, fall, fallt;      /* dust colour (0 dark .. 99 bright), pixel grid, infall (semitones / 2), its time */
    uint8_t a, d, s, r;                     /* amplitude envelope */
    int8_t eamt;                            /* eccentricity envelope amount, -99..99 */
    uint8_t ea, ed, vsens;                  /* its attack and decay; velocity sensitivity */
    uint8_t level, pan, send, gate;         /* level, pan (50 centre), SPACE send, gate (a ring's, % of a step) */
    int8_t oct;                             /* octave, -3..3 */
    uint8_t flags;
    uint8_t tide;                           /* how hard the other planets pull this one (TIDE: their sum on its phase) */
    uint8_t insp, inspt;                    /* INSPIRAL: from up to 4 octaves under, a binary's chirp onto the note; its time */
    uint8_t beam;                           /* BEAM: the planet's light pulses at the moon's ratio (AM) */
    uint8_t lib;                            /* (the app's: the library sound it came from, 1.., 0 none; the engine leaves it) */
    uint8_t rsv;
} orb_snd_t;                                /* 32 B */

typedef struct {                            /* a ring */
    uint8_t ratio, len, hits, rot;          /* speed (ORB_RATIO_*), length 1..64, even hits, rotation */
    uint8_t dir, logic, ltgt, prob;         /* direction, trigger logic, its other ring, probability 0..99 */
    int8_t deg;                             /* the degree it sings (of the tuning), -14..21 */
    uint8_t mute, rsv[2];
    uint8_t on[ORB_NSTEP];                  /* 0 rest, 1 hit, 2 accent */
    int8_t dg[ORB_NSTEP];                   /* each step's degree, added to deg */
} orb_ring_t;                               /* 140 B */

typedef struct {
    uint8_t src, dst;
    int8_t amt;                             /* -99..99 */
    uint8_t rsv;
} orb_mod_t;

typedef struct {
    char name[12];
    orb_snd_t snd[ORB_NPART];
    orb_ring_t ring[ORB_NRING];
    uint8_t bpm, swing, mutate, hum;        /* 40..240; swing 0..99 (0 straight); mutate, hum */
    uint8_t var, tuning, sun, keymode;      /* new hits' degree variation; ORB_SCALE row; the sun 0..11; KM_* */
    uint8_t cut, res, fmode, drive;         /* the filter map */
    uint8_t dtime, dfb, dtone, ping;        /* delay: ORB_DT_*, feedback, tone, ping-pong */
    uint8_t rsize, rdecay, rdamp, rmix;     /* NEBULA */
    uint8_t dmix, hgrain, hretro;           /* the delay's return; HORIZON: grain length, the share of grains backwards */
    int8_t hshift;                          /* HORIZON: the shift at full depth, -24..24 semitones (redshift < 0) */
    uint8_t macro[ORB_NMACRO];
    uint8_t ox, oy, orad, orate;            /* the orbit pad: centre, radius, period (ORB_SYNC_*) */
    uint8_t lrate[2], lshape[2], lsync[2];  /* LFO 1 and 2: rate (free 0..99, synced ORB_SYNC_* index), shape */
    orb_mod_t mod[ORB_NMOD];
    /* BACKGROUND RADIATION (a pad under everything, the sun's harmonic series from every side): its level, its glow
     * into NEBULA, Silk damping (the high peaks), baryon loading (the odd peaks over the even), anisotropy (its tiny
     * fluctuations), the low cut (of it, and of what goes into NEBULA); its temperature: the octave, -2..1 */
    uint8_t bg_level, bg_glow, bg_silk, bg_baryon, bg_aniso, bg_cut;
    int8_t bg_temp;
    uint8_t bloom;                          /* NEBULA's own light back into it: longer, denser, 0..99 */
    uint8_t tired;                          /* TIRED LIGHT: each echo a just interval redder (ORB_TIRED_*), 0 off */
    uint8_t rsv[7];
} orb_sys_t;

/* ---- the engine's own state */
typedef struct {
    uint32_t M, mm, inc, minc, prec;        /* phases: mean anomaly, moon, their steps; the precession angle */
    int32_t e, es;                          /* eccentricity (row units, Q11 a row) and its step a frame */
    int32_t a, as;                          /* amplitude Q15 and its step a frame */
    int32_t sl, sls;                        /* sling Q15 and its step */
    int32_t env, eenv;                      /* amplitude envelope Q24; eccentricity envelope Q24 */
    uint8_t stage, estage, part, on;        /* env stage 0 off 1 A 2 D 3 S 4 R; ecc env 1 A 2 D 0 done */
    uint8_t id, kill, insp_grow, rsv;       /* kill: shed (the load was too high): fades in ~10 ms; insp_grow: the chirp
                                             * started from silence, so it may grow (a ringing voice keeps its level) */
    int32_t target;                         /* pitch, 1/256 semitone (MIDI 60 = 15360) */
    int32_t poff, pvel;                     /* gravity's spring: offset (1/65536 semitone) and its speed */
    int32_t fall;                           /* infall, 1/256 semitone, decaying */
    int32_t gate;                           /* frames to the release (rings), <0 = held (keys) */
    int32_t vel;                            /* Q15 */
    int32_t jit[3];                         /* CHAOS's random walks (Q15) */
    int32_t cl, sl_, cr, sr;                /* the projection, Q12, with the pan in it */
    int32_t dcl, dsl, dcr, dsr;             /* ... and their steps a frame (a tick's change spread over its frames) */
    int32_t gx, gvx, gvy, mr, dq;           /* gains Q12 (row), moon radius Q15, dust Q15 */
    int32_t mrs, dqs, bqs, tks;             /* the steps a frame of the moon, the dust, the beam, the tide: no jumps */
    int32_t kc, nl1, nl2;                   /* dust colour: coefficient Q15, the two noise states */
    int32_t pixm, hp;                       /* pixel mask; dust high-pass (1) or low-pass (0) */
    int32_t send;                           /* Q15 */
    uint32_t rng, age;
    int32_t tk;                             /* TIDE: the others' pull on the phase (0..98301) */
    int32_t bq;                             /* BEAM's depth, Q15 */
    int32_t insp_t, insp_T, insp_drop;      /* INSPIRAL: ticks so far, its length (0 = done), the drop (1/256 st) */
} orb_voice_t;

typedef struct {
    uint32_t base, next;                    /* the step's unswung tick, the tick it fires on */
    uint32_t k, pos, spin, laps;            /* steps so far, position, SPIRAL's turn, laps */
    uint8_t cur, lit, fired, rsv;           /* the step now, whether it is a hit, whether it fired */
    uint32_t vm[2];                         /* MUTATE: the steps this lap plays otherwise than written (a bit a step) */
    uint8_t von[ORB_NSTEP];                 /* ... and what they play instead (the pattern itself is never written) */
    int8_t vdg[ORB_NSTEP];
    uint32_t rat_next, rat_dt, rat_left;    /* FRACT's ratchets */
    int32_t rat_pitch, rat_vel;
    volatile uint32_t flash;                /* counts fires (the screen) */
    int32_t last_p;                         /* the last pitch (legato glides) */
} orb_rstate_t;

typedef struct {
    int16_t dl[ORB_DLEN], dr[ORB_DLEN];
    int16_t rl[4][ORB_RLEN];
    int16_t ap[2][ORB_APLEN];
    int16_t hb[ORB_HLEN];
    int16_t hr[256];                        /* HORIZON's right: the wet a little later */
} orb_mem_t;

typedef struct {
    uint8_t type, id, vel, part;
    int32_t pitch;
} orb_ev_t;
enum { EV_NONE, EV_ON, EV_OFF, EV_ALLOFF, EV_PLAY, EV_STOP, EV_PART, EV_GLIDE, EV_PAT };
#define ORB_NEV 64u

typedef struct {
    orb_sys_t *sys;
    orb_mem_t *mem;
    orb_voice_t v[ORB_NV];
    orb_rstate_t rs[ORB_NRING];
    orb_ev_t evq[ORB_NEV];
    volatile uint32_t evw, evr;
    volatile uint32_t playing;
    uint32_t T, Tf;                         /* the clock: ticks, and the fraction (Q16). It always runs (LFOs, the pad);
                                               PLAY restarts it, and only then do the rings step */
    uint32_t rnd_k;                         /* the base step RND last took a value on */
    uint32_t rng, ages, vrng;               /* (vrng: MUTATE's own, so its draws leave the rest's alone) */
    uint32_t lph[2], orbph;                 /* LFO and orbit pad phases */
    int32_t lval[2], lsh[2], lsh0[2];       /* LFO values Q15; S&H and DRIFT's targets */
    int32_t src[ORB_NSRC];                  /* the modulation sources now, Q15 */
    int32_t md[ORB_NDST];                   /* the modulation at each destination, Q15 (-32767..32767) */
    int32_t conj;                           /* the conjunction's flare, Q15 */
    int32_t rnd, lastvel;
    volatile int32_t modw, aft;             /* from MIDI (main loop), Q15 */
    volatile uint32_t conj_count;           /* conjunctions so far (the screen) */
    volatile int32_t orb_x, orb_y;          /* the pad's point, 0..9900 (the screen) */
    /* FX */
    int32_t f_ic[2][2], f_a1, f_a2, f_a3, f_k, f_m, f_in, f_on;
    int32_t d_cur, d_w, d_lp[2];            /* delay: time Q8 (smoothed), write index, tone states */
    uint32_t r_w, r_lfo;
    int32_t r_len[4], r_lp[4], r_g, r_damp, r_apw[2], r_acc, r_ol[2], r_or[2];
    int32_t r_ge, r_be, r_b, r_q, r_hp, r_wet, r_cut;   /* NEBULA: the lines' gain and BLOOM's share as they are now (both
                                             * times r_q, how clear it is: a moved sun clears it), BLOOM's share, the
                                             * low cut's state, its own light (for BLOOM), the low cut's coefficient */
    uint32_t r_sun, r_dip;                  /* (r_dip: the clearing going down, ~50 ms, before it comes back) */
    /* BACKGROUND RADIATION, at NEBULA's half rate: the partials' phases, steps (and where they glide to), amplitudes
     * (and their steps a half-rate frame), pans, fluctuations (and their targets); the swell; its output */
    uint32_t bg_ph[ORB_NBG], bg_inc[ORB_NBG], bg_pan[ORB_NBG];
    int32_t bg_a[ORB_NBG], bg_as[ORB_NBG], bg_gl[ORB_NBG], bg_gr[ORB_NBG], bg_fl[ORB_NBG], bg_ft[ORB_NBG];
    int32_t bg_env, bg_on, bg_glow, bg_ol[2], bg_or[2], bg_g[ORB_NBG];   /* bg_g: Silk, baryons, the low cut (Q15) */
    uint32_t bg_k;
    /* TIRED LIGHT: the feedback's slow-down (Q16 a frame), its ramp (Q16), the shifted taps' tone */
    uint32_t z_rate, z_p;
    int32_t z_lp[2];
    int32_t dc_x[2], dc_y[2];
    int16_t scope[128];                     /* the output (L), every other frame (the screen) */
    uint32_t scope_w;
    volatile uint32_t peak;                 /* the output's peak since the screen took it */
    int32_t busd[2 * ORB_SUB], buss[2 * ORB_SUB];
    /* TIDE: the voices' mono sum in the last sub-block, the one being made, and each voice's own share of the last */
    int32_t tide_prev[ORB_SUB], tide_next[ORB_SUB], own[ORB_NV][ORB_SUB];
    uint32_t tide_on;
    uint8_t own_live[ORB_NV];
    /* HORIZON: the write index, the source point (Q16), two grains */
    uint32_t h_w, h_P, h_wi, h_G, h_rate, h_rw;
    int32_t h_depth, h_mix, h_v, h_on;     /* h_mix glides (no click when the depth jumps); h_on while it is not 0 */
    uint32_t h_pos[2], h_wph[2];
    int32_t h_dir[2], h_t[2];
    volatile int32_t h_level;               /* how deep it is now (the screen), 0..99 */
} orb_t;

/* ---- small helpers (always inlined: the firmware is built -Os, where a call each would cost more than the work) */
#define ORB_INL static inline __attribute__((always_inline))
ORB_INL int32_t orb_rz(int32_t t, int n)   /* round toward zero (a feedback loop never sticks at -1): no branch */
{
    return (t + (int32_t)((uint32_t)(t >> 31) & ((1u << n) - 1u))) >> n;
}
ORB_INL int32_t orb_clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
ORB_INL int32_t orb_clamp64(int64_t v) { return v < -(1 << 24) ? -(1 << 24) : v > (1 << 24) ? (1 << 24) : (int32_t)v; }
ORB_INL int32_t orb_rz64(int64_t t, int n) { return (int32_t)(t < 0 ? -((-t) >> n) : t >> n); }
ORB_INL int32_t orb_sin(uint32_t ph)       /* Q15, interpolated */
{
    uint32_t i = ph >> 22;
    int32_t f = (int32_t)((ph >> 7) & 0x7FFFu), a = ORB_SIN[i];
    return a + (((ORB_SIN[i + 1u] - a) * f) >> 15);
}
ORB_INL int32_t orb_cos(uint32_t ph) { return orb_sin(ph + 0x40000000u); }
ORB_INL void orb_sincos(uint32_t ph, int32_t *s, int32_t *c)   /* both from one index: cos is a quarter on */
{
    uint32_t i = ph >> 22, j = (i + 256u) & 1023u;
    int32_t f = (int32_t)((ph >> 7) & 0x7FFFu), a = ORB_SIN[i], b = ORB_SIN[j];
    *s = a + (((ORB_SIN[i + 1u] - a) * f) >> 15);
    *c = b + (((ORB_SIN[j + 1u] - b) * f) >> 15);
}
ORB_INL uint32_t orb_xs(uint32_t *s) { uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x; }
ORB_INL int32_t orb_lin(uint8_t v) { return (int32_t)v * 331; }          /* 0..99 -> Q15 0..32769 */
static int32_t orb_q15(uint8_t v)                                              /* clamped */
{
    int32_t q = (int32_t)v * 331;
    return q > 32767 ? 32767 : q;
}

/* pitch (1/256 semitone, MIDI 0 = 0) -> phase increment */
static uint32_t orb_inc(int32_t p)
{
    uint32_t oct, fr, i, w, a, b;
    if (p < 0) p = 0;
    if (p > 143 * 256) p = 143 * 256;
    oct = (uint32_t)p / 3072u;
    fr = (uint32_t)p - oct * 3072u;
    i = fr >> 2;
    w = fr & 3u;
    a = ORB_EXP2[i];
    b = ORB_EXP2[i + 1u];
    a += ((b - a) >> 2) * w;
    return oct >= 11u ? a : a >> (11u - oct);
}

/* a degree of the tuning, above the sun: 1/256 semitone */
static int32_t orb_degree(const orb_sys_t *s, int32_t deg)
{
    uint32_t t = s->tuning < ORB_NTUNING ? s->tuning : 0u, n = ORB_SCALE_N[t];
    int32_t oct = 0;
    while (deg < 0 && oct > -8) { deg += (int32_t)n; oct--; }         /* (bounded) */
    while (deg >= (int32_t)n && oct < 8) { deg -= (int32_t)n; oct++; }
    return oct * 3072 + ORB_SCALE[t][deg];
}

/* the pull toward the just ratios a key gets (Q8: 0..256) from GRAVITY: 0..50 -> 0..1, then full */
static int32_t orb_pull(const orb_sys_t *s)
{
    int32_t g = s->macro[MAC_GRAVITY];
    return g >= 50 ? 256 : g * 256 / 50;
}

/* a key's pitch: SOLAR, the keyboard with each semitone above the sun on its ratio; HARMONIC, white keys harmonics
 * 1..16 of the sun, black ones 17..27. note: MIDI (SOLAR) or the key's index 0..26 from F3 (HARMONIC, keyoct: OCT) */
static const uint8_t ORB_WHITE_H[27] = {1, 17, 2, 18, 3, 19, 4, 5, 20, 6, 21, 7, 22, 8, 9, 23, 10, 24, 11, 12, 25,
                                        13, 26, 14, 27, 15, 16};   /* F3 .. G5: white keys 1..16, black 17..27 */
static int32_t orb_key_pitch(const orb_sys_t *s, uint32_t note, int32_t keyoct, int32_t *et)
{
    uint32_t t = s->tuning < ORB_NTUNING ? s->tuning : 0u;
    if (s->keymode == KM_HARMONIC) {
        int32_t h = note < 27u ? ORB_WHITE_H[note] : 1, p = (36 + (int32_t)s->sun + 12 * keyoct) * 256 + ORB_HARM[h - 1];
        if (et) *et = p;
        return p;
    }
    {
        int32_t d = ((int32_t)note - (int32_t)s->sun) % 12, p = (int32_t)note * 256;
        if (d < 0) d += 12;
        if (et) *et = p;
        return p + ((ORB_KEYDEV[t][d] * orb_pull(s)) >> 8);
    }
}

/* ---- patterns (main loop) */
static void orb_euclid(orb_ring_t *r)       /* HITS spread evenly over LEN, turned by ROT; accents and degrees kept */
{
    uint32_t i, len = r->len < 1u ? 1u : r->len > ORB_NSTEP ? ORB_NSTEP : r->len, h = r->hits > len ? len : r->hits;
    for (i = 0; i < ORB_NSTEP; i++) {
        uint32_t j = (i + len - (r->rot % len)) % len;
        int hit = i < len && h && ((j * h) % len) < h;
        if (!hit) r->on[i] = 0;
        else if (!r->on[i]) r->on[i] = 1;
    }
}

/* ---- events (main loop -> the interrupt) */
static void orb_push_part(orb_t *o, uint8_t type, uint8_t id, uint8_t vel, int32_t pitch, uint8_t part)
{
    uint32_t w = o->evw;
    if (w - o->evr >= ORB_NEV) return;      /* full: dropped */
    o->evq[w % ORB_NEV].part = part;
    o->evq[w % ORB_NEV].type = type;
    o->evq[w % ORB_NEV].id = id;
    o->evq[w % ORB_NEV].vel = vel;
    o->evq[w % ORB_NEV].pitch = pitch;
    __asm__ volatile("" ::: "memory");
    o->evw = w + 1u;
}
static void orb_push(orb_t *o, uint8_t type, uint8_t id, uint8_t vel, int32_t pitch)
{
    uint32_t w = o->evw;
    if (w - o->evr >= ORB_NEV) return;      /* full: dropped */
    o->evq[w % ORB_NEV].type = type;
    o->evq[w % ORB_NEV].id = id;
    o->evq[w % ORB_NEV].vel = vel;
    o->evq[w % ORB_NEV].pitch = pitch;
    __asm__ volatile("" ::: "memory");
    o->evw = w + 1u;
}
static void orb_note_on(orb_t *o, uint8_t id, int32_t pitch, uint8_t vel) { orb_push_part(o, EV_ON, id, vel, pitch, ORB_KEYS); }
/* a key played with another part's sound (A..D) on the keys' voices: the sequence goes on, untouched, on the rings' own */
static void orb_note_on_part(orb_t *o, uint8_t id, int32_t pitch, uint8_t vel, uint32_t part)
{
    orb_push_part(o, EV_ON, id, vel, pitch, (uint8_t)(part <= ORB_D ? part : ORB_KEYS));
}
/* a ring's pattern was written (a step, its degree, LEN HITS ROT, a system): MUTATE's variation of it goes, the lap plays
 * it as written. r: 0..3, or ORB_NRING for all */
static void orb_pattern_edited(orb_t *o, uint32_t r) { orb_push(o, EV_PAT, (uint8_t)r, 0, 0); }
static void orb_note_off(orb_t *o, uint8_t id) { orb_push(o, EV_OFF, id, 0, 0); }
static void orb_all_off(orb_t *o) { orb_push(o, EV_ALLOFF, 0, 0, 0); }
/* a ring's sound played directly (part 1..4: A..D, on its own voice), for gate_10ms x 10 ms */
static void orb_part_on(orb_t *o, uint8_t part, int32_t pitch, uint8_t vel, uint8_t gate_10ms)
{
    orb_push_part(o, EV_PART, gate_10ms, vel, pitch, part);
}
/* a held key's note glides to another pitch, by gravity's spring (fast at GRAVITY <= 50, swooping above) */
static void orb_glide(orb_t *o, uint8_t id, int32_t pitch) { orb_push(o, EV_GLIDE, id, 0, pitch); }
static void orb_play(orb_t *o, int on) { orb_push(o, on ? EV_PLAY : EV_STOP, 0, 0, 0); }

/* ---- voices */
static void orb_trigger(orb_t *o, uint32_t vi, uint32_t part, int32_t pitch, int32_t vel, int32_t gate, int32_t from)
{
    orb_voice_t *v = &o->v[vi];
    const orb_snd_t *s = &o->sys->snd[part];
    int32_t g = o->sys->macro[MAC_GRAVITY];
    int sounding = v->stage && v->stage != 4u && v->env > (1 << 18);
    v->part = (uint8_t)part;
    v->vel = vel;
    v->gate = gate;
    v->target = pitch + (int32_t)s->oct * 3072;
    v->fall = (int32_t)s->fall * 128;       /* fall/2 semitones above, in 1/256 */
    v->age = ++o->ages;
    /* gravity: above 50 a note falls into tune on a spring, from where it was (legato, or a key's equal-tempered
     * pitch) or a little off */
    v->pvel = 0;
    v->poff = 0;
    if (g > 50 && !s->fall) {
        int32_t sgn = (orb_xs(&o->rng) & 1u) ? 1 : -1, kick = (g - 50) * 80;   /* up to ~40 cents (in 1/256: 102) */
        int32_t off = from != INT32_MIN ? from + (int32_t)s->oct * 3072 - v->target : 0;
        off = orb_clamp(off, -1536, 1536) + sgn * kick / 40;
        v->poff = off * 256;
    } else if (from != INT32_MIN && (s->flags & SF_LEGATO) && sounding) {
        v->poff = orb_clamp(from + (int32_t)s->oct * 3072 - v->target, -3072, 3072) * 256;
    }
    if (!sounding || (s->flags & SF_PHASE)) {
        if (s->flags & SF_PHASE) { v->M = 0; v->mm = 0; }
        if (!v->stage) { v->e = 0; v->a = 0; v->sl = 0; }
    }
    v->stage = 1;
    v->estage = 1;
    v->eenv = 0;
    v->on = 1;
    v->kill = 0;
    v->insp_T = 0;
    if (s->insp) {                          /* INSPIRAL: the chirp first; the eccentricity's flash waits for the merge */
        v->insp_T = (int32_t)(((uint32_t)ORB_TIME_MS[s->inspt % 100u] * 1411u) >> 10);   /* ms -> ticks (1378 a second) */
        if (v->insp_T < 8) v->insp_T = 8;
        v->insp_t = 0;
        v->insp_drop = (int32_t)s->insp * 12288 / 99;                                    /* up to 48 semitones */
        v->insp_grow = (uint8_t)!sounding;
        v->estage = 0;
        v->poff = 0; v->pvel = 0;
    }
    if (!v->rng) v->rng = 0x9E3779B9u ^ (vi * 0x85EBCA6Bu);
}

/* the load was too high: the quietest sounding voice fades out (the app calls it, from the interrupt's measure) */
static void orb_shed(orb_t *o)
{
    uint32_t k, pick = ORB_NV;
    int32_t least = 1 << 30;
    for (k = 0; k < ORB_NV; k++)
        if (o->v[k].stage && !o->v[k].kill && o->v[k].a < least) { least = o->v[k].a; pick = k; }
    if (pick < ORB_NV) o->v[pick].kill = 1;
}

static void orb_release(orb_voice_t *v)
{
    if (v->stage && v->stage < 4u) v->stage = 4;
    v->on = 0;
}

/* the control tick of a voice: envelopes, pitch, eccentricity, the projection. Then 32 frames. */
static void orb_voice_tick(orb_t *o, orb_voice_t *v)
{
    const orb_sys_t *sy = o->sys;
    const orb_snd_t *s = &sy->snd[v->part];
    const int32_t *md = o->md;
    int32_t e, cap, pitch, row, lvl, amp, pan, gl, gr, chaos = sy->macro[MAC_CHAOS], insp_left;
    uint32_t th, sp;
    int32_t spec = sy->macro[MAC_SPECTRAL], fract = sy->macro[MAC_FRACT], orbm = sy->macro[MAC_ORBIT];
    uint32_t pr;

    if (v->kill) {                          /* shed: a quick fade, then off */
        v->env -= v->env >> 2;
        v->stage = v->env < (1 << 12) ? 0 : 4;
        if (!v->stage) { v->env = 0; v->kill = 0; v->on = 0; }
    }
    /* the gate (rings) */
    if (v->gate > 0) { v->gate -= (int32_t)ORB_SUB; if (v->gate <= 0) { v->gate = 0; orb_release(v); } }
    /* the amplitude envelope */
    switch (v->stage) {
    case 1:
        v->env += (int32_t)ORB_ATK[s->a % 100u];
        if (v->env >= (1 << 24)) { v->env = 1 << 24; v->stage = 2; }
        break;
    case 2: {
        int32_t sus = (int32_t)s->s * 169467;   /* 99 -> 2^24 */
        v->env += (int32_t)(((int64_t)(sus - v->env) * ORB_DEC[s->d % 100u]) >> 16);
        if (v->env - sus < (1 << 12) && v->env - sus > -(1 << 12)) { v->env = sus; v->stage = 3; }
        if (!s->s && v->env < (1 << 10)) { v->env = 0; v->stage = 0; }
        break;
    }
    case 3:
        v->env = (int32_t)s->s * 169467;
        if (!s->s) v->stage = 0;
        break;
    case 4:
        v->env -= (int32_t)(((int64_t)v->env * ORB_DEC[s->r % 100u]) >> 16);
        if (v->env < (1 << 10)) { v->env = 0; v->stage = 0; }
        break;
    default:
        v->env = 0;
        break;
    }
    /* the eccentricity envelope: attack, then decay to nothing */
    if (v->estage == 1) {
        v->eenv += (int32_t)ORB_ATK[s->ea % 100u];
        if (v->eenv >= (1 << 24)) { v->eenv = 1 << 24; v->estage = 2; }
    } else if (v->estage == 2) {
        v->eenv -= (int32_t)(((int64_t)v->eenv * ORB_DEC[s->ed % 100u]) >> 16);
        if (v->eenv < (1 << 8)) { v->eenv = 0; v->estage = 0; }
    }
    /* CHAOS: three slow random walks */
    if (chaos) {
        uint32_t k;
        for (k = 0; k < 3u; k++) {
            int32_t r = (int32_t)(orb_xs(&v->rng) >> 17) - 16384;   /* +-0.5 */
            v->jit[k] += orb_rz((r * 2 - v->jit[k]) * 3, 8);
        }
    }
    /* pitch: the target, gravity's spring, the infall, PITCH */
    if (v->poff || v->pvel) {
        int32_t g = sy->macro[MAC_GRAVITY], sg = g > 50 ? g - 50 : 0;   /* 0..49 */
        int64_t k = 3140 + (int64_t)(49 - sg) * 648;      /* (w dt)^2, Q24: 10 Hz just above 50 .. 3 Hz at 99 */
        int64_t c = 68900 + (int64_t)(49 - sg) * 14206;   /* 2 zeta w dt, Q24: zeta 0.5 .. 0.15 (overshoot, settle) */
        v->pvel += (int32_t)((-(k * v->poff) - c * v->pvel) >> 24);
        v->poff += v->pvel;
        if (v->poff < 64 && v->poff > -64 && v->pvel < 16 && v->pvel > -16) { v->poff = 0; v->pvel = 0; }
    }
    if (v->fall) v->fall -= orb_rz((int32_t)(((int64_t)v->fall * ORB_DEC[s->fallt % 100u]) >> 16), 0) + (v->fall > 0 ? 1 : -1);
    if (v->fall < 0) v->fall = 0;
    pitch = v->target + (v->poff >> 8) + v->fall + (md[DST_PITCH] >> 6);   /* PITCH: +-2 semitones */
    insp_left = 0;
    if (v->insp_T) {                        /* INSPIRAL: the share of the drop still to go, by a binary's chirp */
        uint32_t u = (uint32_t)v->insp_t * 4096u / (uint32_t)v->insp_T, i = u >> 6, f = u & 63u;   /* (a division a tick) */
        if (i > 63u) { i = 63u; f = 64u; }
        insp_left = ORB_CHIRP[i] + (((ORB_CHIRP[i + 1u] - ORB_CHIRP[i]) * (int32_t)f) >> 6);
        pitch -= (int32_t)(((int64_t)insp_left * v->insp_drop) >> 15);
        if (++v->insp_t >= v->insp_T) { v->insp_T = 0; v->estage = 1; v->eenv = 0; }   /* the merge */
    }
    if (chaos) pitch += (v->jit[2] * chaos) >> 13;                          /* a wobble, up to ~+-0.2 semitone */
    v->inc = orb_inc(pitch);
    /* eccentricity: the sound's, its envelope, SPECTRAL, CHAOS, ECC; capped by the note (aliasing) */
    e = (int32_t)s->ecc * 662 + (((v->eenv >> 12) * ((int32_t)s->eamt * 662)) >> 12);
    e += spec * 330 + md[DST_ECC] * 2 + ((v->jit[0] * chaos) >> 7);
    {
        int32_t sl = orb_lin(s->sling) + (md[DST_SLING]) + spec * 165;
        int32_t n = pitch >> 8, c0, c1;
        sl = orb_clamp(sl, 0, 32767);
        n = orb_clamp(n, 0, 143);
        c0 = ORB_ECAP[n];
        c1 = ORB_ECAPV[n];
        cap = c0 + (((c1 - c0) * (sl > 16383 ? 32767 : sl * 2)) >> 15);
        v->sls = orb_rz(sl - v->sl, 5);   /* (toward zero: the ramp never passes its target, so no clamp a frame) */
    }
    e = orb_clamp(e, 0, cap < 65535 ? cap : 65535);
    if (e > 65535) e = 65535;
    v->es = orb_rz(e - v->e, 5);
    {   /* the gains, between the rows at the eccentricity mid-block */
        int32_t em = orb_clamp(v->e + v->es * 16, 0, 65535), f = em & 2047;
        row = em >> 11;
        v->gx = ORB_GX[row] + (((ORB_GX[row + 1] - ORB_GX[row]) * f) >> 11);
        v->gvx = ORB_GVX[row] + (((ORB_GVX[row + 1] - ORB_GVX[row]) * f) >> 11);
        v->gvy = ORB_GVY[row] + (((ORB_GVY[row + 1] - ORB_GVY[row]) * f) >> 11);
    }
    /* the moon */
    {
        int32_t mr = (orb_lin(s->moon) * 3) >> 2;   /* radius up to 0.75 */
        uint32_t ratio = ORB_MRATIO[s->mratio % ORB_NMOON];
        mr += (md[DST_MOON] * 3) >> 2;
        mr += spec * 60;
        if (chaos) ratio = (uint32_t)((int32_t)ratio + ((((v->jit[1] * chaos) >> 15) * (int32_t)ratio) >> 10));   /* off the whole ratio, +-5 % */
        v->minc = (v->inc >> 8) * ratio;
        if (v->inc >> 8 && v->minc / (v->inc >> 8) != ratio) v->minc = 0;       /* (overflow: no moon) */
        if (v->minc > 0x70000000u) mr = 0;  /* above ~19 kHz: none */
        v->mrs = orb_rz(orb_clamp(mr, 0, 24575) - v->mr, 5);
    }
    /* dust and its colour; pixel */
    {
        int32_t d = orb_lin(s->dust) + md[DST_DUST], c = s->color;
        d = orb_clamp(d, 0, 32767);
        v->dqs = orb_rz(((d * d) >> 15) - v->dq, 5);   /* a curve: the first half is gentle */
        if (c < 50) { v->hp = 0; v->kc = 1200 + c * 640; }
        else { v->hp = 1; v->kc = (c - 50) * 640 + 200; }
        if (v->kc > 32767) v->kc = 32767;
    }
    {
        int32_t px = (int32_t)s->pixel + (md[DST_PIXEL] * 99 >> 15) + (fract * 6) / 10;
        px = orb_clamp(px, 0, 99);
        v->pixm = px ? ~((1 << (6 + (px * 9) / 99)) - 1) : -1;   /* 512 .. 1 levels a unit */
    }
    /* level: the envelope, velocity, the sound's level (squared), LEVEL */
    lvl = orb_lin(s->level);
    lvl = (lvl * lvl) >> 15;
    lvl = orb_clamp(lvl + ((lvl * md[DST_LEVEL]) >> 15), 0, 32767);
    {
        int32_t vs = orb_lin(s->vsens), vel = 32767 - (((32767 - v->vel) * vs) >> 15);
        amp = (((v->env >> 9) * vel) >> 15);                 /* Q15 */
        amp = (amp * lvl) >> 16;                             /* a voice at most half scale */
        if (insp_left && v->insp_grow) amp = (amp * (32767 - ((insp_left * 21845) >> 16))) >> 15;   /* a chirp grows: 1/3 .. 1 */
    }
    v->as = orb_rz(amp - v->a, 5);
    if (!v->stage && v->a < 64 && v->a > -64) { v->a = 0; v->as = 0; }   /* (done: no residue keeps it rendering) */
    /* the projection: angle, spread (ORBIT widens), precession; the pan in the gains */
    pr = s->prec ? ORB_LFO[s->prec % 100u] >> 1 : 0u;
    pr += (uint32_t)orbm * 40000u;
    if (md[DST_PREC] > 0) pr += (uint32_t)md[DST_PREC] * 300u;
    v->prec += pr;
    /* (in uint32: angles wrap) 0..180 deg; ANG +-90; CHAOS +-55. Spread: half each side, ORBIT widens */
    th = (uint32_t)s->ang * 21474836u + (uint32_t)(md[DST_ANG] * 32768) + (uint32_t)(v->jit[1] * chaos * 200);
    sp = (uint32_t)s->spr * 10737418u + (uint32_t)(md[DST_SPR] * 16384) + (uint32_t)orbm * 5000000u;
    {
        uint32_t tl = th + v->prec - sp, tr = th + v->prec + sp;
        pan = orb_clamp((int32_t)s->pan, 0, 99);
        gl = orb_cos((uint32_t)pan * 10845877u);          /* 0..99 -> 0..90 deg: constant power */
        gr = orb_sin((uint32_t)pan * 10845877u);
        gl = (gl * 23170) >> 15;                          /* (x sqrt 2 at the centre: unity) */
        gr = (gr * 23170) >> 15;
        gl = gl * 2 > 32767 ? 32767 : gl * 2;
        gr = gr * 2 > 32767 ? 32767 : gr * 2;
        v->dcl = orb_rz(((orb_cos(tl) * gl) >> 18) - v->cl, 5);   /* Q12 */
        v->dsl = orb_rz(((orb_sin(tl) * gl) >> 18) - v->sl_, 5);
        v->dcr = orb_rz(((orb_cos(tr) * gr) >> 18) - v->cr, 5);
        v->dsr = orb_rz(((orb_sin(tr) * gr) >> 18) - v->sr, 5);
    }
    v->send = orb_clamp(orb_lin(s->send) + md[DST_SEND], 0, 32767);
    {   /* TIDE: how hard the others pull (the macro, its modulation, the sound's share); BEAM */
        int32_t mt = orb_clamp(orb_lin(sy->macro[MAC_TIDE]) + md[DST_TIDE], 0, 32767);
        v->tks = orb_rz((mt * (int32_t)s->tide) / 99 * 3 - v->tk, 5);   /* up to 98301: ~1.4 rad at 0.3 of the others */
        v->bqs = orb_rz(orb_clamp(orb_lin(s->beam) + md[DST_BEAM], 0, 32767) - v->bq, 5);
    }
}

static void orb_voice_render(orb_voice_t *v, int32_t *bd, int32_t *bs, const int32_t *tp, int32_t *ow, int32_t *tn)
{
    uint32_t i, M = v->M, mm = v->mm, inc = v->inc, minc = v->minc, rng = v->rng;
    int32_t e = v->e, es = v->es, a = v->a, as = v->as, sl = v->sl, sls = v->sls;
    int32_t gx = v->gx, gvx = v->gvx, gvy = v->gvy, mr = v->mr, dq = v->dq, kc = v->kc, n1 = v->nl1, n2 = v->nl2;
    int32_t cl = v->cl, sL = v->sl_, cr = v->cr, sR = v->sr, pm = v->pixm, send = v->send, hp = v->hp;
    int32_t dcl = v->dcl, dsl = v->dsl, dcr = v->dcr, dsr = v->dsr, mrs = v->mrs, dqs = v->dqs, bqs = v->bqs, tks = v->tks;
    int32_t bq = v->bq, tk = v->tk;
    int use_v = sl > 0 || sls > 0, use_m = mr > 0 || mrs > 0, use_d = dq > 0 || dqs > 0, use_p = pm != -1, use_s = send > 0;
    int use_t = tp && (tk > 0 || tks > 0), use_b = bq > 0 || bqs > 0, use_o = tn != 0;   /* (tn: TIDE on: all give) */
    for (i = 0; i < ORB_SUB; i++) {
        uint32_t row, col, E;
        int32_t wr, wc, d0, d1, d, cE, sE, x, y, L, R;
        const int16_t *k0;
        uint32_t Mt;
        M += inc;
        e += es;                            /* (0..65535 by the ramp's construction) */
        a += as;
        if (use_t) { tk += tks; Mt = M + (uint32_t)(tp[i] - ow[i]) * (uint32_t)tk; }   /* TIDE: the others' sum on the phase */
        else Mt = M;
        row = (uint32_t)e >> 11;
        wr = e & 2047;
        col = Mt >> 23;
        wc = (int32_t)((Mt >> 8) & 0x7FFFu);
        k0 = &ORB_KEP[row][col];
        d0 = k0[0] + (((k0[1] - k0[0]) * wc) >> 15);
        d1 = k0[ORB_NM + 1] + (((k0[ORB_NM + 2] - k0[ORB_NM + 1]) * wc) >> 15);
        d = d0 + (((d1 - d0) * wr) >> 11);
        E = Mt + ((uint32_t)d << 16);
        orb_sincos(E, &sE, &cE);
        x = ((cE + ((e * 15729) >> 16)) * gx) >> 12;    /* cos E + e/2: zero mean over time */
        y = sE;
        if (use_v) {                        /* SLING: the velocity, dE/dM from the second table */
            const int16_t *w0 = &ORB_KEPW[row][col];
            int32_t a0 = w0[0] + (((w0[1] - w0[0]) * wc) >> 15);
            int32_t a1 = w0[ORB_NM + 1] + (((w0[ORB_NM + 2] - w0[ORB_NM + 1]) * wc) >> 15);
            int32_t w = a0 + (((a1 - a0) * wr) >> 11), vx, vy;   /* Q10 */
            vx = ((((-sE) * w) >> 12) * gvx) >> 10;
            vy = (((cE * w) >> 12) * gvy) >> 10;
            vx = vx > 98303 ? 98303 : vx < -98303 ? -98303 : vx;   /* (a row's gain holds it to ~2; between rows, 3) */
            vy = vy > 98303 ? 98303 : vy < -98303 ? -98303 : vy;
            sl += sls;
            x += (((vx - x) >> 1) * (sl >> 1)) >> 13;
            y += (((vy - y) >> 1) * (sl >> 1)) >> 13;
        }
        if (use_m || use_b) {
            int32_t ms, mc;
            mm += minc;
            orb_sincos(mm, &ms, &mc);
            if (use_m) {
                mr += mrs;
                x += (mr * mc) >> 15;
                y += (mr * ms) >> 15;
            }
            if (use_b) {                    /* BEAM: a narrow pulse of light at the moon's phase, (1 + cos)/2 to the 4th */
                int32_t q = (mc + 32768) >> 1, g;
                bq += bqs;
                q = (q * q) >> 15;
                q = (q * q) >> 15;
                g = 32767 - (((32767 - q) * bq) >> 15);
                x = ((x >> 1) * g) >> 14;   /* (halved: x spans ~3 units) */
                y = ((y >> 1) * g) >> 14;
            }
        }
        if (use_d) {                        /* dust: two noises, coloured, crossfaded in */
            int32_t nx, ny, ox, oy;
            rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
            nx = (int32_t)(int16_t)rng;
            ny = (int32_t)(int16_t)(rng >> 16);
            n1 += ((nx - n1) * kc) >> 15;
            n2 += ((ny - n2) * kc) >> 15;
            ox = hp ? nx - n1 : n1;
            oy = hp ? ny - n2 : n2;
            dq += dqs;
            x += (((ox - x) >> 1) * dq) >> 14;   /* (halved: x - ox spans ~3 units) */
            y += (((oy - y) >> 1) * dq) >> 14;
        }
        if (use_p) { x &= pm; y &= pm; }
        cl += dcl; sL += dsl; cr += dcr; sR += dsr;
        L = (x * cl + y * sL) >> 15;        /* Q12 */
        R = (x * cr + y * sR) >> 15;
        L = (L * a) >> 12;                  /* Q15 */
        R = (R * a) >> 12;
        bd[2u * i] += L;
        bd[2u * i + 1u] += R;
        if (use_o) {                        /* this voice's share of the tide (the others feel it next block) */
            int32_t m = (L + R) >> 1;
            ow[i] = m;
            tn[i] += m;
        }
        if (use_s) {
            bs[2u * i] += (L * send) >> 15;
            bs[2u * i + 1u] += (R * send) >> 15;
        }
    }
    v->M = M; v->mm = mm; v->rng = rng; v->e = e; v->a = a; v->sl = sl; v->nl1 = n1; v->nl2 = n2;
    v->cl = cl; v->sl_ = sL; v->cr = cr; v->sr = sR;
    v->mr = use_m ? mr : mr + mrs * (int32_t)ORB_SUB; v->dq = use_d ? dq : dq + dqs * (int32_t)ORB_SUB;   /* (unused: at once) */
    v->bq = use_b ? bq : bq + bqs * (int32_t)ORB_SUB; v->tk = use_t ? tk : tk + tks * (int32_t)ORB_SUB;
}

/* ---- the orrery */
static uint32_t orb_step_ticks(const orb_ring_t *r) { return ORB_RATIO_TICKS[r->ratio % ORB_NRATIO]; }

/* a step as this lap plays it: 0 rest, 1 hit, 2 accent; *dg its degree. The pattern as written, but where MUTATE varies it */
static uint32_t orb_step_on(const orb_t *o, uint32_t r, uint32_t i, int32_t *dg)
{
    const orb_rstate_t *R = &o->rs[r];
    const orb_ring_t *g = &o->sys->ring[r];
    i &= ORB_NSTEP - 1u;
    if (R->vm[i >> 5] >> (i & 31u) & 1u) { if (dg) *dg = R->vdg[i]; return R->von[i]; }
    if (dg) *dg = g->dg[i];
    return g->on[i];
}

/* MUTATE, at a ring's lap: the next lap's variation of the pattern, drawn afresh from the pattern as written (so it never
 * drifts away from it, and the pattern itself is never written). Some hits move to a free neighbour, some degrees go up
 * or down by 1 + VAR/25. MUTATE (+ CHAOS / 2) 0..99: how many (1 .. a quarter of the ring) and how likely each */
static void orb_vary(orb_t *o, uint32_t r)
{
    orb_rstate_t *R = &o->rs[r];
    const orb_sys_t *sy = o->sys;
    const orb_ring_t *g = &sy->ring[r];
    uint32_t len = g->len < 1u ? 1u : g->len > ORB_NSTEP ? ORB_NSTEP : g->len, amt, n, k;
    int32_t span = 1 + (int32_t)sy->var / 25;
    R->vm[0] = R->vm[1] = 0;
    amt = (uint32_t)sy->mutate + sy->macro[MAC_CHAOS] / 2u;
    if (amt > 99u) amt = 99u;
    if (!amt) return;
    n = 1u + amt * len / 400u;
    for (k = 0; k < 16u; k++) {             /* (16 draws at most, whatever the ring) */
        uint32_t x = orb_xs(&o->vrng), i = x % len, j;
        if (k >= n || (x >> 8) % 100u >= amt || !g->on[i] || (R->vm[i >> 5] >> (i & 31u) & 1u)) continue;
        if ((x >> 16) & 1u) {               /* the hit moves to a free neighbour */
            j = ((x >> 17) & 1u) ? (i + 1u) % len : (i + len - 1u) % len;
            if (g->on[j] || (R->vm[j >> 5] >> (j & 31u) & 1u)) continue;
            R->von[i] = 0;
            R->von[j] = g->on[i];
            R->vdg[j] = g->dg[i];
            R->vm[i >> 5] |= 1u << (i & 31u);
            R->vm[j >> 5] |= 1u << (j & 31u);
        } else {                            /* its degree steps */
            R->von[i] = g->on[i];
            R->vdg[i] = (int8_t)orb_clamp(g->dg[i] + (int32_t)((x >> 18) % (uint32_t)(2 * span + 1)) - span, -14, 21);
            R->vm[i >> 5] |= 1u << (i & 31u);
        }
    }
}
static uint32_t orb_gate_frames(const orb_sys_t *s, uint32_t ticks, uint32_t gate)   /* ticks * gate% -> frames */
{
    uint32_t bpm = s->bpm < 40u ? 40u : s->bpm;
    return ticks * (gate ? gate : 1u) * 21u / (8u * bpm);   /* 44100 / (168 bpm) frames a tick, / 100 */
}

static void orb_ring_fire(orb_t *o, uint32_t r, int32_t pitch, int32_t vel)
{
    orb_rstate_t *R = &o->rs[r];
    const orb_ring_t *g = &o->sys->ring[r];
    const orb_snd_t *s = &o->sys->snd[ORB_A + r];
    uint32_t gf = orb_gate_frames(o->sys, orb_step_ticks(g), s->gate);
    orb_trigger(o, r, ORB_A + r, pitch, vel, (int32_t)(gf < 32u ? 32u : gf), R->last_p);
    R->last_p = pitch;
    R->flash++;
    o->lastvel = vel;
}

/* one sub-block of the clock: the rings that reach a step, then their logic, then the hits */
static void orb_clock(orb_t *o)
{
    orb_sys_t *sy = o->sys;
    uint32_t r, stepping = 0, tick[ORB_NRING];
    int32_t sw = sy->swing;
    uint32_t nconj = 0;
    for (r = 0; r < ORB_NRING; r++) {      /* pass 1: who steps now, and onto what */
        orb_rstate_t *R = &o->rs[r];
        orb_ring_t *g = &sy->ring[r];
        uint32_t len = g->len < 1u ? 1u : g->len > ORB_NSTEP ? ORB_NSTEP : g->len, idx;
        R->fired = 0;
        /* FRACT's ratchets due */
        if (R->rat_left && (int32_t)(o->T - R->rat_next) >= 0) {
            orb_ring_fire(o, r, R->rat_pitch, R->rat_vel);
            R->rat_left--;
            R->rat_next += R->rat_dt;
        }
        if ((int32_t)(o->T - R->next) < 0) continue;
        stepping |= 1u << r;
        tick[r] = R->base;
        if (R->pos >= len) R->pos = 0;
        switch (g->dir % ORB_NDIR) {
        case DIR_REV: idx = len - 1u - R->pos; break;
        case DIR_PING: {
            uint32_t per = len > 1u ? 2u * len - 2u : 1u, p = R->k % per;
            idx = p < len ? p : per - p;
            break;
        }
        case DIR_SPIRAL: idx = (R->pos + R->spin) % len; break;
        case DIR_RND: idx = orb_xs(&o->rng) % len; break;
        default: idx = R->pos; break;
        }
        R->cur = (uint8_t)idx;
        R->lit = orb_step_on(o, r, idx, 0) != 0;
        R->k++;
        if (++R->pos >= len) { R->pos = 0; R->laps++; R->spin = (R->spin + 1u) % len; orb_vary(o, r); }
        R->base += orb_step_ticks(g);
        R->next = R->base;
        if (sw && (R->k & 1u)) R->next += orb_step_ticks(g) * (uint32_t)sw / 200u;   /* odd steps later */
        if (sy->hum) R->next += (orb_xs(&o->rng) % 1000u) * orb_step_ticks(g) * sy->hum / 400000u;
    }
    if (!stepping) return;
    for (r = 0; r < ORB_NRING; r++) {      /* pass 2: the logic, the conjunctions, the hits */
        orb_rstate_t *R = &o->rs[r];
        orb_ring_t *g = &sy->ring[r];
        uint32_t t = g->ltgt % ORB_NRING, q, conj = 0, fire, prob;
        int32_t vel, pitch, deg, dg;
        if (!(stepping >> r & 1u)) continue;
        for (q = 0; q < ORB_NRING; q++)
            if (q != r && (stepping >> q & 1u) && o->rs[q].lit && tick[q] == tick[r] && !sy->ring[q].mute) conj++;
        switch (g->logic % ORB_NLOGIC) {
        case LOG_AND: fire = R->lit && o->rs[t].lit; break;
        case LOG_NOT: fire = R->lit && !o->rs[t].lit; break;
        case LOG_XOR: fire = R->lit != o->rs[t].lit; break;
        case LOG_CONJ: fire = R->lit && conj; break;
        default: fire = R->lit; break;
        }
        prob = g->prob;
        prob = (uint32_t)orb_clamp((int32_t)prob - (int32_t)sy->macro[MAC_CHAOS] / 4 + (o->md[DST_PROB] * 99 >> 15), 0, 99);
        if (fire && prob < 99u && orb_xs(&o->rng) % 100u > prob) fire = 0;
        if (!fire || g->mute) continue;
        R->fired = 1;
        /* velocity: a hit 0.7, an accent 1.0, each other ring in the conjunction +0.15; HUM */
        vel = orb_step_on(o, r, R->cur, &dg) == 2u ? 32767 : 22937;
        vel += (int32_t)conj * 4915;
        if (sy->hum) vel -= (int32_t)(orb_xs(&o->rng) % 6000u) * sy->hum / 99;
        vel = orb_clamp(vel, 3000, 32767);
        deg = g->deg + dg;
        pitch = (36 + (int32_t)(sy->sun % 12u)) * 256 + orb_degree(sy, deg);
        orb_ring_fire(o, r, pitch, vel);
        if (conj) nconj++;
        /* FRACT: a hit may split into 2..4 */
        R->rat_left = 0;
        if (sy->macro[MAC_FRACT] && orb_xs(&o->rng) % 100u < sy->macro[MAC_FRACT] / 2u) {
            uint32_t n = 2u + orb_xs(&o->rng) % (1u + sy->macro[MAC_FRACT] / 40u);
            R->rat_dt = orb_step_ticks(g) / n;
            R->rat_next = tick[r] + R->rat_dt;
            R->rat_left = n - 1u;
            R->rat_pitch = pitch;
            R->rat_vel = vel * 3 / 4;
        }
    }
    if (nconj) { o->conj = 32767; o->conj_count++; }   /* the flare (CONJ), and the screen's count */
}

static void orb_restart(orb_t *o)          /* the clock from the top: every ring at step 0, now */
{
    uint32_t r;
    o->T = 0;
    o->Tf = 0;
    for (r = 0; r < ORB_NRING; r++) {
        orb_rstate_t *R = &o->rs[r];
        R->base = 0; R->next = 0; R->k = 0; R->pos = 0; R->spin = 0; R->rat_left = 0; R->lit = 0;
        R->vm[0] = R->vm[1] = 0;            /* (the first lap as written) */
        R->last_p = INT32_MIN;
    }
}

/* ---- modulation: LFOs, the orbit pad, the sources, the matrix */
static uint32_t orb_sync_phase(uint32_t T, uint32_t idx)
{
    uint32_t P = (uint32_t)ORB_SYNC_STEPS[idx % ORB_NSYNC] * ORB_TPS;
    return (T % P) * (0xFFFFFFFFu / P);
}
static int32_t orb_shape(uint32_t ph, uint32_t shape, int32_t sh, int32_t sh0)
{
    switch (shape) {
    case LS_TRI: { int32_t t = (int32_t)(ph >> 16); return t < 32768 ? t * 2 - 32767 : 98303 - t * 2 > 32767 ? 32767 : 98303 - t * 2; }
    case LS_SAW: return (int32_t)(ph >> 16) - 32768;
    case LS_SQR: return (int32_t)ph < 0 ? -32767 : 32767;
    case LS_SH: return sh;
    case LS_DRIFT: return sh0 + (int32_t)(((int64_t)(sh - sh0) * (int32_t)(ph >> 17)) >> 15);
    default: return orb_sin(ph);
    }
}
static void orb_mods(orb_t *o)
{
    orb_sys_t *sy = o->sys;
    uint32_t k;
    int32_t rad, X, Y, *src = o->src;
    for (k = 0; k < 2u; k++) {
        uint32_t ph0 = o->lph[k], ph;
        ph = sy->lsync[k] ? orb_sync_phase(o->T, sy->lrate[k]) : ph0 + ORB_LFO[sy->lrate[k] % 100u];
        if (ph < ph0 && ph0 - ph > 0x80000000u) {   /* a new cycle: S&H takes a value */
            o->lsh0[k] = o->lsh[k];
            o->lsh[k] = (int32_t)(orb_xs(&o->rng) >> 16) - 32768;
        }
        o->lph[k] = ph;
        o->lval[k] = orb_shape(ph, sy->lshape[k] % ORB_NLSHAPE, o->lsh[k], o->lsh0[k]);
    }
    /* the orbit pad: a point circling (X, Y) at RAD (+ ORBIT, RAD), once in its period */
    o->orbph = orb_sync_phase(o->T, sy->orate);
    rad = (int32_t)sy->orad * 100 + (int32_t)sy->macro[MAC_ORBIT] * 40 + ((o->md[DST_RAD] * 50) >> 15) * 100;
    rad = orb_clamp(rad, 0, 5000);
    X = (int32_t)sy->ox * 100 + ((rad * orb_cos(o->orbph)) >> 15);
    Y = (int32_t)sy->oy * 100 + ((rad * orb_sin(o->orbph)) >> 15);
    X = orb_clamp(X, 0, 9900);
    Y = orb_clamp(Y, 0, 9900);
    o->orb_x = X;
    o->orb_y = Y;
    src[SRC_OFF] = 0;
    src[SRC_LFO1] = o->lval[0];
    src[SRC_LFO2] = o->lval[1];
    src[SRC_ORBX] = (X - 4950) * 6;
    src[SRC_ORBY] = (Y - 4950) * 6;
    src[SRC_RND] = o->rnd;
    {
        const orb_ring_t *a = &sy->ring[0];
        uint32_t len = a->len ? a->len : 1u;
        src[SRC_STEP] = (int32_t)(o->rs[0].pos * 65535u / len) - 32768;
    }
    src[SRC_CONJ] = o->conj;
    src[SRC_VEL] = o->lastvel;
    src[SRC_MODW] = o->modw;
    src[SRC_AFT] = o->aft;
    o->conj -= o->conj >> 7;                /* the flare decays (~90 ms) */
    for (k = 0; k < ORB_NDST; k++) o->md[k] = 0;
    for (k = 0; k < ORB_NMOD; k++) {
        const orb_mod_t *m = &sy->mod[k];
        if (m->src && m->src < ORB_NSRC && m->dst < ORB_NDST && m->amt)
            o->md[m->dst] += (src[m->src] * (int32_t)m->amt) / 99;
    }
    for (k = 0; k < ORB_NDST; k++) o->md[k] = orb_clamp(o->md[k], -32767, 32767);
}

/* ---- FX: the filter map, SPACE, the end */
static void orb_fx_tick(orb_t *o)
{
    const orb_sys_t *sy = o->sys;
    int32_t ci, c, g, g0, g1, k, den, a1, res, m;
    ci = (int32_t)sy->cut * 331 + sy->macro[MAC_SPECTRAL] * 100 + (o->md[DST_CUT] >> 0);   /* Q15 of the 128 steps */
    ci = orb_clamp(ci, 0, 32767);
    c = ci >> 8;                            /* 0..127 */
    g0 = ORB_GCUT[c];
    g1 = ORB_GCUT[c + 1];
    g = g0 + (int32_t)(((int64_t)(g1 - g0) * (ci & 255)) >> 8);
    res = orb_clamp((int32_t)sy->res * 331 + o->md[DST_RES], 0, 32767);
    k = (1 << 25) - (int32_t)(((int64_t)res * 31 << 20) >> 15);   /* 2.0 .. 0.0625 (Q24) */
    den = (1 << 24) + (int32_t)(((int64_t)g * (g + k)) >> 24);
    a1 = (1 << 30) / ((den >> 8) > 0 ? (den >> 8) : 1);       /* Q14 */
    o->f_a1 = a1 << 14;                                       /* Q28 */
    o->f_a2 = (int32_t)(((int64_t)g * o->f_a1) >> 24);
    o->f_a3 = (int32_t)(((int64_t)g * o->f_a2) >> 24);
    o->f_k = k;
    m = orb_clamp((int32_t)sy->fmode * 662 + o->md[DST_MODE] * 2, 0, 65535);
    o->f_m = m;
    o->f_in = 32767 - (res >> 1);           /* resonance: the input down to -6 dB */
    o->f_on = !(sy->cut >= 97u && sy->res < 4u && sy->fmode < 3u && !sy->drive && !o->md[DST_CUT] && !o->md[DST_RES] &&
                !o->md[DST_MODE] && !sy->macro[MAC_SPECTRAL]);
    /* the delay's time: the beat fraction at the tempo, smoothed (Q8) */
    {
        uint32_t bpm = sy->bpm < 40u ? 40u : sy->bpm, i = sy->dtime % ORB_NDT;
        int32_t t = (int32_t)((2646000u / bpm) * ORB_DT_NUM[i] / ORB_DT_DEN[i]);   /* frames */
        t = orb_clamp(t, 64, (int32_t)ORB_DLEN - 4);
        if (!o->d_cur) o->d_cur = t << 8;
        o->d_cur += orb_rz((t << 8) - o->d_cur, 6);
    }
    /* HORIZON: the depth (its macro, HORIZON), how wet, how fast the source still moves, the shift, the grain */
    {
        int32_t d = orb_clamp(orb_lin(sy->macro[MAC_HORIZON]) + o->md[DST_HORIZON], 0, 32767), sh, i, f;
        uint32_t G = 256u + (uint32_t)sy->hgrain * 39u;
        if (G > 4096u) G = 4096u;
        if (d && !o->h_on) {                /* falling in: the grains start, staggered by half */
            o->h_P = (o->h_w - 3u * G) << 16;
            o->h_t[0] = 0;
            o->h_t[1] = (int32_t)G / 2;
            o->h_wph[1] = 0x40000000u;
            o->h_pos[1] = o->h_P;
            o->h_dir[1] = 1;
        }
        o->h_depth = d;
        o->h_level = (d * 99) >> 15;
        {   /* all wet a quarter of the way in; the mix glides there (~10 ms), out too */
            int32_t mt = d >= 8192 ? 32767 : d * 4;
            o->h_mix += mt > o->h_mix ? (mt - o->h_mix + 7) >> 3 : -((o->h_mix - mt + 7) >> 3);
            if (o->h_mix < 0) o->h_mix = 0;
            o->h_on = d || o->h_mix;
        }
        o->h_v = 65536 - (d * 2);                               /* the source slows to a standstill */
        sh = (int32_t)orb_clamp(sy->hshift, -24, 24) * 256 * (d >> 3) / 4096 + 24 * 256;   /* Q8, 0 .. 48 semitones */
        i = sh >> 8;
        f = sh & 255;
        if (i >= 48) { i = 47; f = 256; }
        o->h_rate = ORB_SEMI[i] + (uint32_t)(((int32_t)(ORB_SEMI[i + 1] - ORB_SEMI[i]) * f) >> 8);
        o->h_G = G;
        o->h_wi = 0x80000000u / G;
    }
    /* NEBULA: line lengths by size, the decay, the damping */
    {
        static const int16_t BASE[4] = {1499, 1889, 2381, 2999};   /* (at 22 kHz: 68 .. 136 ms) */
        int32_t sz = 11000 + (int32_t)sy->rsize * 220, i;   /* Q15: 0.34 .. 1.0 */
        for (i = 0; i < 4; i++) {           /* a new size: the lines glide there, a frame a tick (a jump would click) */
            int32_t tl = orb_clamp((BASE[i] * sz) >> 15, 200, (int32_t)ORB_RLEN - 16);
            if (!o->r_len[i]) o->r_len[i] = tl;
            else o->r_len[i] += tl > o->r_len[i] ? 1 : tl < o->r_len[i] ? -1 : 0;
        }
        o->r_g = 16384 + (int32_t)sy->rdecay * 158;          /* 0.5 .. 0.977 */
        o->r_damp = 4000 + (99 - (int32_t)sy->rdamp) * 290;  /* the lines' low-pass */
        /* BLOOM: its own light back into it: the lines' feedback up towards 0.985 (much longer), and 0.01 of its wet
         * again through the diffusers (denser); the two never sum past 0.995, so it is stable whatever the rest. A sun
         * that moves clears it a moment (r_q 0.55 -> 1 in ~0.6 s), so the old chord's tail does not clash */
        if (sy->bloom) o->r_g += (32276 - o->r_g) * (int32_t)sy->bloom / 99;
        o->r_b = 327 * (int32_t)sy->bloom / 99;
        if (!o->r_q) { o->r_q = 32767; o->r_sun = sy->sun; }   /* (at the start: whole) */
        if (sy->sun != o->r_sun) { o->r_sun = sy->sun; if (sy->bg_level || sy->bloom) o->r_dip = 1; }
        if (o->r_dip) { o->r_q -= 220; if (o->r_q <= 18000) { o->r_q = 18000; o->r_dip = 0; } }   /* down in ~50 ms */
        else if (o->r_q < 32767) o->r_q = o->r_q + 24 > 32767 ? 32767 : o->r_q + 24;           /* back in ~0.6 s */
        o->r_ge = o->r_q >= 32767 ? o->r_g : (o->r_g * o->r_q) >> 15;
        o->r_be = o->r_q >= 32767 ? o->r_b : (o->r_b * o->r_q) >> 15;
        o->r_cut = sy->bg_cut ? 190 + (int32_t)sy->bg_cut * 37 : 0;   /* one pole, 20 .. 416 Hz at 22 kHz (Q15) */
    }
    /* TIRED LIGHT */
    o->z_rate = ORB_TIRED_RATE[sy->tired % ORB_NTIRED];
    /* BACKGROUND RADIATION: the sun's harmonic series (the acoustic peaks of the first light: a harmonic series, odd
     * peaks taller with baryons, the high ones damped by photon diffusion, Silk's), from every side, its tiny
     * fluctuations drifting; it swells in while the orrery plays and fades when it stops */
    {
        static const uint8_t NUM[ORB_NTUNING][ORB_NBG] = {{1, 2, 3, 4, 5, 6, 7, 8}, {1, 2, 3, 4, 5, 6, 7, 8},
            {3, 4, 6, 8, 12, 16, 0, 0}, {1, 2, 3, 4, 5, 6, 8, 10}, {1, 2, 3, 4, 5, 6, 8, 10}};   /* (UTONAL: of a third) */
        static const uint8_t DEN[ORB_NTUNING] = {1, 1, 3, 1, 1};
        static const int16_t TILT[17] = {0, 32767, 23170, 18918, 16384, 14654, 13377, 12385, 11585, 10923, 10362,
                                         9880, 9459, 9088, 8757, 8460, 8192};   /* 1 / sqrt(n) */
        int32_t target = sy->bg_level && o->playing ? 32767 : 0, de = target - o->bg_env, st = orb_rz(de, 11);
        uint32_t k, live = 0;
        if (!st && de) st = de > 0 ? (de < 16 ? de : 16) : (de > -16 ? de : -16);   /* (the last of it: never past) */
        o->bg_env += st;                          /* (~1.5 s in, ~1.5 s out) */
        for (k = 0; k < ORB_NBG; k++) live |= (uint32_t)o->bg_a[k];
        o->bg_on = o->bg_env > 0 || live;
        if (!o->bg_on) { o->bg_ol[0] = o->bg_ol[1] = o->bg_or[0] = o->bg_or[1] = 0; }
        if (o->bg_on) {
            uint32_t t = sy->tuning % ORB_NTUNING, fi, cinc;
            int32_t temp = orb_clamp(sy->bg_temp, -2, 1), lvl = orb_lin(sy->bg_level), sl, bar, an;
            fi = orb_inc((48 + 12 * temp + (int32_t)(sy->sun % 12u)) * 256) * 2u / DEN[t];   /* C3 + the sun, half rate */
            cinc = (uint32_t)(20 + (int32_t)sy->bg_cut * 4) * 194783u;   /* the low cut's frequency, as a step */
            lvl = (int32_t)(((lvl * lvl) >> 15) * (o->bg_env >> 0) >> 15);
            sl = (int32_t)sy->bg_silk * sy->bg_silk / 40;
            bar = (int32_t)sy->bg_baryon * 331;           /* Q15 */
            an = (int32_t)sy->bg_aniso * 200;             /* the fluctuations' depth, up to ~0.6 */
            o->bg_glow = orb_lin(sy->bg_glow);
            if ((o->bg_k & 127u) == 0u) {         /* a new fluctuation for one partial each 93 ms */
                uint32_t q = (o->bg_k >> 7) % ORB_NBG;
                o->bg_ft[q] = (int32_t)(orb_xs(&o->rng) >> 17) - 16384;
            }
            o->bg_k++;
            for (k = 0; k < ORB_NBG; k++) {
                uint32_t n = NUM[t][k], tinc = fi * n;
                int32_t a, d;
                o->bg_fl[k] += orb_rz((o->bg_ft[k] - o->bg_fl[k]), 10);
                o->bg_pan[k] += 90000u + k * 31000u;       /* each drifts round (~0.03 .. 0.06 Hz) */
                if ((o->bg_k & 3u) == (k & 3u) || !o->bg_gl[k]) {   /* (two a tick: they move slowly) */
                    uint32_t th = 0x20000000u + (uint32_t)(orb_sin(o->bg_pan[k] + k * 0x3A000000u) * 0x3000);
                    o->bg_gl[k] = orb_cos(th);    /* from every side: its pan swings +-34 degrees about the middle */
                    o->bg_gr[k] = orb_sin(th);
                }
                if (!n) a = 0;
                else {
                if (!o->bg_inc[k]) o->bg_inc[k] = tinc;
                o->bg_inc[k] += (uint32_t)orb_rz((int32_t)(tinc - o->bg_inc[k]), 9);   /* a moved sun: it glides */
                if ((o->bg_k & 7u) == k || !o->bg_g[k]) {   /* its shape: one partial a tick (they move with a knob) */
                    a = TILT[n > 16u ? 16u : n];
                    a = (a * (int32_t)(2147418112u / (65536u + n * n * (uint32_t)sl))) >> 15;   /* Silk: the high damped */
                    d = n & 1u ? 32767 + bar / 2 : 32767 - (bar * 4) / 5;   /* baryons: the odd peaks taller */
                    a = (int32_t)(((int64_t)a * d) >> 15);
                    if (sy->bg_cut) {             /* the low cut on each partial: (f/fc)^2 / (1 + (f/fc)^2) */
                        uint32_t r = cinc / ((tinc >> 8) | 1u);   /* fc / f, Q8 */
                        a = (int32_t)(((int64_t)a * (int32_t)(2147418112u / (65536u + r * r))) >> 15);
                    }
                    o->bg_g[k] = a > 0 ? a : 1;
                }
                a = (int32_t)(((int64_t)o->bg_g[k] * (32768 + ((o->bg_fl[k] * an) >> 15))) >> 15);
                a = (int32_t)(((int64_t)a * lvl) >> 17);   /* (8 of them: a quarter each at most, then the level) */
                }
                d = a - o->bg_a[k];
                if (d < 16 && d > -16) { o->bg_a[k] = a; o->bg_as[k] = 0; }   /* (the last of a ramp: there) */
                else o->bg_as[k] = orb_rz(d, 4);  /* (16 half-rate frames a tick: there exactly, never past) */
            }
        }
    }
}

ORB_INL int32_t orb_soft(int32_t x)   /* a cubic: unity at 0, 1.0 at +-1.5, flat after */
{
    uint32_t au;
    int32_t c;
    if (x > 49151) x = 49151;
    if (x < -49151) x = -49151;
    au = (uint32_t)(x < 0 ? -x : x);
    c = (int32_t)(((((au * au) >> 15) * au) >> 15) * 4855u >> 15);
    return x < 0 ? x + c : x - c;
}

static void orb_fx_render(orb_t *o, int32_t *out)
{
    const orb_sys_t *sy = o->sys;
    orb_mem_t *mm = o->mem;
    uint32_t i;
    int32_t fb = orb_clamp((int32_t)sy->dfb * 323 + o->md[DST_FDBK], 0, 31000);
    int32_t tone = 3000 + (int32_t)sy->dtone * 300, dmix = orb_lin(sy->dmix), rmix = orb_lin(sy->rmix);
    int32_t drive = 4096 + (int32_t)sy->drive * 290;        /* Q12: 1 .. 8 */
    int ping = sy->ping != 0;
    for (i = 0; i < ORB_SUB; i++) {
        int32_t sL = o->buss[2u * i], sR = o->buss[2u * i + 1u], L = o->busd[2u * i], R = o->busd[2u * i + 1u];
        int32_t dlo, dro, inL, inR, rin, y[4];
        uint32_t w;
        /* the delay: read (interpolated), feed back through its tone, write */
        {
            uint32_t dt = (uint32_t)o->d_cur, di = dt >> 8, df = dt & 255u, ri = (o->d_w - di) & (ORB_DLEN - 1u);
            uint32_t ri1 = (ri - 1u) & (ORB_DLEN - 1u);
            dlo = mm->dl[ri] + (((mm->dl[ri1] - mm->dl[ri]) * (int32_t)df) >> 8);
            dro = mm->dr[ri] + (((mm->dr[ri1] - mm->dr[ri]) * (int32_t)df) >> 8);
            o->d_lp[0] += orb_rz((dlo - o->d_lp[0]) * tone, 15);
            o->d_lp[1] += orb_rz((dro - o->d_lp[1]) * tone, 15);
            {
                int32_t fl = o->d_lp[0], fr = o->d_lp[1];
                if (o->z_rate) {    /* TIRED LIGHT: what goes round again is read slower than it was written (two taps
                                     * half a window apart, each a triangle: together one), so each echo comes back a
                                     * just interval lower than the last, and through the tone, dimmer: redder */
                    uint32_t W = 2048u << 16, q, base = di > ORB_DLEN - 2056u ? ORB_DLEN - 2056u : di;
                    int32_t za = 0, zb = 0;
                    for (q = 0; q < 2u; q++) {
                        uint32_t pz = q ? (o->z_p + (W >> 1)) & (W - 1u) : o->z_p, off = base + (pz >> 16), zf = (pz >> 8) & 255u;
                        uint32_t i0 = (o->d_w - off) & (ORB_DLEN - 1u), i1 = (i0 - 1u) & (ORB_DLEN - 1u);
                        int32_t w = (int32_t)((pz >> 16) < 1024u ? (pz >> 16) : 2048u - (pz >> 16)) * 32;   /* Q15 */
                        za += ((mm->dl[i0] + (((mm->dl[i1] - mm->dl[i0]) * (int32_t)zf) >> 8)) * w) >> 15;
                        zb += ((mm->dr[i0] + (((mm->dr[i1] - mm->dr[i0]) * (int32_t)zf) >> 8)) * w) >> 15;
                    }
                    o->z_lp[0] += orb_rz((za - o->z_lp[0]) * tone, 15);
                    o->z_lp[1] += orb_rz((zb - o->z_lp[1]) * tone, 15);
                    fl = o->z_lp[0];
                    fr = o->z_lp[1];
                    o->z_p = (o->z_p + o->z_rate) & (W - 1u);
                }
                if (ping) { inL = ((sL + sR) >> 1) + orb_rz(fr * fb, 15); inR = orb_rz(fl * fb, 15); }
                else { inL = sL + orb_rz(fl * fb, 15); inR = sR + orb_rz(fr * fb, 15); }
            }
            mm->dl[o->d_w] = (int16_t)orb_clamp(inL, -32767, 32767);
            mm->dr[o->d_w] = (int16_t)orb_clamp(inR, -32767, 32767);
            o->d_w = (o->d_w + 1u) & (ORB_DLEN - 1u);
        }
        /* NEBULA at half the rate (22 kHz: a damped tail has nothing above 11 kHz to lose): the odd frame runs two
         * allpasses on the pair's sum, four lines and a Hadamard mix; the output steps half-way each frame */
        o->r_acc += (sL + sR + ((dlo + dro) >> 1)) >> 2;   /* (the lines run hot: their tail's last bits are its own) */
        if (i & 1u) {
            static const int16_t APL[2] = {120, 278};
            uint32_t k;
            int32_t h0, h1, h2, h3;
            rin = o->r_acc;
            o->r_acc = 0;
            if (o->bg_on) {                 /* BACKGROUND RADIATION: its partials, each panned; its glow into NEBULA */
                int32_t bl = 0, br = 0;
                for (k = 0; k < ORB_NBG; k++) {
                    int32_t sv;
                    if (!o->bg_a[k] && !o->bg_as[k]) continue;   /* (a partial damped away, or the tuning's unused) */
                    sv = (orb_sin(o->bg_ph[k]) * o->bg_a[k]) >> 15;
                    o->bg_ph[k] += o->bg_inc[k];
                    o->bg_a[k] += o->bg_as[k];
                    bl += (sv * o->bg_gl[k]) >> 15;
                    br += (sv * o->bg_gr[k]) >> 15;
                }
                rin += (((bl + br) >> 1) * o->bg_glow) >> 15;
                o->bg_ol[0] = o->bg_ol[1]; o->bg_ol[1] = bl;
                o->bg_or[0] = o->bg_or[1]; o->bg_or[1] = br;
            }
            if (o->r_be) rin += orb_rz(o->r_wet * o->r_be, 15);   /* BLOOM: its own light back in */
            if (o->r_cut) { o->r_hp += orb_rz((rin - o->r_hp) * o->r_cut, 15); rin -= o->r_hp; }   /* the low cut */
            rin = orb_clamp(rin, -65534, 65534);
            for (k = 0; k < 2u; k++) {
                int32_t rd = mm->ap[k][((uint32_t)o->r_apw[k] - (uint32_t)APL[k]) & (ORB_APLEN - 1u)];
                int32_t wv = rin + orb_rz(rd * 20000, 15);
                rin = rd - orb_rz(wv * 20000, 15);
                mm->ap[k][o->r_apw[k]] = (int16_t)orb_clamp(wv, -32767, 32767);
                o->r_apw[k] = (int32_t)(((uint32_t)o->r_apw[k] + 1u) & (ORB_APLEN - 1u));
            }
            w = o->r_w;
            for (k = 0; k < 4u; k++) {
                uint32_t ri = (w - (uint32_t)o->r_len[k]) & (ORB_RLEN - 1u);
                int32_t yv;
                if (k == 0u) {              /* a line swims (a slow LFO, +-3 frames): the tail never stands still */
                    int32_t mo = orb_sin(o->r_lfo) * 3, mi = mo >> 15, mf = (mo & 0x7FFF) >> 7;
                    uint32_t r0 = (ri - (uint32_t)(mi + 4)) & (ORB_RLEN - 1u), r1 = (r0 - 1u) & (ORB_RLEN - 1u);
                    yv = mm->rl[k][r0] + (((mm->rl[k][r1] - mm->rl[k][r0]) * mf) >> 8);
                } else
                    yv = mm->rl[k][ri];
                o->r_lp[k] += orb_rz((yv - o->r_lp[k]) * o->r_damp, 15);
                y[k] = o->r_lp[k];
            }
            h0 = y[0] + y[1]; h1 = y[0] - y[1]; h2 = y[2] + y[3]; h3 = y[2] - y[3];
            y[0] = h0 + h2; y[1] = h1 + h3; y[2] = h0 - h2; y[3] = h1 - h3;   /* x 2 */
            for (k = 0; k < 4u; k++) {
                int64_t v = (int64_t)y[k] * o->r_ge;   /* (Hadamard / 2) * g, rounded once, toward zero */
                int32_t wv = (int32_t)((v + (v < 0 ? 65535 : 0)) >> 16) + rin;
                if (wv > 24576 || wv < -24576) {   /* a full nebula saturates, warmly: a soft knee, never a hard clip */
                    int32_t dk = (wv < 0 ? -wv : wv) - 24576;
                    dk = 24576 + dk * 8192 / (8192 + dk);
                    wv = wv < 0 ? -dk : dk;
                }
                mm->rl[k][w] = (int16_t)orb_clamp(wv, -32767, 32767);
            }
            o->r_w = (w + 1u) & (ORB_RLEN - 1u);
            o->r_lfo += 48000u;             /* ~0.25 Hz */
            o->r_ol[0] = o->r_ol[1];
            o->r_or[0] = o->r_or[1];
            o->r_ol[1] = ((o->r_lp[0] + o->r_lp[2]) * rmix) >> 16;   /* (the lines at twice: half out) */
            o->r_or[1] = ((o->r_lp[1] + o->r_lp[3]) * rmix) >> 16;
            o->r_wet = (o->r_lp[0] + o->r_lp[1] + o->r_lp[2] + o->r_lp[3]) >> 2;
            L += (o->r_ol[0] + o->r_ol[1] + o->bg_ol[0] + o->bg_ol[1]) >> 1;
            R += (o->r_or[0] + o->r_or[1] + o->bg_or[0] + o->bg_or[1]) >> 1;
        } else {
            L += o->r_ol[1] + o->bg_ol[1];
            R += o->r_or[1] + o->bg_or[1];
        }
        L += (dlo * dmix) >> 15;
        R += (dro * dmix) >> 15;
        /* HORIZON: the sound falls into a black hole. A memory of the mix (frozen when deep), a source point that
         * slows to a standstill, two grains about it (triangles staggered by half: they sum to one), read at the
         * shifted rate, some backwards (RETRO); R is the wet 151 frames later (a short delay: the width, cheaply) */
        {
            int32_t m = (L + R) >> 1;
            if (o->h_depth < 27000) {
                mm->hb[o->h_w] = (int16_t)orb_clamp(m, -32767, 32767);
                o->h_w = (o->h_w + 1u) & (ORB_HLEN - 1u);
            }
            if (o->h_on) {
                uint32_t g, G = o->h_G, half = (o->h_rate >> 1) * (G >> 4) >> 12, lag, lo, hi;   /* rate * G / 2 */
                int32_t wl = 0, wr = 0;
                lo = half + G + 256u;           /* a forward grain at the fastest rate never reaches the write head */
                hi = ORB_HLEN - G - half - 512u;   /* a RETRO grain, going back while the write goes on, never laps it */
                if (hi < lo) hi = lo;
                o->h_P += (uint32_t)o->h_v;
                lag = (o->h_w - (o->h_P >> 16)) & (ORB_HLEN - 1u);
                if (lag < lo) o->h_P = (o->h_w - lo) << 16;
                else if (lag > hi) o->h_P = (o->h_w - hi) << 16;
                for (g = 0; g < 2u; g++) {
                    uint32_t idx, i1, fr, ph;
                    int32_t a0, w;
                    if (o->h_t[g] <= 0) {   /* a new grain about the source point */
                        int retro = sy->hretro && (orb_xs(&o->rng) % 100u) < sy->hretro;
                        o->h_t[g] = (int32_t)G;
                        o->h_wph[g] = 0;
                        o->h_dir[g] = retro ? -1 : 1;
                        o->h_pos[g] = retro ? o->h_P + (half << 16) : o->h_P - (half << 16);
                    }
                    idx = (o->h_pos[g] >> 16) & (ORB_HLEN - 1u);
                    fr = (o->h_pos[g] >> 8) & 255u;
                    i1 = (idx + 1u) & (ORB_HLEN - 1u);
                    a0 = mm->hb[idx] + (((mm->hb[i1] - mm->hb[idx]) * (int32_t)fr) >> 8);
                    ph = o->h_wph[g] >> 15;                 /* 0..65535 over the grain: a triangle 0 .. 1 .. 0 */
                    w = (int32_t)(ph < 32768u ? ph : 65535u - ph);
                    wl += (a0 * w) >> 15;
                    o->h_wph[g] += o->h_wi;
                    o->h_pos[g] += o->h_dir[g] > 0 ? o->h_rate : (uint32_t)0 - o->h_rate;
                    o->h_t[g]--;
                }
                mm->hr[o->h_rw & 255u] = (int16_t)orb_clamp(wl, -32767, 32767);
                wr = mm->hr[(o->h_rw - 151u) & 255u];
                o->h_rw++;
                L += (((wl - L) >> 1) * o->h_mix) >> 14;
                R += (((wr - R) >> 1) * o->h_mix) >> 14;
            }
        }
        /* the filter map: drive, a state-variable filter, LP -> BP -> HP */
        if (o->f_on) {
            int32_t ch, x[2] = {L, R};
            for (ch = 0; ch < 2; ch++) {
                int32_t v0 = (int32_t)(((int64_t)x[ch] * o->f_in) >> 15), v1, v2, v3, lp, bp, hp;
                int32_t ic1 = o->f_ic[ch][0], ic2 = o->f_ic[ch][1];
                if (drive > 4096) v0 = orb_soft((int32_t)orb_clamp64(((int64_t)v0 * drive) >> 12));
                v0 *= 16;                  /* headroom inside the filter */
                v3 = v0 - ic2;
                v1 = (int32_t)(((int64_t)o->f_a1 * ic1 + (int64_t)o->f_a2 * v3) >> 28);
                v2 = ic2 + (int32_t)(((int64_t)o->f_a2 * ic1 + (int64_t)o->f_a3 * v3) >> 28);
                ic1 = 2 * v1 - ic1;
                ic2 = 2 * v2 - ic2;
                ic1 = orb_clamp(ic1, -(1 << 23), 1 << 23);
                ic2 = orb_clamp(ic2, -(1 << 23), 1 << 23);
                o->f_ic[ch][0] = ic1;
                o->f_ic[ch][1] = ic2;
                lp = v2;
                bp = v1;
                hp = v0 - (int32_t)(((int64_t)o->f_k * v1) >> 24) - v2;
                if (o->f_m < 32768) x[ch] = lp + (int32_t)(((int64_t)(bp - lp) * o->f_m) >> 15);
                else x[ch] = bp + (int32_t)(((int64_t)(hp - bp) * (o->f_m - 32768)) >> 15);
                x[ch] >>= 4;
            }
            L = x[0];
            R = x[1];
        }
        /* the end: DC out, the soft clip */
        {
            int32_t yl = L - o->dc_x[0] + orb_rz64((int64_t)o->dc_y[0] * 32702, 15);
            int32_t yr = R - o->dc_x[1] + orb_rz64((int64_t)o->dc_y[1] * 32702, 15);
            o->dc_x[0] = L; o->dc_x[1] = R; o->dc_y[0] = yl; o->dc_y[1] = yr;
            L = orb_soft(yl);
            R = orb_soft(yr);
        }
        L = (L * 31130) >> 15;              /* -0.45 dB: the clip's 1.0 stays under full scale */
        R = (R * 31130) >> 15;
        out[2u * i] += L;
        out[2u * i + 1u] += R;
        {
            uint32_t al = (uint32_t)(L < 0 ? -L : L);
            if (al > o->peak) o->peak = al;
        }
        if (i & 1u) { o->scope[o->scope_w & 127u] = (int16_t)L; o->scope_w++; }
    }
}

/* ---- events */
static void orb_events(orb_t *o)
{
    uint32_t guard = 0;
    while (o->evr != o->evw && guard++ < ORB_NEV) {
        orb_ev_t *e = &o->evq[o->evr % ORB_NEV];
        uint32_t k, pick = ORB_NV - ORB_NKV, best = 0xFFFFFFFFu;
        switch (e->type) {
        case EV_ON: {
            int32_t from = INT32_MIN, et = e->pitch;
            for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++) {   /* the same id, else free, else the oldest */
                const orb_voice_t *v = &o->v[k];
                uint32_t score = v->on && v->id == e->id ? 0u : v->on ? 0x80000000u + (v->age & 0x3FFFFFFFu)
                               : v->stage ? 0x40000000u + (uint32_t)(v->env >> 8) : 1u;   /* held: the oldest first */
                if (score < best) { best = score; pick = k; }
            }
            {   /* gravity: a key falls from the key played before it */
                uint32_t newest = 0;
                for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)
                    if (o->v[k].stage && o->v[k].age >= newest && k != pick) {
                        newest = o->v[k].age;
                        from = o->v[k].target - (int32_t)o->sys->snd[o->v[k].part % ORB_NPART].oct * 3072;
                    }
                if (from == INT32_MIN && o->sys->macro[MAC_GRAVITY] > 50) from = et;
            }
            o->v[pick].id = e->id;
            orb_trigger(o, pick, e->part <= ORB_D ? e->part : ORB_KEYS, e->pitch, (int32_t)e->vel * 258, -1, from);
            o->lastvel = (int32_t)e->vel * 258;
            break;
        }
        case EV_OFF:
            for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)
                if (o->v[k].on && o->v[k].id == e->id) orb_release(&o->v[k]);
            break;
        case EV_ALLOFF:
            for (k = 0; k < ORB_NV; k++) orb_release(&o->v[k]);
            break;
        case EV_PART:
            if (e->part >= ORB_A && e->part <= ORB_D) {
                orb_trigger(o, e->part - 1u, e->part, e->pitch, (int32_t)e->vel * 258, (int32_t)e->id * 441, INT32_MIN);
                o->rs[e->part - 1u].flash++;
            }
            break;
        case EV_GLIDE:                      /* the target moves; the spring carries the pitch there from where it is */
            for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)
                if (o->v[k].on && o->v[k].id == e->id) {
                    int32_t nt = e->pitch + (int32_t)o->sys->snd[o->v[k].part % ORB_NPART].oct * 3072;
                    o->v[k].poff = orb_clamp(o->v[k].poff + (o->v[k].target - nt) * 256, -3072 * 256, 3072 * 256);
                    o->v[k].target = nt;
                }
            break;
        case EV_PLAY:
            orb_restart(o);
            o->playing = 1;
            break;
        case EV_PAT:
            for (k = 0; k < ORB_NRING; k++) if (e->id >= ORB_NRING || e->id == k) o->rs[k].vm[0] = o->rs[k].vm[1] = 0;
            break;
        case EV_STOP:
            o->playing = 0;
            for (k = 0; k < ORB_NRING; k++) orb_release(&o->v[k]);
            break;
        default:
            break;
        }
        o->evr++;
    }
}

/* ---- the interrupt */
static void orb_render(orb_t *o, int32_t *out, uint32_t n)
{
    uint32_t sb, v, i;
    for (sb = 0; sb + ORB_SUB <= n; sb += ORB_SUB) {
        orb_events(o);
        o->Tf += (uint32_t)(o->sys->bpm < 40u ? 40u : o->sys->bpm) * 7989u;   /* 168 bpm ticks a second, 32 frames, Q16 */
        o->T += o->Tf >> 16;
        o->Tf &= 0xFFFFu;
        if (o->playing) orb_clock(o);
        if (o->T / ORB_TPS != o->rnd_k) { o->rnd_k = o->T / ORB_TPS; o->rnd = (int32_t)(orb_xs(&o->rng) >> 16) - 32768; }
        orb_mods(o);
        orb_fx_tick(o);
        for (i = 0; i < 2u * ORB_SUB; i++) { o->busd[i] = 0; o->buss[i] = 0; }
        {   /* TIDE: on while its macro or its modulation is: each voice gives its share, and feels the others' */
            uint32_t on = o->sys->macro[MAC_TIDE] || o->md[DST_TIDE] > 0;
            if (on) for (i = 0; i < ORB_SUB; i++) o->tide_next[i] = 0;
            else if (o->tide_on) for (i = 0; i < ORB_SUB; i++) o->tide_prev[i] = 0;
            o->tide_on = on;
        }
        for (v = 0; v < ORB_NV; v++) {
            orb_voice_t *vo = &o->v[v];
            if (!vo->stage && !vo->a) {
                if (o->own_live[v]) { for (i = 0; i < ORB_SUB; i++) o->own[v][i] = 0; o->own_live[v] = 0; }
                continue;
            }
            orb_voice_tick(o, vo);
            if (o->tide_on) {
                orb_voice_render(vo, o->busd, o->buss, o->tide_prev, o->own[v], o->tide_next);
                o->own_live[v] = 1;
            } else
                orb_voice_render(vo, o->busd, o->buss, 0, o->own[v], 0);
        }
        if (o->tide_on) for (i = 0; i < ORB_SUB; i++) o->tide_prev[i] = o->tide_next[i];
        orb_fx_render(o, out + 2u * sb);
    }
}

static void orb_init(orb_t *o, orb_sys_t *sys, orb_mem_t *mem)
{
    uint32_t i, *p = (uint32_t *)o;
    for (i = 0; i < sizeof *o / 4u; i++) p[i] = 0;
    p = (uint32_t *)mem;
    for (i = 0; i < sizeof *mem / 4u; i++) p[i] = 0;
    o->sys = sys;
    o->mem = mem;
    o->rng = 0x2545F491u;
    o->vrng = 0x1234567u;
    orb_restart(o);
}

#endif
