/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX: an orbital groove engine for the M-VAVE FM-1. Every rhythm is an interval, and every interval a rhythm:
 * four rings circle a sun at small ratios, and the same ratios, heard as pitch, are pure intervals above it. The voice
 * is a planet on an ellipse; you hear where it is (docs/orbitrix/PLAN.md). Built on Dinghy 0.5 (docs/DINGHY.md).
 *
 *   HOME      ORRERY (Astralica, the sun) -> MACROS     SEQ   STEPS -> TIME      EDIT  SOUND -> ORBIT -> BODY -> COSMOS
 *   ENV       ENV      LFO  MOD -> LFO     FX  FILTER MAP -> SPACE -> HORIZON     GLO  ORBIT PAD     ARP  KEYS
 *   K1..K4    the four values in the row; SEL held: the second row. ALGORITHM: the page's extra (the hint line)
 *   SELECT    the focus: KEYS, A, B, C, D: what the pages edit, and what the keys play (A..D: that track's sound)
 *   PRESETS   the system (KITTEN, MILKY WAY and the sky's 13, U1..U8 your own); SAVE: SAVE again to store it where PRESETS picks
 *   PLAY      the orrery; REC: the keys write into the focused track (playing: at its step)
 *   keys      play the focus (SOLAR: each key retuned to the sun; HARMONIC: white keys harmonics 1..16, black 17..27);
 *             SEL + a key: the sun moves there. On STEPS the white keys are the track's steps (OCT +-: the page of 16):
 *             a tap toggles rest -> hit -> accent, a held step and K1 set its note; the black keys: tracks A..D, IDEA,
 *             mute A..D, CLEAR, UNDO
 *   SEL + SEQ the focused track muted / not.  OCT- OCT+: the keys' octave
 *   MIDI      notes play the focus (and REC writes them), CC 1 is MODW, channel pressure AFT; clock and START / STOP drive the orrery */
static void app_idle(void);
#define LCD_IDLE app_idle
#if defined(DINGHY_HOST) && !defined(DINGHY_WEB)
#define ORB_LEGEND 1                            /* the legend of Astralica: rendered on the host for the page (ORB_INTRO=1,
                                                 * tools/make_legend.py), not on the FM-1 */
#endif
#ifndef APP_NAME
#define APP_NAME "ORBITRIX"
#endif
#define CV_MAX (240u * 144u)                    /* the main picture in one canvas (69 KB of the pool) */
#include "platform.c"
#include "orb_systems.h"
#include "orb_splash.h"
#if BOARD_SMK37
#include "smk_wheel.h"
#endif
#ifdef DINGHY_HOST
static uint32_t fm1_ticks(void) { return fm1_ms * 24000u; }
#define FM1_TICKS_PER_US 24u
#endif

/* ---- the engine, its memory, the interrupt */
static orb_t eng;
static orb_sys_t sys;
#ifdef DINGHY_WEB
static orb_mem_t orb_mem;                       /* (the page: plain memory, no zeros in the .wasm) */
#else
static orb_mem_t orb_mem __attribute__((section(".pool")));
#endif
static volatile uint32_t cpu_t, cpu_max;        /* the render's time (ticks) a callback: the latest, the worst */

static volatile uint32_t shed_n;              /* voices shed so far (the hint line says so) */
static void app_audio(int32_t *out, uint32_t n)
{
    static uint32_t calm;
    uint32_t t0 = fm1_ticks(), dt;
    orb_render(&eng, out, n);
    dt = fm1_ticks() - t0;
    cpu_t = dt;
    if (dt > cpu_max) cpu_max = dt;
    if (calm) calm--;                           /* over 80 % of the 5.8 ms: the quietest voice goes, one each 50 ms at most */
    else if (dt > 5805u * FM1_TICKS_PER_US * 4u / 5u) { orb_shed(&eng); shed_n++; calm = 8; }
}

/* ---- the look (the drawings: deep indigo, violet frames, lime, the ring colours) */
#define C_BG RGB(10, 6, 26)
#define C_PANEL RGB(18, 11, 44)
#define C_FRAME RGB(112, 66, 232)
#define C_FDIM RGB(52, 32, 116)
#define C_LIME RGB(198, 255, 26)
#define C_YEL RGB(255, 232, 26)
#define C_CYAN RGB(38, 224, 255)
#define C_MAG RGB(255, 79, 168)
#define C_VIO RGB(164, 116, 255)
#define C_TXT RGB(240, 232, 255)
#define C_GREY RGB(128, 108, 184)
#define C_DIMT RGB(76, 60, 120)
#define C_INK RGB(16, 8, 34)
static const uint16_t RINGC[5] = {C_LIME, C_CYAN, C_MAG, C_YEL, C_VIO};   /* KEYS, A, B, C, D */

static uint16_t mix565(uint16_t a, uint16_t b, uint32_t t)   /* a -> b, t 0..15 */
{
    uint32_t r = ((a >> 11) * (15u - t) + (b >> 11) * t) / 15u, g = (((a >> 5) & 63u) * (15u - t) + ((b >> 5) & 63u) * t) / 15u;
    uint32_t bl = ((a & 31u) * (15u - t) + (b & 31u) * t) / 15u;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static uint16_t dim565(uint16_t c, uint32_t t) { return mix565(C_BG, c, t); }

/* text blended onto a background (cv_text blends onto black only) */
static int32_t tx(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint16_t bg)
{
    uint16_t ramp[16];
    uint32_t a;
    for (a = 0; a < 16u; a++) ramp[a] = mix565(bg, c, a);
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v) cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, ramp[v]);
            }
        x += f->adv[gi];
    }
    return x;
}
static void txc(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint16_t bg)
{
    tx(cx - text_w(f, s) / 2, y, f, s, c, bg);
}
static void frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a box with cut corners */
{
    cv_rect(x + 2, y, w - 4, 1, c);
    cv_rect(x + 2, y + h - 1, w - 4, 1, c);
    cv_rect(x, y + 2, 1, h - 4, c);
    cv_rect(x + w - 1, y + 2, 1, h - 4, c);
    cv_pset(x + 1, y + 1, c); cv_pset(x + w - 2, y + 1, c); cv_pset(x + 1, y + h - 2, c); cv_pset(x + w - 2, y + h - 2, c);
}
static void pill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)  /* a filled box with cut corners */
{
    cv_rect(x + 1, y, w - 2, h, c);
    cv_rect(x, y + 1, w, h - 2, c);
}
static void disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t y, x;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r + r) cv_pset(cx + x, cy + y, c);
}
static void circle(int32_t cx, int32_t cy, int32_t r, uint16_t c, uint32_t dash)   /* dash: every n-th pixel (0: all) */
{
    int32_t x = r, y = 0, err = 1 - r, k = 0;
    while (x >= y && k < 2000) {
        if (!dash || (k % (int32_t)dash) == 0) {
            cv_pset(cx + x, cy + y, c); cv_pset(cx + y, cy + x, c); cv_pset(cx - y, cy + x, c); cv_pset(cx - x, cy + y, c);
            cv_pset(cx - x, cy - y, c); cv_pset(cx - y, cy - x, c); cv_pset(cx + y, cy - x, c); cv_pset(cx + x, cy - y, c);
        }
        y++; k++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}
static int32_t isin(uint32_t ph, int32_t r) { return (orb_sin(ph) * r) >> 15; }
static int32_t icos(uint32_t ph, int32_t r) { return (orb_cos(ph) * r) >> 15; }

static void stars(int32_t w, int32_t h, uint32_t seed, uint32_t n)   /* pixel stars, some twinkling */
{
    uint32_t i, s = seed;
    for (i = 0; i < n; i++) {
        uint32_t x = orb_xs(&s), y = orb_xs(&s), tw = (x >> 20) & 7u;
        int32_t px = (int32_t)(x % (uint32_t)w), py = (int32_t)(y % (uint32_t)h);
        uint32_t b = ((fm1_ms >> 7) + tw) & 7u;
        cv_pset(px, py, dim565(tw < 2u ? C_TXT : C_VIO, b < 2u ? 15u : 5u + tw));
        if (tw == 0u && b == 0u) { cv_pset(px - 1, py, dim565(C_TXT, 7)); cv_pset(px + 1, py, dim565(C_TXT, 7)); cv_pset(px, py - 1, dim565(C_TXT, 7)); cv_pset(px, py + 1, dim565(C_TXT, 7)); }
    }
}

/* ---- numbers to text */
static char *put_s(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char *put_u(char *p, uint32_t v)
{
    char t[12];
    int n = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 11);
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}
static char *put_i(char *p, int32_t v, int plus)
{
    if (v < 0) { *p++ = '-'; v = -v; }
    else if (plus && v > 0) *p++ = '+';
    return put_u(p, (uint32_t)v);
}
static const char *const NOTE_NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/* the ratio of a degree of the tuning (names per tuning) */
static const char *const DEG_NAME[ORB_NTUNING][12] = {
    {"1/1", "16/15", "9/8", "6/5", "5/4", "4/3", "7/5", "3/2", "8/5", "5/3", "7/4", "15/8"},
    {"1/1", "9/8", "5/4", "11/8", "3/2", "13/8", "7/4", "15/8"},
    {"1/1", "16/15", "8/7", "16/13", "4/3", "16/11", "8/5", "16/9"},
    {"1/1", "9/8", "5/4", "3/2", "5/3"},
    {"0", "100", "200", "300", "400", "500", "600", "700", "800", "900", "1000", "1100"}};
static char *put_deg(char *p, int32_t deg)
{
    uint32_t t = sys.tuning % ORB_NTUNING, n = ORB_SCALE_N[t];
    int32_t oct = 0;
    while (deg < 0 && oct > -8) { deg += (int32_t)n; oct--; }
    while (deg >= (int32_t)n && oct < 8) { deg -= (int32_t)n; oct++; }
    p = put_s(p, DEG_NAME[t][deg]);
    while (oct > 0) { *p++ = '\''; oct--; }
    while (oct < 0) { *p++ = ','; oct++; }
    *p = 0;
    return p;
}

/* ---- settings kept over a power-off */
#define SET_VER 6u                              /* (6: the new set of sixteen, U1.. moved: the settings start again) */
static struct { uint8_t oct, ch, sysi, ver; } set = {0, 0, 0, SET_VER};
static uint32_t set_dirty, set_t;
static uint32_t drawn_bar;                      /* (the bar drawn for: ~0 draws it again; defined with the screens') */
static void set_touch(void) { set_dirty = 1; set_t = fm1_ms; }

/* ---- UNDO: each ring's pattern as it was before the last gesture on it (an IDEA, a CLEAR, LEN HITS ROT, steps): one
 * level, and UNDO again is REDO. A gesture: changes to that ring with under 1.5 s between them */
static struct { uint8_t on[ORB_NSTEP]; int8_t dg[ORB_NSTEP]; uint8_t len, hits, rot, ok, follow; int8_t deg; } undo[ORB_NRING];
static uint32_t undo_t[ORB_NRING], undo_redo[ORB_NRING];
static void undo_take(uint32_t r, int fresh)
{
    const orb_ring_t *g = &sys.ring[r];
    uint32_t i;
    if (fresh || !undo[r].ok || fm1_ms - undo_t[r] > 1500u) {
        for (i = 0; i < ORB_NSTEP; i++) { undo[r].on[i] = g->on[i]; undo[r].dg[i] = g->dg[i]; }
        undo[r].len = g->len; undo[r].hits = g->hits; undo[r].rot = g->rot; undo[r].ok = 1;
        undo[r].follow = g->follow; undo[r].deg = g->deg;
        undo_redo[r] = 0;
    }
    undo_t[r] = fm1_ms;
}
static void undo_forget(void) { uint32_t r; for (r = 0; r < ORB_NRING; r++) undo[r].ok = 0; }

/* ---- the pages and their values */
enum { PG_ORRERY, PG_MACRO, PG_RING, PG_TIME, PG_SOUND, PG_ORBIT, PG_BODY, PG_COSMOS, PG_ENV, PG_MOD, PG_LFO, PG_FILTER,
       PG_SPACE, PG_HORIZON, PG_BG, PG_PAD, PG_KEYS, PG_CUTE, NPAGE };
static const char *const PAGE_NAME[NPAGE] = {"ORRERY", "MACROS", "STEPS", "TIME", "SOUND", "ORBIT", "BODY", "COSMOS", "ENV", "MOD",
                                             "LFO", "FILTER MAP", "SPACE", "HORIZON", "BACKGROUND", "ORBIT PAD", "KEYS",
                                             "CUTE"};
enum { W_SYS, W_SND, W_RING, W_RA, W_RB, W_RC, W_RD, W_MOD, W_SET, W_SA, W_SB, W_SC, W_SD, W_NONE };   /* (W_SA..: A..D's sound) */
enum { F_NUM, F_SIGNED, F_TIME, F_RATIO, F_DIR, F_LOGIC, F_DEG, F_TUNING, F_NOTE, F_KMODE, F_MRATIO, F_DT, F_SYNC,
       F_SHAPE, F_SRC, F_DST, F_OCT, F_BPM, F_ONOFF, F_PAN, F_LRATE0, F_LRATE1, F_TRIG, F_CH, F_SLOT, F_PCT,
       F_INSP, F_GRAIN, F_SEMI, F_LIB, F_TEMP, F_LOWCUT, F_TIRED, F_BGPROG, F_CHANGE, F_HALO, F_MOOD };
typedef struct {
    const char *name;
    uint8_t where, fmt;
    uint16_t off;
    int16_t lo, hi;
} pd_t;
#define SO(f) ((uint16_t)__builtin_offsetof(orb_sys_t, f))
#define NO(f) ((uint16_t)__builtin_offsetof(orb_snd_t, f))
#define RO(f) ((uint16_t)__builtin_offsetof(orb_ring_t, f))
#define MO(f) ((uint16_t)__builtin_offsetof(orb_mod_t, f))
#define P_NONE {"", W_NONE, F_NUM, 0, 0, 0}
#define P_BPM {"BPM", W_SYS, F_BPM, SO(bpm), 40, 240}
static const pd_t PD[NPAGE][9] = {   /* K1..K4, SEL K1..K4, ALGORITHM */
    [PG_ORRERY] = {{"HITS A", W_RA, F_NUM, RO(hits), 0, 64}, {"HITS B", W_RB, F_NUM, RO(hits), 0, 64},
                   {"HITS C", W_RC, F_NUM, RO(hits), 0, 64}, {"HITS D", W_RD, F_NUM, RO(hits), 0, 64},
                   {"LEVEL A", W_SA, F_NUM, NO(level), 0, 99}, {"LEVEL B", W_SB, F_NUM, NO(level), 0, 99},
                   {"LEVEL C", W_SC, F_NUM, NO(level), 0, 99}, {"LEVEL D", W_SD, F_NUM, NO(level), 0, 99}, P_BPM},
    [PG_MACRO] = {{"CHAOS", W_SYS, F_PCT, SO(macro[MAC_CHAOS]), 0, 99}, {"SPECTR", W_SYS, F_PCT, SO(macro[MAC_SPECTRAL]), 0, 99},
                  {"GRAVITY", W_SYS, F_PCT, SO(macro[MAC_GRAVITY]), 0, 99}, {"ORBIT", W_SYS, F_PCT, SO(macro[MAC_ORBIT]), 0, 99},
                  {"FRACT", W_SYS, F_PCT, SO(macro[MAC_FRACT]), 0, 99}, {"TIDE", W_SYS, F_PCT, SO(macro[MAC_TIDE]), 0, 99},
                  {"HORIZON", W_SYS, F_PCT, SO(macro[MAC_HORIZON]), 0, 99}, {"MUTATE", W_SYS, F_PCT, SO(mutate), 0, 99}, P_BPM},
    [PG_RING] = {{"SPEED", W_RING, F_RATIO, RO(ratio), 0, ORB_NRATIO - 1}, {"LEN", W_RING, F_NUM, RO(len), 1, 64},
                 {"HITS", W_RING, F_NUM, RO(hits), 0, 64}, {"ROT", W_RING, F_NUM, RO(rot), 0, 63},
                 {"NOTE", W_RING, F_DEG, RO(deg), -14, 21}, {"DIR", W_RING, F_DIR, RO(dir), 0, ORB_NDIR - 1},
                 {"PROB", W_RING, F_PCT, RO(prob), 0, 99}, {"LOGIC", W_RING, F_LOGIC, RO(logic), 0, 13},
                 {"FOLLOW", W_RING, F_ONOFF, RO(follow), 0, 1}},
    [PG_TIME] = {P_BPM, {"SWING", W_SYS, F_PCT, SO(swing), 0, 99}, {"MUTATE", W_SYS, F_PCT, SO(mutate), 0, 99},
                 {"HUM", W_SYS, F_PCT, SO(hum), 0, 99}, {"VAR", W_SYS, F_PCT, SO(var), 0, 99},
                 {"GATE", W_SND, F_PCT, NO(gate), 1, 99}, {"MUTE", W_RING, F_ONOFF, RO(mute), 0, 1},
                 {"TRIG", W_SND, F_TRIG, NO(flags), 0, 3}, P_BPM},
    [PG_SOUND] = {{"SOUND", W_SND, F_LIB, NO(lib), 1, ORB_NLIB - 1}, {"TONE", W_SND, F_NUM, NO(ecc), 0, 99},
                  {"DECAY", W_SND, F_TIME, NO(d), 0, 99}, {"LEVEL", W_SND, F_NUM, NO(level), 0, 99},
                  {"OCT", W_SND, F_OCT, NO(oct), -3, 3}, {"PAN", W_SND, F_PAN, NO(pan), 0, 99},
                  {"SPACE", W_SND, F_NUM, NO(send), 0, 99}, {"WOBBLE", W_SND, F_NUM, NO(moon), 0, 99}, P_BPM},
    [PG_ORBIT] = {{"ECC", W_SND, F_NUM, NO(ecc), 0, 99}, {"SLING", W_SND, F_NUM, NO(sling), 0, 99},
                  {"ANG", W_SND, F_NUM, NO(ang), 0, 99}, {"SPR", W_SND, F_NUM, NO(spr), 0, 99},
                  {"PREC", W_SND, F_NUM, NO(prec), 0, 99}, {"MOON", W_SND, F_NUM, NO(moon), 0, 99},
                  {"RATIO", W_SND, F_MRATIO, NO(mratio), 0, ORB_NMOON - 1}, {"DUST", W_SND, F_NUM, NO(dust), 0, 99}, P_BPM},
    [PG_BODY] = {{"FALL", W_SND, F_NUM, NO(fall), 0, 99}, {"FALL T", W_SND, F_TIME, NO(fallt), 0, 99},
                 {"COLOR", W_SND, F_NUM, NO(color), 0, 99}, {"PIXEL", W_SND, F_NUM, NO(pixel), 0, 99},
                 {"LEVEL", W_SND, F_NUM, NO(level), 0, 99}, {"PAN", W_SND, F_PAN, NO(pan), 0, 99},
                 {"SEND", W_SND, F_NUM, NO(send), 0, 99}, {"OCT", W_SND, F_OCT, NO(oct), -3, 3},
                 {"TRIG", W_SND, F_TRIG, NO(flags), 0, 3}},
    [PG_COSMOS] = {{"TIDE", W_SND, F_NUM, NO(tide), 0, 99}, {"INSPRL", W_SND, F_INSP, NO(insp), 0, 99},
                   {"INSP T", W_SND, F_TIME, NO(inspt), 0, 99}, {"BEAM", W_SND, F_NUM, NO(beam), 0, 99},
                   {"TIDE ALL", W_SYS, F_PCT, SO(macro[MAC_TIDE]), 0, 99}, {"RATIO", W_SND, F_MRATIO, NO(mratio), 0, ORB_NMOON - 1},
                   P_NONE, P_NONE, P_BPM},
    [PG_ENV] = {{"ATTACK", W_SND, F_TIME, NO(a), 0, 99}, {"DECAY", W_SND, F_TIME, NO(d), 0, 99},
                {"SUSTAIN", W_SND, F_NUM, NO(s), 0, 99}, {"RELEASE", W_SND, F_TIME, NO(r), 0, 99},
                {"ECC AMT", W_SND, F_SIGNED, NO(eamt), -99, 99}, {"ECC ATK", W_SND, F_TIME, NO(ea), 0, 99},
                {"ECC DEC", W_SND, F_TIME, NO(ed), 0, 99}, {"VEL", W_SND, F_NUM, NO(vsens), 0, 99}, P_BPM},
    [PG_MOD] = {{"SOURCE", W_MOD, F_SRC, MO(src), 0, ORB_NSRC - 1}, {"DEST", W_MOD, F_DST, MO(dst), 0, ORB_NDST - 1},
                {"AMOUNT", W_MOD, F_SIGNED, MO(amt), -99, 99}, {"SLOT", W_NONE, F_SLOT, 0, 0, 7},
                {"SOURCE", W_MOD, F_SRC, MO(src), 0, ORB_NSRC - 1}, {"DEST", W_MOD, F_DST, MO(dst), 0, ORB_NDST - 1},
                {"AMOUNT", W_MOD, F_SIGNED, MO(amt), -99, 99}, {"SLOT", W_NONE, F_SLOT, 0, 0, 7},
                {"SLOT", W_NONE, F_SLOT, 0, 0, 7}},
    [PG_LFO] = {{"L1 RATE", W_SYS, F_LRATE0, SO(lrate[0]), 0, 99}, {"L1 WAVE", W_SYS, F_SHAPE, SO(lshape[0]), 0, ORB_NLSHAPE - 1},
                {"L2 RATE", W_SYS, F_LRATE1, SO(lrate[1]), 0, 99}, {"L2 WAVE", W_SYS, F_SHAPE, SO(lshape[1]), 0, ORB_NLSHAPE - 1},
                {"L1 SYNC", W_SYS, F_ONOFF, SO(lsync[0]), 0, 1}, P_NONE,
                {"L2 SYNC", W_SYS, F_ONOFF, SO(lsync[1]), 0, 1}, P_NONE, P_BPM},
    [PG_FILTER] = {{"CUT", W_SYS, F_NUM, SO(cut), 0, 99}, {"RES", W_SYS, F_NUM, SO(res), 0, 99},
                   {"MODE", W_SYS, F_NUM, SO(fmode), 0, 99}, {"DRIVE", W_SYS, F_NUM, SO(drive), 0, 99},
                   P_NONE, P_NONE, P_NONE, P_NONE, P_BPM},
    [PG_SPACE] = {{"TIME", W_SYS, F_DT, SO(dtime), 0, ORB_NDT - 1}, {"FDBK", W_SYS, F_NUM, SO(dfb), 0, 99},
                  {"SIZE", W_SYS, F_NUM, SO(rsize), 0, 99}, {"NEBULA", W_SYS, F_NUM, SO(rmix), 0, 99},
                  {"TONE", W_SYS, F_NUM, SO(dtone), 0, 99}, {"DECAY", W_SYS, F_NUM, SO(rdecay), 0, 99},
                  {"DAMP", W_SYS, F_NUM, SO(rdamp), 0, 99}, {"ECHO", W_SYS, F_NUM, SO(dmix), 0, 99},
                  {"PING", W_SYS, F_ONOFF, SO(ping), 0, 1}},
    [PG_HORIZON] = {{"DEPTH", W_SYS, F_PCT, SO(macro[MAC_HORIZON]), 0, 99}, {"SHIFT", W_SYS, F_SEMI, SO(hshift), -24, 24},
                    {"RETRO", W_SYS, F_PCT, SO(hretro), 0, 99}, {"GRAIN", W_SYS, F_GRAIN, SO(hgrain), 0, 99},
                    {"TIRED", W_SYS, F_TIRED, SO(tired), 0, ORB_NTIRED - 1}, {"BLOOM", W_SYS, F_NUM, SO(bloom), 0, 99},
                    P_NONE, P_NONE, P_BPM},
    [PG_BG] = {{"LEVEL", W_SYS, F_NUM, SO(bg_level), 0, 99}, {"CHORDS", W_SYS, F_BGPROG, SO(bg_prog), 0, ORB_NBGPROG - 1},
               {"CHANGE", W_SYS, F_CHANGE, SO(bg_change), 0, 3}, {"HALO", W_SYS, F_HALO, SO(bg_halo), 0, 99},
               {"SILK", W_SYS, F_NUM, SO(bg_silk), 0, 99}, {"WIDTH", W_SYS, F_NUM, SO(bg_width), 0, 99},
               {"LOW CUT", W_SYS, F_LOWCUT, SO(bg_cut), 0, 99}, {"TEMP", W_SYS, F_TEMP, SO(bg_temp), -2, 1},
               {"GLOW", W_SYS, F_NUM, SO(bg_glow), 0, 99}},
    [PG_PAD] = {{"X", W_SYS, F_NUM, SO(ox), 0, 99}, {"Y", W_SYS, F_NUM, SO(oy), 0, 99},
                {"RAD", W_SYS, F_NUM, SO(orad), 0, 50}, {"PERIOD", W_SYS, F_SYNC, SO(orate), 0, ORB_NSYNC - 1},
                P_NONE, P_NONE, P_NONE, P_NONE, P_BPM},
    [PG_CUTE] = {{"MOOD", W_SYS, F_MOOD, SO(mood), 0, ORB_NMOOD - 1}, {"CHORDS", W_SYS, F_BGPROG, SO(bg_prog), 0, ORB_NBGPROG - 1},
                 {"HALO", W_SYS, F_HALO, SO(bg_halo), 0, 99}, P_BPM,
                 {"CHORD LV", W_SYS, F_NUM, SO(bg_level), 0, 99}, {"CHANGE", W_SYS, F_CHANGE, SO(bg_change), 0, 3},
                 {"SWING", W_SYS, F_PCT, SO(swing), 0, 99}, {"KEYS", W_SYS, F_KMODE, SO(keymode), 0, ORB_NKM - 1},
                 {"MUTATE", W_SYS, F_PCT, SO(mutate), 0, 99}},
    [PG_KEYS] = {{"MODE", W_SYS, F_KMODE, SO(keymode), 0, ORB_NKM - 1}, {"TUNING", W_SYS, F_TUNING, SO(tuning), 0, ORB_NTUNING - 1},
                 {"SUN", W_SYS, F_NOTE, SO(sun), 0, 11}, {"OCT", W_SET, F_OCT, 0, -3, 3},
                 {"GRAVITY", W_SYS, F_PCT, SO(macro[MAC_GRAVITY]), 0, 99}, {"MIDI CH", W_SET, F_CH, 1, 0, 16},
                 P_NONE, P_NONE, P_BPM},
};
static uint32_t page = PG_ORRERY, focus = ORB_KEYS, slot;   /* focus: 0 KEYS, 1..4 A..D: what the pages edit and the keys play */
static uint32_t ring_focus(void) { return focus ? focus - 1u : 0u; }
/* CHORD keys: whether a key plays the chord's bass now (it glows) */
static int chord_bass_key(uint32_t note)
{
    uint32_t id = orb_chord_at(&sys, eng.T);
    int32_t p = orb_key_chord(&sys, eng.T, note), b = (int32_t)(sys.sun % 12u) * 256 + orb_chord_tone(id, 0);
    return (((p - b) % 3072) + 3072) % 3072 == 0;
}

/* a ring's note: a degree of the tuning, or with FOLLOW the chord's note (#0 its bass, #1 the next up ..) and what it
 * is in the chord now */
static char *put_note_of(char *p, uint32_t r, int32_t deg)
{
    if (!sys.ring[r % ORB_NRING].follow) return put_deg(p, deg);
    {
        int32_t pc = (((int32_t)(sys.sun % 12u) * 256 + orb_chord_tone(orb_chord_at(&sys, eng.T), deg)) / 256 % 12 + 12) % 12;
        p = put_i(put_s(p, "#"), deg, 0);
        *p++ = ' ';
        return put_s(p, NOTE_NAME[pc]);
    }
}
static uint8_t *pd_ptr(const pd_t *d)
{
    switch (d->where) {
    case W_SYS: return (uint8_t *)&sys + d->off;
    case W_SND: return (uint8_t *)&sys.snd[focus] + d->off;
    case W_RING: return (uint8_t *)&sys.ring[ring_focus()] + d->off;
    case W_RA: case W_RB: case W_RC: case W_RD: return (uint8_t *)&sys.ring[d->where - W_RA] + d->off;
    case W_MOD: return (uint8_t *)&sys.mod[slot % ORB_NMOD] + d->off;
    case W_SET: return (uint8_t *)&set + d->off;
    case W_SA: case W_SB: case W_SC: case W_SD: return (uint8_t *)&sys.snd[ORB_A + d->where - W_SA] + d->off;
    default: return 0;
    }
}
static int pd_signed(const pd_t *d) { return d->lo < 0; }
static int32_t pd_get(const pd_t *d)
{
    uint8_t *p = pd_ptr(d);
    if (d->fmt == F_SLOT) return (int32_t)slot;
    if (!p) return 0;
    if (d->fmt == F_LOGIC) {                    /* FREE, AND A..D, NOT A..D, XOR A..D, CONJ */
        orb_ring_t *r = (orb_ring_t *)(p - d->off);
        return r->logic == LOG_FREE ? 0 : r->logic == LOG_CONJ ? 13 : 1 + ((int32_t)r->logic - 1) * 4 + (r->ltgt & 3);
    }
    if (d->fmt == F_TRIG) return *p & 3u;
    return pd_signed(d) ? (int32_t)(int8_t)*p : (int32_t)*p;
}
/* SOUND: a sound from the library into the focus; stopped, you hear it at once (playing: at its next hit) */
static uint32_t preview_t;
static void lib_pick(uint32_t li)
{
    orb_lib_load(&sys.snd[focus], li, focus);
    if (eng.playing) return;
    if (focus) orb_part_on(&eng, (uint8_t)focus, (36 + (int32_t)(sys.sun % 12u)) * 256, 100, 30);
    else {
        orb_note_on(&eng, 99, orb_key_pitch_o(&eng, sys.keymode == KM_HARMONIC ? 7u : 60u + sys.sun % 12u, 0, 0), 100);
        preview_t = fm1_ms + 350u;
    }
}
static void mood_turn(uint32_t m);
static void say(const char *s);
static void pd_set(const pd_t *d, int32_t v)
{
    uint8_t *p = pd_ptr(d);
    v = orb_clamp(v, d->lo, d->hi);
    if (d->fmt == F_SLOT) { slot = (uint32_t)v; return; }
    if (!p) return;
    if (d->fmt == F_LOGIC) {
        orb_ring_t *r = (orb_ring_t *)(p - d->off);
        if (v == 0) r->logic = LOG_FREE;
        else if (v == 13) r->logic = LOG_CONJ;
        else { r->logic = (uint8_t)(1 + (v - 1) / 4); r->ltgt = (uint8_t)((v - 1) % 4); }
        return;
    }
    if (d->fmt == F_TRIG) { *p = (uint8_t)((*p & ~3u) | (uint32_t)v); return; }
    if (d->fmt == F_LIB) { lib_pick((uint32_t)v); return; }
    if (d->fmt == F_MOOD) { mood_turn((uint32_t)v); drawn_bar = ~0u; return; }
    if (d->where == W_RING && d->off == RO(follow)) {   /* FOLLOW: its notes kept where they were */
        orb_follow_set(&sys, ring_focus(), (uint32_t)v);
        orb_pattern_edited(&eng, ring_focus());
        say(v ? "FOLLOWS THE CHORDS" : "ON THE TUNING");
        return;
    }
    if (d->where >= W_RING && d->where <= W_RD && (d->off == RO(len) || d->off == RO(hits) || d->off == RO(rot)))
        undo_take((uint32_t)((orb_ring_t *)(p - d->off) - sys.ring), 0);
    *p = (uint8_t)v;
    if (d->where == W_SET) set_touch();
    if (d->where >= W_RING && d->where <= W_RD && (d->off == RO(len) || d->off == RO(hits) || d->off == RO(rot))) {
        orb_ring_t *r = (orb_ring_t *)(p - d->off);
        if (r->hits > r->len) r->hits = r->len;
        if (r->rot >= r->len) r->rot = (uint8_t)(r->len - 1u);
        orb_euclid(r);
        orb_pattern_edited(&eng, (uint32_t)(r - sys.ring));
    }
}
static void pd_text(const pd_t *d, char *s)
{
    int32_t v = pd_get(d);
    s[0] = 0;
    if (d->where == W_NONE && d->fmt != F_SLOT) return;
    switch (d->fmt) {
    case F_SIGNED: put_i(s, v, 1); break;
    case F_OCT: put_i(s, v, 1); break;
    case F_TIME: {
        uint32_t ms = ORB_TIME_MS[orb_clamp(v, 0, 99)];
        if (ms < 1000u) { put_s(put_u(s, ms), "ms"); }
        else { char *p = put_u(s, ms / 1000u); *p++ = '.'; p = put_u(p, ms / 100u % 10u); put_s(p, "s"); }
        break;
    }
    case F_RATIO: put_s(s, ORB_RATIO_NAME[v % ORB_NRATIO]); break;
    case F_DIR: put_s(s, ORB_DIR_NAME[v % ORB_NDIR]); break;
    case F_LOGIC:
        if (v == 0) put_s(s, "FREE");
        else if (v == 13) put_s(s, "CONJ");
        else { char *p = put_s(s, ORB_LOGIC_NAME[1 + (v - 1) / 4]); *p++ = ' '; *p++ = (char)('A' + (v - 1) % 4); *p = 0; }
        break;
    case F_DEG: put_note_of(s, ring_focus(), v); break;
    case F_TUNING: put_s(s, ORB_TUNING_NAME[v % ORB_NTUNING]); break;
    case F_NOTE: put_s(s, NOTE_NAME[v % 12]); break;
    case F_KMODE: put_s(s, v == KM_HARMONIC ? "HARMON" : v == KM_CHORD ? "CHORD" : "SOLAR"); break;
    case F_MRATIO: put_s(s, ORB_MRATIO_NAME[v % ORB_NMOON]); break;
    case F_DT: put_s(s, ORB_DT_NAME[v % ORB_NDT]); break;
    case F_SYNC: put_s(s, ORB_SYNC_NAME[v % ORB_NSYNC]); break;
    case F_SHAPE: put_s(s, ORB_LSHAPE_NAME[v % ORB_NLSHAPE]); break;
    case F_SRC: put_s(s, ORB_SRC_NAME[v % ORB_NSRC]); break;
    case F_DST: put_s(s, ORB_DST_NAME[v % ORB_NDST]); break;
    case F_ONOFF: put_s(s, v ? "ON" : "OFF"); break;
    case F_PAN: if (v == 50) put_s(s, "C"); else { s[0] = v < 50 ? 'L' : 'R'; put_u(s + 1, (uint32_t)(v < 50 ? 50 - v : v - 50)); } break;
    case F_LRATE0: case F_LRATE1: {
        uint32_t k = d->fmt == F_LRATE1;
        if (sys.lsync[k]) put_s(s, ORB_SYNC_NAME[(uint32_t)v * ORB_NSYNC / 100u]);
        else put_u(s, (uint32_t)v);
        break;
    }
    case F_TRIG: put_s(s, v == 0 ? "FREE" : v == 1 ? "PHASE" : v == 2 ? "LEGATO" : "BOTH"); break;
    case F_CH: if (v) put_u(s, (uint32_t)v); else put_s(s, "OMNI"); break;
    case F_SLOT: put_u(s, (uint32_t)v + 1u); break;
    case F_INSP: if (v) put_s(put_i(s, -(v * 48 / 99), 0), "st"); else put_s(s, "OFF"); break;
    case F_SEMI: put_s(put_i(s, v, 1), "st"); break;
    case F_GRAIN: put_s(put_u(s, (256u + (uint32_t)v * 39u) * 10u / 441u), "ms"); break;
    case F_LIB: put_s(s, v > 0 && v < (int32_t)ORB_NLIB ? ORB_LIB[v].name : "OWN"); break;
    case F_TEMP: { static const char *const T[4] = {"0.7K", "1.4K", "2.7K", "5.5K"}; put_s(s, T[orb_clamp(v, -2, 1) + 2]); break; }
    case F_LOWCUT: if (v) put_s(put_u(s, 20u + (uint32_t)v * 4u), "Hz"); else put_s(s, "OFF"); break;
    case F_TIRED: put_s(s, ORB_TIRED_NAME[(uint32_t)v % ORB_NTIRED]); break;
    case F_BGPROG: put_s(s, ORB_BGPROG_NAME[(uint32_t)v % ORB_NBGPROG]); break;
    case F_CHANGE: put_s(put_u(s, 1u << ((uint32_t)v & 3u)), v ? " BARS" : " BAR"); break;
    case F_HALO: if (v) put_s(put_u(s, ORB_HALO_SEC[(uint32_t)v % 100u]), " s"); else put_s(s, "DRY"); break;
    case F_MOOD: put_s(s, ORB_MOOD_NAME[(uint32_t)v % ORB_NMOOD]); break;
    case F_BPM: case F_PCT: case F_NUM: default: put_i(s, v, 0); break;
    }
}
static uint32_t pd_fill(const pd_t *d)          /* 0..1024: how full its gauge is */
{
    int32_t v = pd_get(d), span = d->hi - d->lo;
    if (d->fmt == F_LRATE0 || d->fmt == F_LRATE1) span = 99;
    return span > 0 && v > d->lo ? (uint32_t)((v - d->lo) * 1024 / span) : 0u;
}

/* ---- the systems: 10 built in, U1..U8 in the blobs */
#define NSYS (ORB_NSYSTEM + NBLOB)
static uint32_t sysi, sys_pending = 0xFFu, sys_pend_t, msg_t;
static char msg[32];
static uint32_t msg_seq;                        /* (a new message: the hint line at once) */
static uint32_t happy_t, mew_t, mew_n, love_t;   /* Astralica: happy eyes till happy_t; a mew at mew_t; hearts from love_t */
static void cat_happy(void) { happy_t = fm1_ms + 700u; }
static void say(const char *s) { put_s(msg, s); msg_t = fm1_ms + 1400u; msg_seq++; }
static uint32_t made;                           /* the patch is a made one (its name on the bar, not a system's) */
static int user_load(uint32_t u, orb_sys_t *dst) { return blob_load(u, dst, sizeof *dst); }
static void system_load(uint32_t i)
{
    static orb_sys_t tmp;
    if (i >= ORB_NSYSTEM) {
        if (user_load(i - ORB_NSYSTEM, &tmp)) { say("AN EMPTY BOX"); return; }
    } else
        orb_system(i, &tmp);
    orb_all_off(&eng);
    sys = tmp;
    orb_pattern_edited(&eng, ORB_NRING);
    undo_forget();
    cat_happy();
    made = 0;
    sysi = i;
    set.sysi = (uint8_t)i;
    set_touch();
}
/* CUTE: a whole new patch in the mood (SEL+HOME), or the patch turned to a mood (CUTE's MOOD): its tempo, sounds,
 * space and chords, its patterns kept */
static void cute_new(void)
{
    static orb_sys_t tmp;
    static uint32_t seed = 0x5EEDu;
    char s[28];
    seed = seed * 1103515245u + 12345u + fm1_ms;
    orb_cute(&tmp, sys.mood % ORB_NMOOD, seed);
    orb_all_off(&eng);
    sys = tmp;
    orb_pattern_edited(&eng, ORB_NRING);
    undo_forget();
    made = 1;
    cat_happy(); love_t = fm1_ms;
    put_s(put_s(put_s(s, "A NEW "), ORB_MOOD_NAME[sys.mood % ORB_NMOOD]), " ONE, nya~");
    say(s);
}
static void mood_turn(uint32_t m)
{
    char s[28];
    orb_mood(&sys, m);
    if (made) {                                 /* (a made one takes the mood's name) */
        uint32_t k;
        for (k = 0; k < 11u; k++) sys.name[k] = 0;
        for (k = 0; k < 11u && ORB_MOOD_NAME[m % ORB_NMOOD][k]; k++) sys.name[k] = ORB_MOOD_NAME[m % ORB_NMOOD][k];
    }
    orb_pattern_edited(&eng, ORB_NRING);
    cat_happy();
    put_s(put_s(s, "MOOD: "), ORB_MOOD_NAME[m % ORB_NMOOD]);
    say(s);
}
static void sys_label(uint32_t i, char *s)
{
    if (made) { uint32_t k; for (k = 0; k < 11u && sys.name[k]; k++) s[k] = sys.name[k]; s[k] = 0; return; }
    if (i < ORB_NSYSTEM) { put_s(s, ORB_SYSTEM_NAME[i]); return; }
    s[0] = 'U'; s[1] = (char)('1' + i - ORB_NSYSTEM); s[2] = 0;
}


/* ---- little pictures: each kind of sound (9 x 9, the bit 8 at the left), the keys' functions on STEPS (7 x 7), a paw */
static const uint16_t KIND_ICON[ORB_NLK][9] = {
    {0x038, 0x0C6, 0x082, 0x111, 0x139, 0x111, 0x082, 0x0C6, 0x038},   /* KICK: a drum's skin */
    {0x101, 0x082, 0x044, 0x0FE, 0x101, 0x1FF, 0x155, 0x155, 0x1FF},   /* SNARE: a drum and its sticks */
    {0x010, 0x07C, 0x1FF, 0x010, 0x07C, 0x1FF, 0x010, 0x028, 0x044},   /* HAT: two cymbals on a stand */
    {0x00E, 0x01C, 0x038, 0x07E, 0x01C, 0x038, 0x060, 0x0C0, 0x180},   /* PERC: a spark */
    {0x000, 0x0C0, 0x120, 0x110, 0x011, 0x009, 0x006, 0x000, 0x000},   /* BASS: a wave */
    {0x010, 0x038, 0x07C, 0x07C, 0x07C, 0x0FE, 0x1FF, 0x000, 0x010},   /* KEYS: a bell */
    {0x000, 0x036, 0x07F, 0x0FE, 0x1FF, 0x1FF, 0x0FE, 0x000, 0x000},   /* PAD: a cloud */
    {0x010, 0x010, 0x038, 0x1FF, 0x0FE, 0x07C, 0x0EE, 0x0C6, 0x101},   /* LEAD: a star */
    {0x101, 0x183, 0x1FF, 0x1BB, 0x1FF, 0x1EF, 0x0FE, 0x07C, 0x000}};  /* VOICE: her head */
static const uint8_t ICON_DICE[7] = {0x7F, 0x41, 0x51, 0x49, 0x45, 0x41, 0x7F};
static const uint8_t ICON_SPEAKER[7] = {0x08, 0x1A, 0x79, 0x79, 0x79, 0x1A, 0x08};
static const uint8_t ICON_CROSS[7] = {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41};
static const uint8_t ICON_UNDO[7] = {0x10, 0x30, 0x7E, 0x31, 0x11, 0x01, 0x1E};
static const uint8_t ICON_PAW[5] = {0x15, 0x00, 0x0E, 0x1F, 0x0E};
static const uint8_t SPR_KITTY[6] = {0x41, 0x63, 0x7F, 0x6B, 0x7F, 0x3E};        /* 7 x 6: a kitten's head, its eyes open */
static const uint8_t SPR_HEART[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};              /* 5 x 5 */
static const uint8_t SPR_NOTE[7] = {0x0C, 0x0A, 0x09, 0x08, 0x38, 0x78, 0x30};   /* 7 x 7: a note, its flag */
static const uint8_t SPR_SPARK[5] = {0x04, 0x04, 0x1F, 0x04, 0x04};              /* 5 x 5: a twinkle */
static void spr(const uint8_t *b, int32_t w, int32_t h, int32_t x, int32_t y, uint16_t c)   /* bit w-1 at the left */
{
    int32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (b[j] >> (w - 1 - i) & 1u) cv_pset(x + i, y + j, c);
}
static void spr2(const uint8_t *b, int32_t w, int32_t h, int32_t x, int32_t y, uint16_t c)   /* twice the size */
{
    int32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (b[j] >> (w - 1 - i) & 1u) cv_rect(x + 2 * i, y + 2 * j, 2, 2, c);
}
static void kitty(int32_t cx, int32_t cy, uint16_t c) { spr(SPR_KITTY, 7, 6, cx - 3, cy - 3, c); }   /* a kitten planet */
static void ears(int32_t xl, int32_t xr, int32_t y, uint16_t c)   /* two cat's ears, their tips at y, on a box below */
{
    int32_t k;
    for (k = 0; k < 3; k++) { cv_rect(xl - k, y + k, 2 * k + 1, 1, c); cv_rect(xr - k, y + k, 2 * k + 1, 1, c); }
}
static void icon9(const uint16_t *b, int32_t x, int32_t y, int32_t sc, uint16_t c)
{
    int32_t i, j;
    for (j = 0; j < 9; j++)
        for (i = 0; i < 9; i++)
            if (b[j] >> (8 - i) & 1u) cv_rect(x + i * sc, y + j * sc, sc, sc, c);
}
static void icon_w(const uint8_t *b, int32_t n, int32_t x, int32_t y, uint16_t c)   /* n x n, bit n-1 at the left */
{
    int32_t i, j;
    for (j = 0; j < n; j++)
        for (i = 0; i < n; i++)
            if (b[j] >> (n - 1 - i) & 1u) cv_pset(x + i, y + j, c);
}
static uint32_t part_kind(uint32_t part) { return orb_lib_kind(&sys.snd[part % ORB_NPART]); }
static void cat_draw_q8(int32_t ax, int32_t ay, uint32_t s, uint32_t acx, uint32_t acy, int blink, uint32_t look);
static void cat_draw_small(int32_t cx, int32_t cy, uint32_t s);
static int32_t part_level(uint32_t p);
static uint8_t tour[4], tour_n;                 /* the pages of the button pressed last (the hint line, a moment) */
static uint32_t tour_t;
/* STEPS: the black keys (F# G# A# C# below: the track; D#: an IDEA; F# G# A# C# above: mute; D#: CLEAR; F#: UNDO) */
static const uint8_t BLACK_KEY[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
enum { BK_A, BK_B, BK_C, BK_D, BK_IDEA, BK_MA, BK_MB, BK_MC, BK_MD, BK_CLEAR, BK_UNDO };

/* ---- drawing */
static uint32_t drawn_bar = 0xFFFFFFFFu, knobs_dirty = 1, main_dirty = 1, hint_dirty = 1, t_main;
static uint32_t save_mode, save_to, rec;
static uint32_t isel(void) { return (fm1_in.buttons >> BTN_SEL) & 1u; }

static void draw_bar(void)                      /* the top line: the page, the focus, the system, the sun, the tempo */
{
    char s[24];
    int32_t x;
    cv_begin(240, 22, C_BG);
    x = text_w(&FONT_S, PAGE_NAME[page]) + 12;
    pill(2, 3, x, 17, C_LIME);
    ears(7, x - 4, 0, C_LIME);
    tx(8, 3, &FONT_S, PAGE_NAME[page], C_INK, C_LIME);
    {   /* the focus: what the pages edit and the keys play */
        pill(x + 6, 3, 18, 17, RINGC[focus]);
        ears(x + 9, x + 20, 0, RINGC[focus]);
        txc(x + 15, 3, &FONT_S, focus ? ORB_PART_NAME[focus] : "K", C_INK, RINGC[focus]);
        x += 24;
    }
    if (save_mode) {
        char *p = put_s(s, "SAVE> ");
        sys_label(ORB_NSYSTEM + save_to, p);
        tx(x + 8, 3, &FONT_S, s, C_MAG, C_BG);
    } else {
        int32_t rw;                             /* (the right side: the sun's note, the tempo, her head, PLAY) */
        char t[12], *q = put_s(t, NOTE_NAME[sys.sun % 12u]);
        *q++ = ' ';
        put_u(q, sys.bpm);
        rw = text_w(&FONT_S, t) + (eng.playing ? 14 : 4) + 17;
        sys_label(sysi, s);
        if (x + 8 + text_w(&FONT_S, s) < 240 - rw - 4) tx(x + 8, 3, &FONT_S, s, C_GREY, C_BG);
    }
    {   /* right: the sun and the tempo */
        char *p = put_s(s, NOTE_NAME[sys.sun % 12u]);
        int32_t w;
        *p++ = ' ';
        p = put_u(p, sys.bpm);
        w = text_w(&FONT_S, s);
        tx(240 - w - (eng.playing ? 14 : 4), 3, &FONT_S, s, C_TXT, C_BG);
        icon9(KIND_ICON[LK_VOICE], 240 - w - (eng.playing ? 14 : 4) - 13, 6, 1, C_YEL);   /* (the sun is her) */
        if (eng.playing) {                      /* a play triangle */
            int32_t k;
            for (k = 0; k < 7; k++) cv_rect(229 + k, 5 + k / 2 + (k & 1), 1, 12 - 2 * (k / 2) - 2 * (k & 1), C_LIME);
        }
    }
    if (rec) cv_rect(2, 20, x, 2, C_MAG);
    cv_rect(0, 21, 240, 1, C_FDIM);
    cv_blit(0, 0);
}

static void draw_knobs(void)                    /* K1..K4 as four boxes; SEL held: the second row */
{
    uint32_t k, lay = isel() ? 4u : 0u;
    cv_begin(240, 56, C_BG);
    for (k = 0; k < 4u; k++) {
        const pd_t *d = &PD[page][k + lay];
        int32_t x = 1 + (int32_t)k * 60, w = 58, fill;
        char s[24];
        uint16_t acc = lay ? C_MAG : C_LIME;
        frame(x, 1, w, 54, d->where == W_NONE && d->fmt != F_SLOT ? C_FDIM : C_FRAME);
        if (d->where == W_NONE && d->fmt != F_SLOT) continue;
        txc(x + w / 2, 3, &FONT_S, d->name, lay ? C_MAG : C_GREY, C_BG);
        pd_text(d, s);
        txc(x + w / 2, 21, &FONT_S, s, C_TXT, C_BG);
        fill = (int32_t)(pd_fill(d) * (uint32_t)(w - 10) >> 10);
        cv_rect(x + 5, 43, w - 10, 6, C_FDIM);
        cv_rect(x + 5, 43, fill, 6, acc);
        spr(ICON_PAW, 5, 5, x + 3 + (fill > 2 ? fill - 2 : 0), 43, C_TXT);   /* (a paw at its end) */
        if (pd_signed(d) && d->fmt == F_SIGNED) cv_rect(x + w / 2, 41, 1, 10, C_TXT);
    }
    cv_blit(0, 184);
}

static void draw_hint(void)                     /* the line above the knobs: ALGORITHM's value, or a message */
{
    char s[40], *p = s;
    const pd_t *d = &PD[page][8];
    cv_begin(240, 16, C_BG);
    if (msg_t && fm1_ms < msg_t) txc(120, 0, &FONT_S, msg, C_YEL, C_BG);
    else if (tour_t && fm1_ms < tour_t && tour_n > 1u) {   /* this button's pages, this one lit */
        uint32_t i;
        int32_t w = 0, x;
        for (i = 0; i < tour_n; i++) w += text_w(&FONT_S, PAGE_NAME[tour[i]]) + (i ? 10 : 0);
        x = 120 - w / 2;
        for (i = 0; i < tour_n; i++) {
            if (i) { cv_rect(x + 3, 7, 3, 2, C_FDIM); x += 10; }
            x = tx(x, 0, &FONT_S, PAGE_NAME[tour[i]], tour[i] == page ? C_LIME : C_DIMT, C_BG);
        }
    } else {
        if (d->where != W_NONE || d->fmt == F_SLOT) {
            p = put_s(p, "ALG ");
            p = put_s(p, d->name);
            *p++ = ' ';
            pd_text(d, p);
            tx(4, 0, &FONT_S, s, C_GREY, C_BG);
        }
        {   /* the load, at the right */
            uint32_t pc = cpu_t * 100u / (5805u * FM1_TICKS_PER_US);
            char c[12], *q = put_s(c, "CPU ");
            q = put_u(q, pc > 99u ? 99u : pc);
            put_s(q, "%");
            tx(236 - text_w(&FONT_S, c), 0, &FONT_S, c, pc > 70u ? C_MAG : C_DIMT, C_BG);
        }
    }
    cv_blit(0, 168);
}

/* the orrery: the four rings around the sun, the planets on them, conjunctions; the lanes at the right */
static uint32_t flash_seen[ORB_NRING], flash_t[ORB_NRING], conj_seen, conj_t;

static uint32_t ring_progress(uint32_t r)       /* 0..65535 of the current step's way (the planet glides) */
{
    const orb_rstate_t *R = &eng.rs[r];
    uint32_t st = orb_step_ticks(&sys.ring[r]), el = eng.T - (R->base - st);
    if (!eng.playing || el > st) return 0;
    return el * 65535u / st;
}
static void draw_orrery(void)
{
    static const int16_t RAD[4] = {28, 41, 54, 67};
    int32_t cx = 80, cy = 72;
    uint32_t r, i;
    cv_begin(240, 144, C_BG);
    stars(240, 144, 77, 46);
    for (r = 0; r < ORB_NRING; r++) {
        const orb_ring_t *g = &sys.ring[r];
        uint32_t len = g->len ? g->len : 1u;
        uint16_t c = RINGC[r + 1u];
        int32_t rad = RAD[r];
        circle(cx, cy, rad, dim565(c, g->mute ? 2u : 4u), 3);
        for (i = 0; i < len; i++) {
            uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)i << 32) / len);
            int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad);
            uint32_t on = orb_step_on(&eng, r, i, 0);   /* (as this lap plays it: MUTATE's changes hollow) */
            if (on) {
                uint16_t cc = g->mute ? dim565(c, 5) : (on == 2u ? mix565(c, C_TXT, 8) : c);
                if (on == 2u && len <= 32u) spr(SPR_HEART, 5, 5, x - 2, y - 2, cc);   /* an accent: a heart */
                else cv_rect(x - 2, y - 2, 4, 4, cc);
                if (eng.rs[r].vm[i >> 5] >> (i & 31u) & 1u) cv_rect(x - 1, y - 1, 2, 2, C_BG);
            } else
                cv_pset(x, y, dim565(c, 6));
        }
        if (eng.playing) {                      /* the planet: between its last step and the next */
            const orb_rstate_t *R = &eng.rs[r];
            uint32_t idx = R->cur, ph = 0xC0000000u + (uint32_t)(((uint64_t)idx << 32) / len)
                         + (uint32_t)(((uint64_t)ring_progress(r) << 16) / len);
            int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad), big = fm1_ms - flash_t[r] < 120u;
            if (big) disc(x, y, 5, dim565(c, 6));
            kitty(x, y, big ? C_TXT : c);       /* the planets are her kittens */
            if (conj_t && fm1_ms - conj_t < 200u && big)   /* where the orbits meet: a twinkle */
                spr(SPR_SPARK, 5, 5, x + 4, y - 9, (fm1_ms >> 5) & 1u ? C_TXT : C_YEL);
        }
    }
    if (eng.conj_count != conj_seen) { conj_seen = eng.conj_count; conj_t = fm1_ms; }
    {   /* Astralica, the sun: she bobs on the beat, blinks, her eyes light where the orbits meet; stopped, she naps */
        uint32_t beat = eng.T % (4u * ORB_TPS), awake = eng.playing, flare = conj_t && fm1_ms - conj_t < 220u, k;
        int32_t bob = awake && beat < ORB_TPS ? 1 : 0;
        int blink = fm1_ms < happy_t ? 2 : !awake || (fm1_ms % 3700u) < 130u;
        static uint32_t bar_seen, note_t[3], note_n, was_awake;
        cat_draw_q8(cx, cy + bob, 256u, CAT_W, CAT_H, blink, awake && flare && blink != 2);
        if (awake && (!was_awake || eng.T / (16u * ORB_TPS) != bar_seen)) { bar_seen = eng.T / (16u * ORB_TPS); note_t[note_n++ % 3u] = fm1_ms; }
        was_awake = awake;
        for (k = 0; k < 3u; k++) {              /* a note rises from her each bar, and fades */
            uint32_t age = fm1_ms - note_t[k];
            if (!note_t[k] || age > 1500u) continue;
            spr(SPR_NOTE, 7, 7, cx + 16 + (int32_t)age / 70 + isin(age * 3000000u, 3), cy - 20 - (int32_t)age / 40,
                dim565(k == 1u ? C_CYAN : k == 2u ? C_MAG : C_YEL, (uint32_t)orb_clamp(15 - (int32_t)age / 100, 1, 15)));
        }
        if (fm1_ms - mew_t < 800u && mew_t) {   /* the keys: she sings along */
            static const char *const MEW[4] = {"mew", "nya", "mrrp", "prr"};
            const char *w = MEW[mew_n % 4u];
            int32_t bw = text_w(&FONT_S, w) + 8, bx = cx - 30 - bw, by = cy - 40;
            pill(bx, by, bw, 18, C_TXT);
            cv_rect(bx + bw - 4, by + 18, 3, 2, C_TXT); cv_pset(bx + bw - 1, by + 20, C_TXT);   /* its tail, towards her */
            tx(bx + 4, by + 1, &FONT_S, w, C_INK, C_TXT);
        }
        if (!awake) {                           /* zz */
            uint32_t t = fm1_ms % 2400u, k;
            for (k = 0; k < 2u; k++) {
                uint32_t u = (t + k * 1200u) % 2400u;
                int32_t zx = cx + 20 + (int32_t)u / 150, zy = cy - 16 - (int32_t)u / 90;
                tx(zx, zy, &FONT_S, "z", dim565(C_TXT, (uint32_t)orb_clamp(15 - (int32_t)u / 160, 2, 15)), C_BG);
            }
        }
    }
    /* the lanes: each track, its sound, its speed and level (napping: muted) */
    for (r = 0; r < ORB_NRING; r++) {
        const orb_ring_t *g = &sys.ring[r];
        const orb_snd_t *sn = &sys.snd[ORB_A + r];
        char s[16], *p;
        int32_t y = 4 + (int32_t)r * 35, lw;
        uint16_t c = RINGC[r + 1u], cc = g->mute ? C_FDIM : c, fl = fm1_ms - flash_t[r] < 90u ? C_TXT : cc;
        frame(156, y, 82, 33, g->mute ? C_FDIM : dim565(c, focus == ORB_A + r ? 15u : 8u));
        pill(159, y + 3, 15, 15, fl);
        txc(167, y + 2, &FONT_S, ORB_PART_NAME[r + 1u], C_INK, fl);
        tx(178, y + 2, &FONT_S, orb_lib_name(sn), g->mute ? C_DIMT : C_TXT, C_BG);
        p = put_s(s, ORB_RATIO_NAME[g->ratio % ORB_NRATIO]);
        (void)p;
        tx(160, y + 17, &FONT_S, s, g->mute ? C_DIMT : dim565(c, 11), C_BG);
        if (g->mute) tx(206, y + 17, &FONT_S, "zz", C_DIMT, C_BG);
        else {
            lw = sn->level * 30 / 99;
            cv_rect(202, y + 24, 31, 3, C_FDIM);
            cv_rect(202, y + 24, lw, 3, c);
        }
    }
    cv_blit(0, 24);
}

/* MACROS: five tiles with an icon, a knob arc and the amount */
static void arc(int32_t cx, int32_t cy, int32_t r, uint32_t fill, uint16_t c, uint16_t bg)   /* 270 deg, from the lower left */
{
    uint32_t k;
    for (k = 0; k <= 48u; k++) {
        uint32_t ph = 0x60000000u + k * (0xC0000000u / 48u);
        int32_t x = cx + icos(ph, r), y = cy + isin(ph, r);
        uint16_t cc = k * 1024u / 48u <= fill ? c : bg;
        cv_pset(x, y, cc); cv_pset(x, y + 1, cc); cv_pset(x + 1, y, cc);
    }
}
static const uint16_t SPR_FISH[7] = {0x070, 0x1F9, 0x3BF, 0x7FE, 0x3FF, 0x1F9, 0x070};   /* 11 x 7, facing left */
static void spr16(const uint16_t *b, int32_t w, int32_t h, int32_t x, int32_t y, uint16_t c)
{
    int32_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            if (b[j] >> (w - 1 - i) & 1u) cv_pset(x + i, y + j, c);
}
static void macro_icon(uint32_t id, int32_t cx, int32_t cy, uint16_t c)
{
    uint32_t t = fm1_ms, k, ph;
    switch (id) {
    case MAC_CHAOS: {                           /* a ball of yarn, turning, its thread loose */
        int32_t dx, dy, ca = orb_cos(t * 150000u) >> 11, sa = orb_sin(t * 150000u) >> 11;   /* (Q4) */
        for (dy = -8; dy <= 8; dy++)
            for (dx = -8; dx <= 8; dx++) {
                int32_t u, v;
                if (dx * dx + dy * dy > 70) continue;
                u = (dx * ca + dy * sa) >> 4;
                v = (dy * ca - dx * sa) >> 4;
                u += v * v / 10;                /* the strands curve round the ball */
                cv_pset(cx - 2 + dx, cy - 1 + dy, (u + 30) % 3 == 0 ? mix565(c, C_INK, 8) : dx + dy < -9 ? mix565(c, C_TXT, 6) : c);
            }
        for (k = 0; k < 9u; k++) cv_pset(cx + 4 + (int32_t)k, cy + 7 + isin(k * 0x1C000000u + t * 2500000u, 2), c);
        break;
    }
    case MAC_SPECTRAL: {                        /* a rainbow */
        static const uint16_t RB[4] = {C_MAG, C_YEL, C_LIME, C_CYAN};
        for (k = 0; k < 4u; k++)
            for (ph = 0; ph <= 48u; ph++) {
                uint32_t a = 0x80000000u + ph * (0x80000000u / 48u);
                int32_t r = 12 - (int32_t)k * 2;
                cv_pset(cx + icos(a, r), cy + 6 + isin(a, r), RB[k]);
                cv_pset(cx + icos(a, r - 1), cy + 6 + isin(a, r - 1), RB[k]);
            }
        break;
    }
    case MAC_GRAVITY:                           /* three orbits round a nucleus */
        for (k = 0; k < 3u; k++)
            for (ph = 0; ph < 48u; ph++) {
                uint32_t a = (ph << 26) + (ph << 24), rot = k * 0x55555555u;
                int32_t ex = icos(a, 10), ey = isin(a, 4);
                cv_pset(cx + ((ex * orb_cos(rot) - ey * orb_sin(rot)) >> 15), cy + ((ex * orb_sin(rot) + ey * orb_cos(rot)) >> 15), c);
            }
        disc(cx, cy, 2, C_TXT);
        break;
    case MAC_FRACT:                             /* paws, walking up */
        for (k = 0; k < 3u; k++)
            spr(ICON_PAW, 5, 5, cx - 9 + (int32_t)k * 6, cy + 3 - (int32_t)k * 5, (t / 260u) % 4u > k ? C_TXT : c);
        break;
    case MAC_TIDE:                              /* a fish, bobbing on the tide */
        for (k = 0; k < 20u; k++) cv_pset(cx - 10 + (int32_t)k, cy + 7 + isin(k * 0x18000000u + t * 3000000u, 2), c);
        spr16(SPR_FISH, 11, 7, cx - 6, cy - 6 + isin(t * 3000000u, 2), C_TXT);
        cv_pset(cx - 2, cy - 4 + isin(t * 3000000u, 2), C_INK);
        break;
    case MAC_HORIZON:                           /* a black hole with ears, its ring */
        for (ph = 0; ph < 64u; ph++) cv_pset(cx + icos(ph << 26, 11), cy + 2 + isin(ph << 26, 3), ph & 1u ? c : C_YEL);
        for (k = 0; k < 4u; k++) { cv_rect(cx - 4 - (int32_t)k, cy - 8 + (int32_t)k, 2 * (int32_t)k + 1, 1, c); cv_rect(cx + 4 - (int32_t)k, cy - 8 + (int32_t)k, 2 * (int32_t)k + 1, 1, c); }
        disc(cx, cy + 2, 5, C_INK);
        circle(cx, cy + 2, 6, c, 0);
        break;
    case MAC_ORBIT: {                           /* a kitten going round */
        uint32_t a = t * 1200000u;
        circle(cx, cy, 3, C_TXT, 0);
        for (ph = 0; ph < 48u; ph++) cv_pset(cx + icos((ph << 26) + (ph << 24), 11), cy + isin((ph << 26) + (ph << 24), 5), c);
        kitty(cx + icos(a, 11), cy + isin(a, 5), C_TXT);
        break;
    }
    default:                                    /* MUTATE: a strand of yarn */
        for (k = 0; k < 16u; k++) {
            int32_t x = cx - 8 + (int32_t)k, y = isin(k * 0x10000000u + t * 2000000u, 4);
            cv_pset(x, cy + y, c);
            cv_pset(x, cy - y, C_TXT);
        }
        break;
    }
}
static void draw_macros(void)                   /* two rows of four, as the knobs: K1..K4 on top, SEL K1..K4 below */
{
    static const uint8_t ID[8] = {MAC_CHAOS, MAC_SPECTRAL, MAC_GRAVITY, MAC_ORBIT, MAC_FRACT, MAC_TIDE, MAC_HORIZON, 99};
    static const uint16_t MC[8] = {C_VIO, C_MAG, C_YEL, C_LIME, C_CYAN, C_CYAN, C_MAG, C_VIO};
    static const char *const SHORT[8] = {"CHAOS", "SPECT", "GRAV", "ORBIT", "FRACT", "TIDE", "HORIZ", "MUTAT"};
    uint32_t m, lay = isel();
    cv_begin(240, 144, C_BG);
    stars(240, 144, 31, 26);
    for (m = 0; m < 8u; m++) {
        uint32_t v = ID[m] == 99u ? sys.mutate : sys.macro[ID[m]];
        int32_t x = 1 + (int32_t)(m % 4u) * 60, y = 2 + (int32_t)(m / 4u) * 71, cx = x + 29, w = 58;
        uint16_t c = MC[m];
        char s[8];
        frame(x, y, w, 69, (m / 4u) == lay ? C_FRAME : C_FDIM);
        arc(cx, y + 22, 17, v * 1024u / 99u, c, C_FDIM);
        macro_icon(ID[m], cx, y + 22, c);
        put_s(put_u(s, v), "%");
        txc(cx, y + 34, &FONT_S, s, C_TXT, C_BG);
        pill(x + 3, y + 51, w - 6, 16, c);
        txc(cx, y + 51, &FONT_S, SHORT[m], C_INK, c);
    }
    cv_blit(0, 24);
}

/* STEPS: the track's ring, its hits as paws (further out the higher the note), the page of 16 on the keys marked;
 * its sound; below, the keys as they are now: the white ones its steps, the black ones what they do */
static uint32_t step_page;
static const uint8_t BLACK_AFTER[11] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14};   /* the white key left of each black one */
static void draw_ring(void)
{
    uint32_t r = ring_focus(), len, i;
    const orb_ring_t *g = &sys.ring[r];
    const orb_snd_t *sn = &sys.snd[ORB_A + r];
    uint16_t c = RINGC[r + 1u];
    int32_t cx = 62, cy = 51, R = 41;
    len = g->len ? g->len : 1u;
    cv_begin(240, 144, C_BG);
    stars(240, 104, 5, 22);
    circle(cx, cy, R, dim565(c, 4), 2);
    for (i = 0; i < len; i++) {
        uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)i << 32) / len);
        int32_t rad = R + (g->on[i] ? orb_clamp(g->dg[i], -6, 6) : 0);
        int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad);
        if (i / 16u == step_page) cv_pset(cx + icos(ph, R + 9), cy + isin(ph, R + 9), C_LIME);
        if (g->on[i]) {
            uint16_t pc = g->on[i] == 2u ? C_TXT : c;
            if (len <= 32u) spr(g->on[i] == 2u ? SPR_HEART : ICON_PAW, 5, 5, x - 2, y - 2, pc);   /* a paw; an accent a heart */
            else cv_rect(x - 1, y - 1, 3, 3, pc);
        } else
            cv_rect(x, y, 1, 1, dim565(c, 8));
    }
    if (eng.playing) {
        uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)eng.rs[r].cur << 32) / len);
        disc(cx + icos(ph, R - 10), cy + isin(ph, R - 10), 3, fm1_ms - flash_t[r] < 100u ? C_TXT : dim565(C_TXT, 10));
    }
    icon9(KIND_ICON[part_kind(ORB_A + r)], cx - 9, cy - 9, 2, fm1_ms - flash_t[r] < 100u ? C_TXT : c);
    if (love_t && fm1_ms - love_t < 900u) {     /* an edit: love */
        uint32_t age = fm1_ms - love_t, k;
        for (k = 0; k < 3u; k++) {
            int32_t a = (int32_t)age - (int32_t)k * 120;
            if (a < 0) continue;
            spr(SPR_HEART, 5, 5, cx - 12 + (int32_t)k * 10 + isin((uint32_t)a * 4000000u, 2), cy - 14 - a / 25,
                dim565(k == 1u ? C_TXT : C_MAG, (uint32_t)orb_clamp(15 - a / 60, 1, 15)));
        }
    }
    {   /* the side: the track, its sound, its speed, the page */
        char s[20], *p;
        frame(124, 2, 114, 98, dim565(c, 9));
        pill(129, 6, 18, 18, c);
        txc(138, 7, &FONT_S, ORB_PART_NAME[r + 1u], C_INK, c);
        tx(152, 7, &FONT_S, ORB_LK_NAME[part_kind(ORB_A + r)], C_GREY, C_BG);
        tx(130, 26, &FONT_L, orb_lib_name(sn), c, C_BG);
        p = put_s(s, "SPEED ");
        put_s(p, ORB_RATIO_NAME[g->ratio % ORB_NRATIO]);
        tx(130, 62, &FONT_S, s, C_TXT, C_BG);
        if (g->mute) put_s(s, "NAPPING zz");
        else if (rec) put_s(s, "REC");
        else { p = put_s(s, "KEYS "); p = put_u(p, step_page * 16u + 1u); *p++ = '-'; put_u(p, step_page * 16u + 16u); }
        tx(130, 80, &FONT_S, s, g->mute || rec ? C_MAG : C_LIME, C_BG);
    }
    /* the keys: the white ones the steps of this page, the black ones: A B C D, IDEA, mute A B C D, CLEAR, UNDO */
    for (i = 0; i < 16u; i++) {
        uint32_t st = step_page * 16u + i, on = st < g->len ? g->on[st] : 0u;
        int32_t x = 2 + (int32_t)i * 15, play = eng.playing && st == eng.rs[r].cur;
        if (st >= g->len) { cv_rect(x, 124, 14, 19, dim565(C_FDIM, 6)); continue; }
        cv_rect(x, 124, 14, 19, on == 2u ? C_TXT : on ? c : dim565(C_GREY, 4));
        if (play) frame(x - 1, 123, 16, 21, C_LIME);
    }
    for (i = 0; i < 11u; i++) {
        int32_t x = 2 + (int32_t)(BLACK_AFTER[i] + 1u) * 15 - 7, y = 104;
        uint16_t bg = C_PANEL;
        if (i <= BK_D) {
            uint16_t tc = RINGC[i + 1u];
            int sel = focus == ORB_A + i;
            pill(x, y, 13, 17, sel ? tc : C_PANEL);
            txc(x + 7, y + 1, &FONT_S, ORB_PART_NAME[i + 1u], sel ? C_INK : tc, sel ? tc : C_PANEL);
            continue;
        }
        pill(x, y, 13, 17, bg);
        if (i == BK_IDEA) icon_w(ICON_DICE, 7, x + 3, y + 5, C_LIME);
        else if (i <= BK_MD) {
            uint32_t q = i - BK_MA;
            icon_w(ICON_SPEAKER, 7, x + 3, y + 5, sys.ring[q].mute ? C_FDIM : RINGC[q + 1u]);
        } else if (i == BK_CLEAR) icon_w(ICON_CROSS, 7, x + 3, y + 5, C_MAG);
        else icon_w(ICON_UNDO, 7, x + 3, y + 5, C_CYAN);
    }
    cv_blit(0, 24);
}

/* SOUND: what the focus plays, big: its kind, its name, the library's place; below, its wave as it sounds */
static void draw_sound(void)
{
    const orb_snd_t *sn = &sys.snd[focus];
    uint16_t c = RINGC[focus];
    uint32_t kind = part_kind(focus), lv = (uint32_t)part_level(focus);
    char s[20], *p;
    cv_begin(240, 144, C_BG);
    stars(240, 100, 3, 20);
    frame(2, 2, 116, 98, dim565(c, 7));
    {   /* the kind's picture, a little bigger while it sounds */
        int32_t sc = 4 + (lv > 9000u ? 1 : 0), w = 9 * sc;
        icon9(KIND_ICON[kind], 60 - w / 2, 44 - w / 2, sc, lv > 3000u ? mix565(c, C_TXT, 6) : c);
    }
    txc(60, 80, &FONT_S, ORB_LK_NAME[kind], C_GREY, C_BG);
    {   /* whose: a track, or the keys */
        int32_t x = tx(126, 5, &FONT_S, focus ? "TRACK " : "THE KEYS", C_GREY, C_BG);
        if (focus) tx(x, 5, &FONT_S, ORB_PART_NAME[focus], c, C_BG);
    }
    tx(126, 28, &FONT_L, orb_lib_name(sn), c, C_BG);
    p = put_s(s, "SOUND ");
    if (sn->lib && sn->lib < ORB_NLIB) { p = put_u(p, sn->lib); p = put_s(p, "/"); put_u(p, ORB_NLIB - 1u); }
    else put_s(p, "-");
    tx(126, 64, &FONT_S, s, C_DIMT, C_BG);
    tx(126, 82, &FONT_S, "K1 PICKS", C_GREY, C_BG);
    frame(2, 104, 236, 39, C_FDIM);
    {
        uint32_t w = eng.scope_w, i;
        int32_t lx = 0, ly = 0;
        for (i = 0; i < 116u; i++) {
            int32_t v = eng.scope[(w + 12u + i) & 127u], y = 123 - (v * 16 >> 15), x = 4 + (int32_t)i * 2;
            if (i) cv_line(lx, ly, x, y, C_LIME);
            lx = x; ly = y;
        }
    }
    cv_blit(0, 24);
}

/* TIME: the polyrhythm lanes: one bar (16 base steps) across, every ring's hits where they fall in it, a playhead */
static void draw_time(void)
{
    uint32_t r;
    int32_t x0 = 30, W = 204;
    cv_begin(240, 144, C_BG);
    for (r = 0; r < 16u; r++) cv_rect(x0 + (int32_t)r * W / 16, 4, 1, 132, r % 4u ? dim565(C_FDIM, 9) : C_FDIM);
    for (r = 0; r < ORB_NRING; r++) {
        const orb_ring_t *g = &sys.ring[r];
        uint32_t st = orb_step_ticks(g), len = g->len ? g->len : 1u, k, n = (16u * ORB_TPS) / st + 1u;
        int32_t y = 8 + (int32_t)r * 33;
        uint16_t c = RINGC[r + 1u];
        char s[4] = {'L', (char)('1' + r), 0, 0};
        pill(2, y + 4, 24, 18, g->mute ? C_FDIM : c);
        txc(14, y + 5, &FONT_S, s, C_INK, g->mute ? C_FDIM : c);
        for (k = 0; k < n && k < 128u; k++) {   /* step k of the bar: where it falls, if it is a hit (FWD) */
            uint32_t t = k * st, idx = k % len;
            int32_t x = x0 + (int32_t)((t * (uint32_t)W) / (16u * ORB_TPS));
            if (x >= x0 + W) break;
            if (g->on[idx]) {
                int32_t h = 8 + orb_clamp(g->dg[idx], -4, 8);
                cv_rect(x, y + 22 - h, (int32_t)(st * (uint32_t)W / (16u * ORB_TPS)) > 4 ? 4 : 3, h, g->mute ? C_FDIM : g->on[idx] == 2u ? C_TXT : c);
            } else
                cv_rect(x, y + 21, 2, 1, dim565(c, 4));
        }
    }
    if (eng.playing) {
        uint32_t bar = 16u * ORB_TPS, ph = eng.T % bar;
        int32_t x = x0 + (int32_t)((ph / 16u) * (uint32_t)W / (bar / 16u));
        cv_rect(x, 10, 2, 128, C_LIME);
        kitty(x + 1, 5, C_LIME);
    }
    cv_blit(0, 24);
}

/* ORBIT and BODY: the orbit's path (eccentric, its moon), the L and R views, the live wave */
static void draw_orbit(void)
{
    const orb_snd_t *s = &sys.snd[focus];
    int32_t cx = 66, cy = 70, R = 50, k, px = 0, py = 0;
    int32_t e = orb_clamp((int32_t)s->ecc * 662 + sys.macro[MAC_SPECTRAL] * 330, 0, 65535);
    int32_t ef = (e * 31457) >> 16;              /* the physical e, Q15 */
    int32_t mr = (orb_lin(s->moon) * 3) >> 2;
    uint32_t ratio = ORB_MRATIO[s->mratio % ORB_NMOON], th = (uint32_t)s->ang * 21474836u, sp = (uint32_t)s->spr * 10737418u;
    uint16_t c = RINGC[focus];
    cv_begin(240, 144, C_BG);
    stars(240, 144, 9, 26);
    frame(2, 2, 130, 140, C_FDIM);
    /* the views: L and R axes through the centre */
    cv_line(cx - icos(th - sp, 60), cy - isin(th - sp, 60), cx + icos(th - sp, 60), cy + isin(th - sp, 60), dim565(C_CYAN, 6));
    cv_line(cx - icos(th + sp, 60), cy - isin(th + sp, 60), cx + icos(th + sp, 60), cy + isin(th + sp, 60), dim565(C_MAG, 6));
    for (k = 0; k <= 128; k++) {               /* the path: x = cos E + e/2 (geometric: b sin E), the moon on it */
        uint32_t M = (uint32_t)k << 25, row = (uint32_t)e >> 11, col = M >> 23;
        int32_t d0 = ORB_KEP[row][col], d1 = ORB_KEP[row < 32u ? row + 1u : row][col], d = d0 + (((d1 - d0) * (e & 2047)) >> 11);
        uint32_t E = M + ((uint32_t)d << 16);
        int32_t b = 32767 - ((ef * ef) >> 16), x = orb_cos(E) + ef / 2, y = (orb_sin(E) * b) >> 15, sx, sy;   /* (b ~ sqrt(1-e^2)) */
        x += (mr * orb_cos(M * (ratio >> 8) + (M >> 8) * (ratio & 255u))) >> 15;
        y += (mr * orb_sin(M * (ratio >> 8) + (M >> 8) * (ratio & 255u))) >> 15;
        sx = cx + ((x * R) >> 15);
        sy = cy + ((y * R) >> 15);
        if (k) cv_line(px, py, sx, sy, k & 1 ? c : mix565(c, C_TXT, 6));
        px = sx; py = sy;
    }
    cat_draw_small(cx + ((ef * R) >> 16) + (((ef * R) >> 15) >> 1), cy, 72u);   /* the sun at the focus: her */
    {   /* the planet, going round (fast at the near point) */
        uint32_t M = fm1_ms * 1500000u, row = (uint32_t)e >> 11, col = M >> 23;
        uint32_t E = M + ((uint32_t)ORB_KEP[row][col] << 16);
        int32_t b = 32767 - ((ef * ef) >> 16);
        kitty(cx + (((orb_cos(E) + ef / 2) * R) >> 15), cy + (((orb_sin(E) * b) >> 15) * R >> 15), C_TXT);
    }
    /* the live wave */
    frame(136, 2, 102, 76, C_FDIM);
    tx(142, 4, &FONT_S, "LIVE", C_GREY, C_BG);
    {
        uint32_t w = eng.scope_w, i;
        int32_t lx = 0, ly = 0;
        for (i = 0; i < 96u; i++) {
            int32_t v = eng.scope[(w + 32u + i) & 127u], y = 50 - (v * 22 >> 15), x = 139 + (int32_t)i;
            if (i) cv_line(lx, ly, x, y, C_LIME);
            lx = x; ly = y;
        }
    }
    frame(136, 82, 102, 60, C_FDIM);
    {
        char t[24], *p;
        p = put_s(t, "e ");
        p = put_u(p, (uint32_t)(ef * 100 >> 15));
        p = put_s(p, " SL ");
        put_u(p, s->sling);
        tx(142, 86, &FONT_S, t, C_TXT, C_BG);
        p = put_s(t, "MOON ");
        put_s(p, s->moon ? ORB_MRATIO_NAME[s->mratio % ORB_NMOON] : "-");
        tx(142, 104, &FONT_S, t, C_GREY, C_BG);
        p = put_s(t, page == PG_BODY ? "FALL " : "DUST ");
        put_u(p, page == PG_BODY ? s->fall : s->dust);
        tx(142, 122, &FONT_S, t, C_GREY, C_BG);
    }
    cv_blit(0, 24);
}

/* ENV: the amplitude envelope (lime) and the eccentricity envelope (magenta) */
static void draw_env(void)
{
    const orb_snd_t *s = &sys.snd[focus];
    int32_t x0 = 10, y0 = 128, H = 110, ta, td, tr, sus, k, lx = x0, ly = y0;
    cv_begin(240, 144, C_BG);
    frame(2, 2, 236, 140, C_FDIM);
    for (k = 1; k < 8; k++) cv_rect(2 + k * 29, 6, 1, 132, dim565(C_FDIM, 7));
    ta = 4 + s->a * 50 / 99; td = 8 + s->d * 60 / 99; tr = 8 + s->r * 60 / 99;
    sus = y0 - s->s * H / 99;
    {   /* attack, decay (exponential-ish), the sustain, the release */
        int32_t pts[6][2] = {{x0, y0}, {x0 + ta, y0 - H}, {x0 + ta + td, sus}, {x0 + ta + td + 50, sus}, {x0 + ta + td + 50 + tr, y0}, {230, y0}};
        for (k = 1; k < 6; k++) {
            cv_line(pts[k - 1][0], pts[k - 1][1], pts[k][0], pts[k][1], C_LIME);
            cv_line(pts[k - 1][0], pts[k - 1][1] + 1, pts[k][0], pts[k][1] + 1, C_LIME);
        }
        for (k = 1; k < 5; k++) disc(pts[k][0], pts[k][1], 3, C_LIME);
        (void)lx; (void)ly;
    }
    if (s->eamt) {                              /* the eccentricity envelope: attack, decay to nothing */
        int32_t a = 4 + s->ea * 50 / 99, d = 8 + s->ed * 80 / 99, amt = s->eamt * (H / 2) / 99, y1 = y0 - 60 - amt;
        cv_line(x0, y0 - 60, x0 + a, y1, C_MAG);
        cv_line(x0 + a, y1, x0 + a + d, y0 - 60, C_MAG);
        tx(x0 + a + 6, y1 - 16 > 4 ? y1 - 16 : 4, &FONT_S, "ECC", C_MAG, C_BG);
    }
    cv_blit(0, 24);
}

/* MOD: the matrix, eight slots, the selected one marked */
static void draw_mod(void)
{
    uint32_t k;
    static const uint16_t SC[ORB_NSRC] = {C_DIMT, C_CYAN, C_CYAN, C_LIME, C_LIME, C_VIO, C_YEL, C_MAG, C_TXT, C_TXT, C_TXT};
    cv_begin(240, 144, C_BG);
    for (k = 0; k < ORB_NMOD; k++) {
        const orb_mod_t *m = &sys.mod[k];
        int32_t y = 2 + (int32_t)k * 18, sel = k == slot;
        char s[24];
        if (sel) pill(0, y, 240, 17, C_PANEL);
        frame(1, y, 238, 17, sel ? C_FRAME : C_FDIM);
        put_u(s, k + 1u);
        tx(6, y + 1, &FONT_S, s, sel ? C_LIME : C_DIMT, sel ? C_PANEL : C_BG);
        tx(24, y + 1, &FONT_S, ORB_SRC_NAME[m->src % ORB_NSRC], m->src ? SC[m->src % ORB_NSRC] : C_DIMT, sel ? C_PANEL : C_BG);
        tx(84, y + 1, &FONT_S, ">", C_GREY, sel ? C_PANEL : C_BG);
        tx(98, y + 1, &FONT_S, ORB_DST_NAME[m->dst % ORB_NDST], m->src ? C_TXT : C_DIMT, sel ? C_PANEL : C_BG);
        put_i(s, m->amt, 1);
        tx(200, y + 1, &FONT_S, s, m->amt < 0 ? C_MAG : C_CYAN, sel ? C_PANEL : C_BG);
        if (m->src && m->amt) {                 /* a bar of the source's value now */
            int32_t v = eng.src[m->src % ORB_NSRC] * 18 >> 15;
            cv_rect(160 + (v < 0 ? v : 0), y + 12, v < 0 ? -v : v + 1, 2, SC[m->src % ORB_NSRC]);
        }
    }
    cv_blit(0, 24);
}

/* LFO: the two shapes, moving */
static void draw_lfo(void)
{
    uint32_t k, i;
    cv_begin(240, 144, C_BG);
    for (k = 0; k < 2u; k++) {
        int32_t y0 = 4 + (int32_t)k * 70, lx = 0, ly = 0;
        uint16_t c = k ? C_MAG : C_CYAN;
        frame(2, y0, 236, 66, C_FDIM);
        tx(8, y0 + 2, &FONT_S, k ? "LFO 2" : "LFO 1", c, C_BG);
        for (i = 0; i < 220u; i++) {
            uint32_t ph = i * (0xFFFFFFFFu / 110u);
            int32_t v = orb_shape(ph, sys.lshape[k] % ORB_NLSHAPE, (int32_t)((i / 55u) * 9000u) - 13000, (int32_t)((i / 55u) * 9000u) - 22000);
            int32_t x = 10 + (int32_t)i, y = y0 + 38 - (v * 22 >> 15);
            if (i) cv_line(lx, ly, x, y, dim565(c, 10));
            lx = x; ly = y;
        }
        {
            int32_t v = eng.lval[k], y = y0 + 38 - (v * 22 >> 15);
            kitty(231, y, C_TXT);
        }
    }
    cv_blit(0, 24);
}

/* FILTER MAP: a contour map round the point (X = RES, Y = CUT); the mode on the left, LP at the bottom */
static void draw_filter(void)
{
    int32_t x0 = 36, y0 = 4, W = 196, H = 136, px = x0 + sys.res * W / 99, py = y0 + H - sys.cut * H / 99, x, y;
    cv_begin(240, 144, C_BG);
    for (y = 0; y < H; y += 2)
        for (x = 0; x < W; x += 2) {
            int32_t dx = x0 + x - px, dy = y0 + y - py;
            uint32_t h = (uint32_t)(x * 7 + y * 13) * 2654435761u;
            int32_t d = (dx * dx + dy * dy) / 64 + (int32_t)((h >> 28) & 3u) + ((orb_sin((uint32_t)x * 0x3000000u + (uint32_t)y * 0x1C00000u) * 6) >> 15);
            int32_t band = d / 6;
            uint16_t c = band < 1 ? C_YEL : band < 3 ? C_MAG : band < 6 ? C_VIO : band < 10 ? C_FRAME : band < 16 ? C_FDIM : C_PANEL;
            if ((d % 6) == 0 && band < 16) c = mix565(c, C_TXT, 5);
            cv_rect(x0 + x, y0 + y, 2, 2, c);
        }
    frame(x0 - 1, y0 - 1, W + 2, H + 2, C_FRAME);
    cv_rect(px - 6, py, 13, 1, C_TXT);
    cv_rect(px, py - 6, 1, 13, C_TXT);
    tx(4, 8, &FONT_S, "HP", sys.fmode > 66 ? C_LIME : C_GREY, C_BG);
    tx(4, 64, &FONT_S, "BP", sys.fmode > 33 && sys.fmode <= 66 ? C_LIME : C_GREY, C_BG);
    tx(4, 120, &FONT_S, "LP", sys.fmode <= 33 ? C_LIME : C_GREY, C_BG);
    disc(28, 128 - sys.fmode * 112 / 99, 3, C_LIME);
    if (!eng.f_on) txc(x0 + W / 2, y0 + H - 20, &FONT_S, "OPEN", C_TXT, C_PANEL);
    cv_blit(0, 24);
}

/* SPACE: the echoes falling away, the nebula behind */
static void draw_space(void)
{
    uint32_t k, s = 1234;
    int32_t a = 120, x;
    cv_begin(240, 144, C_BG);
    for (k = 0; k < 40u + sys.rmix * 3u; k++) {   /* the nebula: its size spreads it, its mix fills it */
        int32_t r = (int32_t)(orb_xs(&s) % (uint32_t)(20 + sys.rsize)), ph = (int32_t)orb_xs(&s);
        uint16_t c = k & 1u ? C_VIO : C_MAG;
        cv_pset(170 + icos((uint32_t)ph, r), 72 + isin((uint32_t)ph, r) / 2, dim565(c, 4u + (orb_xs(&s) & 7u)));
    }
    for (k = 0, x = 12; k < 10u && a > 6; k++) {   /* the taps: each the feedback's share of the last */
        int32_t h = a * 100 / 120;
        cv_rect(x, 120 - h, 6, h, k ? dim565(C_CYAN, 6u + (uint32_t)a / 12u > 15u ? 15u : 6u + (uint32_t)a / 12u) : C_LIME);
        if (sys.ping && k) cv_rect(x, 122, 6, 2, k & 1u ? C_MAG : C_CYAN);
        x += 6 + (int32_t)(sys.dtime % ORB_NDT) * 2 + 4;
        a = a * (int32_t)sys.dfb / 99;
    }
    tx(8, 4, &FONT_S, "ECHO", C_CYAN, C_BG);
    tx(148, 4, &FONT_S, "NEBULA", C_VIO, C_BG);
    cv_blit(0, 24);
}

/* BACKGROUND: the first light, as its map is drawn (an oval of the whole sky, its tiny fluctuations warm and cold,
 * drifting), Astralica at its middle; below, the chord now (its name; in a just tuning its ratios), the progression
 * (the chord lit) and its voices, each a star where its note is, as bright as it sounds */
static void draw_bg(void)
{
    static const uint16_t MAP[8] = {RGB(20, 24, 110), RGB(36, 70, 200), RGB(60, 140, 235), RGB(120, 210, 240),
                                    RGB(236, 232, 200), RGB(255, 196, 70), RGB(250, 120, 60), RGB(220, 50, 70)};   /* cold .. warm */
    uint32_t tu = sys.tuning % ORB_NTUNING, pr = sys.bg_prog % ORB_NBGPROG, n = ORB_BGP_N[tu][pr], c = eng.bg_chord;
    int32_t cx = 120, cy = 44, x, y, k, on = eng.bg_on, an = 10 + sys.bg_width / 2;
    uint32_t t = fm1_ms * 5000u, hs = 0x51ED27u;
    const char *st = on ? "GLOWING" : !sys.bg_level ? "LEVEL 0: ASLEEP" : sys.bg_out ? "OUT  SEL+GLO: IN" : "PLAY: IT GLOWS";
    char s[24];
    if (c >= n) c = 0;
    cv_begin(240, 144, C_BG);
    stars(240, 144, 61, 30);
    for (y = -40; y <= 40; y += 2)
        for (x = -108; x <= 108; x += 2) {
            int32_t v, cc;
            uint32_t h;
            if (x * x * 40 * 40 / (108 * 108) + y * y > 40 * 40) continue;
            h = ((uint32_t)(x * 73856093) ^ (uint32_t)(y * 19349663) ^ hs) * 2654435761u;   /* (a grain each cell) */
            v = orb_sin((uint32_t)(x * 0x05000000) + (uint32_t)(y * 0x03400000) + t) +
                orb_sin((uint32_t)(x * -0x03C00000) + (uint32_t)(y * 0x06100000) - t / 2u) +
                (orb_sin((uint32_t)(x * 0x0B300000) - (uint32_t)(y * 0x08700000) + t / 3u) >> 1) +
                (orb_sin((uint32_t)((x + 2 * y) * 0x0E900000) + t * 2u) >> 1) + (int32_t)((h >> 17) & 0x7FFFu) - 16384;
            v = v * an / 45;
            cc = orb_clamp((v + 65536) * 8 / 131072, 0, 7);
            cv_rect(cx + x - 1, cy + y - 1, 2, 2, on ? MAP[cc] : dim565(MAP[cc], 4));
        }
    for (k = 0; k < 96; k++) {                  /* its edge; HALO, a shell of violet light about it, as full as it rings */
        uint32_t ph = (uint32_t)k << 26 | (uint32_t)k << 24, tw = (uint32_t)k * 2654435761u >> 28;
        cv_pset(cx + icos(ph, 109), cy + isin(ph, 42), C_FRAME);
        if (sys.bg_halo && eng.g_on) {
            uint32_t b = 6u + (uint32_t)eng.g_level * 9u / 99u, r = 112u + (tw & 3u) + ((fm1_ms / 300u + tw) & 1u);
            cv_pset(cx + icos(ph + (uint32_t)fm1_ms * 90000u, (int32_t)r), cy + isin(ph + (uint32_t)fm1_ms * 90000u,
                    (int32_t)(r * 42u / 109u) + 2), dim565(C_VIO, b > 15u ? 15u : b));
        }
    }
    cat_draw_small(cx, cy, 80u);
    put_s(put_s(s, "T "), sys.bg_temp <= -2 ? "0.68K" : sys.bg_temp == -1 ? "1.36K" : sys.bg_temp == 0 ? "2.725K" : "5.45K");
    tx(4, 88, &FONT_S, s, C_GREY, C_BG);
    tx(236 - text_w(&FONT_S, st), 88, &FONT_S, st, on ? C_LIME : C_DIMT, C_BG);
    {   /* the chord: its root large, what it is small beside it (and over what bass), its ratios under */
        uint32_t id = ORB_BGP_C[tu][pr][c], root = (sys.sun + ORB_BGC_ROOT[id]) % 12u, bass = (sys.sun + ORB_BGC_BASS[id]) % 12u;
        uint16_t col = on ? C_LIME : C_DIMT;
        char nm[4] = {NOTE_NAME[root][0], NOTE_NAME[root][1], 0, 0}, *q;
        x = tx(6, 106, &FONT_L, nm, col, C_BG) + 2;
        q = put_s(s, ORB_BGC_SUF[id]);
        if (bass != root) put_s(put_s(q, "/"), NOTE_NAME[bass]);
        tx(x, 108, &FONT_S, s, col, C_BG);
        tx(x, 124, &FONT_S, ORB_BGC_RATIO[id][0] ? ORB_BGC_RATIO[id] : ORB_TUNING_NAME[tu], C_GREY, C_BG);
    }
    {   /* the progression, the chord now lit; every CHANGE bars */
        char *q = put_s(s, ORB_BGPROG_NAME[pr]);
        int32_t w = (int32_t)n * 12 - 4, x0 = 236 - w;
        if (n > 1u) { q = put_s(q, "  "); put_s(put_u(q, 1u << (sys.bg_change & 3u)), "BR"); }
        tx(236 - text_w(&FONT_S, s), 104, &FONT_S, s, C_GREY, C_BG);
        for (k = 0; k < (int32_t)n; k++) {
            int lit = (uint32_t)k == c;
            cv_rect(x0 + k * 12, 122, 8, 5, lit ? (on ? C_LIME : C_DIMT) : C_FDIM);
            if (lit) cv_rect(x0 + k * 12 + 2, 120, 4, 9, on ? C_LIME : C_DIMT);
        }
    }
    {   /* its voices: a star each where its note is (an octave and a half below the sun's to three above), bright as
         * it sounds */
        int32_t mx = 1, base = (48 + (int32_t)sys.sun - (sys.sun > 6u ? 12 : 0)) * 256;
        cv_rect(130, 138, 106, 1, C_FDIM);
        for (k = 0; k < (int32_t)ORB_NBG; k++) if (eng.bg_a[k] > mx) mx = eng.bg_a[k];
        for (k = 0; k < (int32_t)ORB_NBG; k++) {
            int32_t a = eng.bg_a[k], nt = eng.bg_note[k] - base - 12 * 256 * orb_clamp(sys.bg_temp, -2, 1), b;
            if (!eng.bg_note[k] || a <= 0 || !on) continue;
            x = 130 + orb_clamp((nt + 9 * 256) * 106 / (45 * 256), 0, 105);
            b = 3 + a * 12 / mx;
            cv_rect(x - 1, 133, 3, 3, dim565(C_TXT, (uint32_t)b));
            if (b > 12) { cv_pset(x, 131, dim565(C_CYAN, 12)); cv_pset(x, 137, dim565(C_CYAN, 12)); }
        }
    }
    cv_blit(0, 24);
}

/* ORBIT PAD: the galaxy, the centre, the circle, the point going round it */
static void draw_pad(void)
{
    int32_t x0 = 4, y0 = 4, S = 136, k, cx = x0 + sys.ox * S / 99, cy = y0 + S - sys.oy * S / 99, rr = sys.orad * S / 99;
    char s[16];
    cv_begin(240, 144, C_BG);
    frame(x0 - 2, y0 - 2, S + 4, S + 4, C_FRAME);
    for (k = 0; k < 360; k++) {                 /* two arms of a log spiral */
        uint32_t ph = ((uint32_t)k << 25) + fm1_ms * 20000u;
        int32_t r = 4 + k / 6;
        uint16_t c = k & 2 ? C_VIO : C_FRAME;
        cv_pset(x0 + S / 2 + icos(ph, r), y0 + S / 2 + isin(ph, r), dim565(c, 6u + (uint32_t)(k & 7)));
        cv_pset(x0 + S / 2 - icos(ph, r), y0 + S / 2 - isin(ph, r), dim565(c, 6u + (uint32_t)(k & 7)));
    }
    cv_rect(x0, y0 + S / 2, S, 1, dim565(C_TXT, 4));
    cv_rect(x0 + S / 2, y0, 1, S, dim565(C_TXT, 4));
    circle(cx, cy, rr, C_LIME, 2);
    {
        int32_t px = x0 + eng.orb_x * S / 9900, py = y0 + S - eng.orb_y * S / 9900;
        cv_line(cx, cy, px, py, dim565(C_LIME, 8));
        disc(px, py, 6, dim565(C_LIME, 5));
        icon9(KIND_ICON[LK_VOICE], px - 4, py - 4, 1, C_LIME);
    }
    frame(146, 2, 92, 140, C_FDIM);
    put_u(put_s(s, "X "), sys.ox); tx(152, 8, &FONT_S, s, C_TXT, C_BG);
    put_u(put_s(s, "Y "), sys.oy); tx(152, 28, &FONT_S, s, C_TXT, C_BG);
    put_u(put_s(s, "RAD "), sys.orad); tx(152, 48, &FONT_S, s, C_TXT, C_BG);
    put_s(put_s(s, "T "), ORB_SYNC_NAME[sys.orate % ORB_NSYNC]); tx(152, 68, &FONT_S, s, C_TXT, C_BG);
    tx(152, 96, &FONT_S, "ORB X/Y", C_GREY, C_BG);
    tx(152, 114, &FONT_S, "-> MOD", C_GREY, C_BG);
    cv_blit(0, 24);
}

/* KEYS: the keyboard, each key's retuning (cents off equal temperament) or its harmonic, the sun's keys lit */
static void draw_keys_page(void)
{
    static const uint8_t WK[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
    uint32_t k, w = 0, lit = 0;
    for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)  /* (the keys sounding: a paw on each) */
        if (eng.v[k].on && eng.v[k].id >= 1u && eng.v[k].id <= 27u) lit |= 1u << (eng.v[k].id - 1u);
    int32_t keyoct = (int8_t)set.oct;
    cv_begin(240, 144, C_BG);
    for (k = 0; k < 27u; k++) {
        uint32_t note = 53u + k, white = 0, i;
        int32_t et, p = orb_key_pitch_o(&eng, sys.keymode == KM_HARMONIC ? k : note + 12u * (uint32_t)keyoct, keyoct, &et), dev = p - et;
        int32_t x, kw, ky, kh;
        uint16_t c;
        char s[8];
        for (i = 0; i < 16u; i++) if (WK[i] == k) { white = 1; w = i; }
        if (white) { x = 2 + (int32_t)w * 15; kw = 14; ky = 60; kh = 80; }
        else { x = 2 + (int32_t)w * 15 + 10; kw = 10; ky = 60; kh = 48; }
        c = (note % 12u) == sys.sun % 12u ? C_YEL : white ? C_TXT : C_GREY;
        if (!white) continue;
        cv_rect(x, ky, kw, kh, dim565(c, 12));
        if (lit >> k & 1u) spr2(ICON_PAW, 5, 5, x + kw / 2 - 5, ky + kh - 32, C_MAG);
        if (sys.keymode == KM_HARMONIC) {
            put_u(s, ORB_WHITE_H[k]);
            txc(x + kw / 2, ky + kh - 18, &FONT_S, s, C_INK, dim565(c, 12));
        } else if (sys.keymode == KM_CHORD) {   /* each white key the chord's note it plays now; its bass lime */
            put_s(s, NOTE_NAME[(p / 256) % 12]);
            s[1] = 0;
            if (chord_bass_key(note + 12u * (uint32_t)keyoct)) cv_rect(x + 2, ky + 2, kw - 4, 6, C_LIME);
            txc(x + kw / 2, ky + kh - 18, &FONT_S, s, C_INK, dim565(c, 12));
        } else if (dev) {
            int32_t h = dev / 3;                /* 1/256 semitone: ~1 px a cent */
            cv_rect(x + 3, 40 - (h > 0 ? h : 0), kw - 6, h > 0 ? h : -h, dev > 0 ? C_CYAN : C_MAG);
        }
    }
    w = 0;
    for (k = 0; k < 27u; k++) {                 /* the black keys over them */
        uint32_t note = 53u + k, i, white = 0;
        int32_t et, p = orb_key_pitch_o(&eng, sys.keymode == KM_HARMONIC ? k : note + 12u * (uint32_t)keyoct, keyoct, &et), dev = p - et, x;
        for (i = 0; i < 16u; i++) if (WK[i] == k) { white = 1; w = i; }
        if (white) continue;
        x = 2 + (int32_t)w * 15 + 10;
        cv_rect(x, 60, 10, 48, (note % 12u) == sys.sun % 12u ? C_YEL : C_FDIM);
        if (lit >> k & 1u) spr2(ICON_PAW, 5, 5, x, 94, C_TXT);
        if (sys.keymode == KM_HARMONIC) {
            char s[4];
            put_u(s, ORB_WHITE_H[k]);
            tx(x - 2, 86, &FONT_S, s, C_TXT, C_FDIM);
        } else if (sys.keymode == KM_CHORD) {
        } else if (dev) {
            int32_t h = dev / 3;
            cv_rect(x + 2, 40 - (h > 0 ? h : 0), 6, h > 0 ? h : -h, dev > 0 ? C_CYAN : C_MAG);
        }
    }
    cv_rect(2, 40, 236, 1, C_FDIM);
    tx(4, 2, &FONT_S, sys.keymode == KM_HARMONIC ? "HARMONICS OF THE SUN" : sys.keymode == KM_CHORD ?
       "THE CHORD'S NOTES: NONE WRONG" : "CENTS OFF EQUAL TEMPERED", C_GREY, C_BG);
    cv_blit(0, 24);
}

/* COSMOS: the tide between the sounding planets, the focused sound's chirp and beam */
static int32_t part_level(uint32_t p)          /* 0..32767: how loud a part sounds now */
{
    uint32_t v;
    int32_t m = 0;
    if (p) return eng.v[p - 1u].a;
    for (v = ORB_NV - ORB_NKV; v < ORB_NV; v++) if (eng.v[v].a > m) m = eng.v[v].a;
    return m;
}
static void draw_cosmos(void)
{
    const orb_snd_t *s = &sys.snd[focus];
    int32_t cx = 64, cy = 72, px[5], py[5], lv[5], k, q;
    cv_begin(240, 144, C_BG);
    stars(240, 144, 13, 26);
    frame(2, 2, 126, 140, C_FDIM);
    tx(8, 4, &FONT_S, "TIDE", C_CYAN, C_BG);
    for (k = 0; k < 5; k++) {                   /* the five sounds round the sun */
        uint32_t ph = 0xC0000000u + (uint32_t)k * 0x33333333u + fm1_ms * 60000u;
        px[k] = cx + icos(ph, 44);
        py[k] = cy + 6 + isin(ph, 44);
        lv[k] = part_level((uint32_t)k);
    }
    for (k = 0; k < 5; k++)                     /* a line where two sound and one is pulled */
        for (q = k + 1; q < 5; q++) {
            int32_t pull = (sys.macro[MAC_TIDE] + (eng.md[DST_TIDE] > 0 ? eng.md[DST_TIDE] / 331 : 0)) *
                           (sys.snd[k].tide + sys.snd[q].tide) / 2, b = lv[k] < lv[q] ? lv[k] : lv[q];
            int32_t t = (pull / 600) * (b > 4000 ? 15 : b / 267) / 15;
            if (t > 0) cv_line(px[k], py[k], px[q], py[q], dim565(mix565(RINGC[k], RINGC[q], 8), (uint32_t)orb_clamp(t + 3, 3, 15)));
        }
    cat_draw_small(cx, cy + 6, 96u);
    for (k = 0; k < 5; k++) {
        int32_t r = 3 + lv[k] / 4000;
        if (r > 5) disc(px[k], py[k], r, dim565(RINGC[k], 5));
        icon9(KIND_ICON[LK_VOICE], px[k] - 4, py[k] - 4, 1, RINGC[k]);
        if ((uint32_t)k == focus) circle(px[k], py[k], r + 3, C_TXT, 0);
    }
    /* the chirp: pitch against time, from the drop up onto the note, then the ring-down */
    frame(132, 2, 106, 76, C_FDIM);
    tx(138, 4, &FONT_S, "INSPIRAL", C_MAG, C_BG);
    {
        int32_t i, lx = 0, ly = 0, drop = s->insp * 48 / 99;
        for (i = 0; i <= 64; i++) {
            int32_t x = 138 + i * 70 / 64, y = 66 - (drop ? ((32767 - ORB_CHIRP[i]) * 40 >> 15) * drop / 48 + (48 - drop) * 40 / 48 : 40);
            if (i) cv_line(lx, ly, x, y, s->insp ? C_MAG : C_FDIM);
            lx = x; ly = y;
        }
        cv_rect(lx, ly, 24, 1, C_TXT);
        disc(lx, ly, 2, C_TXT);
    }
    /* the beam: pulses at the moon's ratio, narrower the deeper */
    frame(132, 82, 106, 60, C_FDIM);
    tx(138, 84, &FONT_S, "BEAM", C_YEL, C_BG);
    for (k = 0; k < 96; k++) {
        uint32_t ph = (uint32_t)k * 0x08000000u * (ORB_MRATIO[s->mratio % ORB_NMOON] >> 7) / 2u + fm1_ms * 900000u;
        int32_t q2 = (orb_cos(ph) + 32768) >> 1, g;
        q2 = (q2 * q2) >> 15; q2 = (q2 * q2) >> 15;
        g = 32767 - (((32767 - q2) * orb_lin(s->beam)) >> 15);
        cv_rect(136 + k, 134 - (g * 30 >> 15), 1, (g * 30 >> 15) + 1, dim565(C_YEL, 6u + (uint32_t)(g >> 12)));
    }
    cv_blit(0, 24);
}

/* HORIZON: the black hole, its ring reddening with the depth, grains falling in (backwards ones flying out) */
static void draw_horizon(void)
{
    int32_t cx = 86, cy = 72, d = eng.h_level, k;
    uint16_t ring = d < 33 ? mix565(C_YEL, C_MAG, (uint32_t)(d * 15 / 33)) : mix565(C_MAG, RGB(255, 40, 40), (uint32_t)orb_clamp((d - 33) * 15 / 66, 0, 15));
    char s[16];
    cv_begin(240, 144, C_BG);
    stars(240, 144, 21, 40);
    for (k = 0; k < 160; k++) {                 /* the accretion disc, behind */
        uint32_t ph = (uint32_t)k * 0x01999999u + fm1_ms * 300000u;
        int32_t x = cx + icos(ph, 72 - (k & 3) * 6), y = cy + isin(ph, 14 - (k & 3));
        if (isin(ph, 14) < 0) cv_pset(x, y, dim565(k & 1 ? C_LIME : C_YEL, 9));
    }
    disc(cx, cy, 30, dim565(ring, 5));
    {   /* its ears: the hole is cat-shaped */
        int32_t k, e;
        for (e = -1; e <= 1; e += 2)
            for (k = 0; k < 13; k++) {
                int32_t tx0 = cx + e * 17 - k / 2, ty = cy - 38 + k;
                cv_rect(tx0, ty, k / 2 * 2 + 1, 1, C_INK);
                cv_pset(tx0, ty, ring);
                cv_pset(tx0 + k / 2 * 2, ty, ring);
            }
    }
    disc(cx, cy, 26, C_INK);
    circle(cx, cy, 27, ring, 0);
    circle(cx, cy, 28, dim565(ring, 10), 0);
    for (k = 0; k < 160; k++) {                 /* the disc in front */
        uint32_t ph = (uint32_t)k * 0x01999999u + fm1_ms * 300000u;
        int32_t x = cx + icos(ph, 72 - (k & 3) * 6), y = cy + isin(ph, 14 - (k & 3));
        if (isin(ph, 14) >= 0) cv_pset(x, y, k & 1 ? C_LIME : C_YEL);
    }
    for (k = 0; k < d / 3; k++) {               /* the grains: spiralling in, RETRO's share out */
        uint32_t h = (uint32_t)k * 2654435761u, life = (fm1_ms / 4u + (h >> 20)) % 256u;
        int retro = (h >> 8) % 100u < sys.hretro;
        int32_t r = retro ? 28 + (int32_t)life / 4 : 92 - (int32_t)life / 4;
        uint32_t ph = (h & 0xFFFFFFu) << 8;
        ph += (uint32_t)(retro ? -1 : 1) * life * 0x00C00000u;
        if (r > 27) cv_pset(cx + icos(ph, r), cy + isin(ph, r / 2), retro ? C_MAG : C_CYAN);
    }
    if (!d) txc(cx, cy - 8, &FONT_S, "OPEN", C_DIMT, C_INK);
    /* the redshift meter */
    frame(176, 2, 62, 140, C_FDIM);
    tx(182, 4, &FONT_S, "SHIFT", C_GREY, C_BG);
    for (k = 0; k < 100; k++) {
        int32_t st = 24 - k * 48 / 99;
        uint16_t c = st >= 0 ? mix565(C_TXT, C_CYAN, (uint32_t)(st * 15 / 24)) : mix565(C_TXT, RGB(255, 40, 40), (uint32_t)(-st * 15 / 24));
        cv_rect(196, 24 + k, 14, 1, dim565(c, 9));
    }
    {
        int32_t now = sys.hshift * d / 99, y = 24 + (24 - now) * 99 / 48;
        cv_rect(190, y - 1, 26, 3, C_TXT);
        put_s(put_i(s, now, 1), "st");
        txc(207, 126, &FONT_S, s, C_TXT, C_BG);
    }
    put_s(put_u(s, (uint32_t)d), "%");
    tx(8, 4, &FONT_S, s, ring, C_BG);
    cv_blit(0, 24);
}

static void draw_cute(void);
static void draw_main(void)
{
    switch (page) {
    case PG_CUTE: draw_cute(); break;
    case PG_ORRERY: draw_orrery(); break;
    case PG_MACRO: draw_macros(); break;
    case PG_RING: draw_ring(); break;
    case PG_TIME: draw_time(); break;
    case PG_SOUND: draw_sound(); break;
    case PG_ORBIT: case PG_BODY: draw_orbit(); break;
    case PG_COSMOS: draw_cosmos(); break;
    case PG_HORIZON: draw_horizon(); break;
    case PG_ENV: draw_env(); break;
    case PG_MOD: draw_mod(); break;
    case PG_LFO: draw_lfo(); break;
    case PG_FILTER: draw_filter(); break;
    case PG_SPACE: draw_space(); break;
    case PG_PAD: draw_pad(); break;
    case PG_BG: draw_bg(); break;
    default: draw_keys_page(); break;
    }
}
static int page_animated(void)
{
    return page == PG_ORRERY || page == PG_MACRO || page == PG_RING || page == PG_TIME || page == PG_SOUND || page == PG_ORBIT ||
           page == PG_BODY || page == PG_MOD || page == PG_LFO || page == PG_PAD || page == PG_COSMOS ||
           page == PG_HORIZON || page == PG_BG || page == PG_CUTE;
}

/* the splash: the cat-planet (tools/gen_splash.py, 3x), stars twinkling round it as in the drawing, planets going
 * round it (behind it on the far side), a glint running along its ring, a blink */
static void plus_star(int32_t x, int32_t y, uint32_t t, uint32_t seed)
{
    uint32_t b = (t / 90u + seed) % 12u, arm = b < 2u ? 6u : b < 6u ? 4u : 3u;
    uint16_t c = b < 2u ? C_TXT : RGB(176, 77, 255);
    cv_rect(x - (int32_t)arm, y - 1, 2 * (int32_t)arm + 1, 3, dim565(c, b < 6u ? 15u : 10u));
    cv_rect(x - 1, y - (int32_t)arm, 3, 2 * (int32_t)arm + 1, dim565(c, b < 6u ? 15u : 10u));
    cv_rect(x - 1, y - 1, 3, 3, C_TXT);
}
static void cat_draw(int32_t ox, int32_t oy, uint32_t t)
{
    uint32_t i, j, blink = (t > 950u && t < 1080u) || (t > 1180u && t < 1260u);
    int32_t glint = (int32_t)((t * 60u / 1000u) % (CAT_W + 16u)) - 8;   /* the cell the glint is on */
    for (j = 0; j < CAT_H; j++)
        for (i = 0; i < CAT_W; i++) {
            uint32_t k = CAT[j][i];
            uint16_t c;
            if (!k) continue;
            c = CAT_PAL[k];
            if (k == CAT_EYE && blink) c = j + 1u < CAT_H && CAT[j + 1u][i] == CAT_EYE ? CAT_PAL[CAT_FACE] : CAT_PAL[CAT_WHITE];
            if (k == CAT_YELLOW && j > CAT_H / 2u) {                    /* the ring's front: the glint */
                int32_t d = (int32_t)i - glint;
                if (d >= -1 && d <= 1) c = mix565(c, C_TXT, d ? 8u : 13u);
            }
            cv_rect(ox + (int32_t)i * 3, oy + (int32_t)j * 3, 3, 3, c);
        }
}
static struct { uint32_t t0, last, tail; uint8_t dfb, dmix, rmix; } spl;
static void splash_begin(int tail)              /* tail: after the story, its echoes and its NEBULA die out meanwhile */
{
    uint32_t rel;
    spl.t0 = fm1_ms;
    spl.last = fm1_ms - 1000u;
    spl.tail = (uint32_t)tail;
    spl.dfb = sys.dfb; spl.dmix = sys.dmix; spl.rmix = sys.rmix;
    lcd_fill(0, 0, 240, 240, C_BG);
    fm1_input_edges(&rel);
}
static int splash_pass(void)                    /* a pass (a millisecond on the host and the page); 0: it is over */
{
    static const int16_t PS[6][2] = {{24, 26}, {214, 44}, {18, 112}, {222, 122}, {60, 8}, {184, 136}};
    static const int16_t DOT[6][2] = {{200, 14}, {12, 66}, {70, 136}, {166, 10}, {230, 90}, {40, 100}};
    static const uint16_t PC[3] = {C_CYAN, C_MAG, C_LIME};
    int32_t ox = (240 - (int32_t)CAT_W * 3) / 2, oy = 30;
    uint32_t t, k, pass, rel;
    platform_poll();
    t = fm1_ms - spl.t0;
    if (t >= 1900u) return 0;
    if (fm1_input_edges(&rel) & (1u << BTN_PLAY)) return 0;   /* PLAY: straight in */
    if (spl.tail) {                             /* the feedback first, then the returns, gone by the end */
        sys.dfb = (uint8_t)(spl.dfb * (1900u - t) / 1900u);
        if (t > 1100u) { sys.dmix = (uint8_t)(spl.dmix * (1900u - t) / 800u); sys.rmix = (uint8_t)(spl.rmix * (1900u - t) / 800u); }
    }
    if (fm1_ms - spl.last < 30u) return 1;     /* ~30 frames a second */
    spl.last = fm1_ms;
    {
        cv_begin(240, 144, C_BG);
        stars(240, 144, 1, 50);
        for (k = 0; k < 6u; k++) cv_rect(DOT[k][0], DOT[k][1], 3, 3, ((t / 140u + k) % 5u) ? C_TXT : dim565(C_TXT, 6));
        for (k = 0; k < 6u; k++) plus_star(PS[k][0], PS[k][1], t, k * 5u);
        for (pass = 0; pass < 2u; pass++) {    /* planets: behind the cat, then the cat, then in front */
            if (pass == 1u) cat_draw(ox, oy, t);
            for (k = 0; k < 3u; k++) {
                uint32_t ph = t * (2600000u + k * 900000u) + k * 0x55555555u;
                int32_t x = 120 + icos(ph, 112 - (int32_t)k * 14), y = oy + 50 + isin(ph, 22 + (int32_t)k * 6) - icos(ph, 8);
                int front = isin(ph, 22) > 0;
                if ((uint32_t)front != pass) continue;
                disc(x, y, 4 - (int32_t)k, PC[k]);
                cv_pset(x - 1, y - 1, mix565(PC[k], C_TXT, 10));
            }
        }
        if (t > 300u && t < 900u) {             /* a shooting star */
            int32_t sx = 236 - (int32_t)(t - 300u) * 2 / 5, sy = 6 + (int32_t)(t - 300u) / 12, q;
            for (q = 0; q < 14; q++) cv_pset(sx + q, sy - q / 3, dim565(C_TXT, (uint32_t)(15 - q)));
        }
        cv_blit(0, 0);
        cv_begin(240, 96, C_BG);
        stars(240, 96, 2, 22);
        txc(120, 4, &FONT_L, "ORBITRIX", C_LIME, C_BG);
        cv_rect(64, 38, 112, 2, C_MAG);
        txc(120, 48, &FONT_S, "AN ORBITAL GROOVE ENGINE", C_GREY, C_BG);
        txc(120, 68, &FONT_S, "v" APP_VER, C_DIMT, C_BG);
        cv_blit(0, 144);
    }
    return 1;
}

/* ---- Astralica drawn at any size (HOME, the splash, the legend) */
static void cat_draw_q8(int32_t ax, int32_t ay, uint32_t s, uint32_t acx, uint32_t acy, int blink, uint32_t look)
{   /* Astralica at s / 256 pixels a cell, the cell (acx, acy) / 2 at screen (ax, ay); look: 1 her eyes glow */
    uint32_t i, j;
    int32_t ox = ax - (int32_t)((acx * s) >> 9), oy = ay - (int32_t)((acy * s) >> 9);
    for (j = 0; j < CAT_H; j++) {
        int32_t y0 = oy + (int32_t)((j * s) >> 8), y1 = oy + (int32_t)(((j + 1u) * s) >> 8);
        if (y1 <= y0 || y1 < -cv_oy || y0 >= (int32_t)cv_h - cv_oy) continue;
        for (i = 0; i < CAT_W; i++) {
            uint32_t k = CAT[j][i];
            int32_t x0 = ox + (int32_t)((i * s) >> 8), x1 = ox + (int32_t)(((i + 1u) * s) >> 8);
            uint16_t c;
            if (!k || x1 <= x0 || x1 < 0 || x0 >= 240) continue;
            c = CAT_PAL[k];
            if (k == CAT_EYE) {
                if (blink == 2) c = j && CAT[j - 1u][i] == CAT_EYE ? CAT_PAL[CAT_FACE] : CAT_PAL[CAT_WHITE];   /* happy: ^ ^ */
                else if (blink) c = j + 1u < CAT_H && CAT[j + 1u][i] == CAT_EYE ? CAT_PAL[CAT_FACE] : CAT_PAL[CAT_WHITE];
                else if (look) c = ((fm1_ms >> 6) & 1u) ? C_LIME : C_CYAN;
            }
            cv_rect(x0, y0, x1 - x0, y1 - y0, c);
        }
    }
}
static void cat_draw_small(int32_t cx, int32_t cy, uint32_t s)   /* under a pixel a cell: each pixel takes its cell */
{
    int32_t w = (int32_t)((CAT_W * s) >> 8), h = (int32_t)((CAT_H * s) >> 8), x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            uint32_t k = CAT[(uint32_t)y * 256u / s][(uint32_t)x * 256u / s];
            if (k) cv_pset(cx - w / 2 + x, cy - h / 2 + y, CAT_PAL[k]);
        }
}
/* CUTE: Astralica in the mood (sleepy: her eyes shut, z's rising; playful: happy eyes, hearts; sparkly: her eyes
 * aglow, stars about her; zoomies: her kittens racing round), the mood's name, and how a new one is made */
static void draw_cute(void)
{
    static const uint16_t MC[ORB_NMOOD] = {C_VIO, C_MAG, C_CYAN, C_YEL};
    uint32_t m = sys.mood % ORB_NMOOD, k, t = fm1_ms;
    int32_t cx = 120, cy = 54, bob = (int32_t)((t / 240u) & 1u);
    cv_begin(240, 144, C_BG);
    stars(240, 144, 77, 26);
    for (k = 0; k < 4u; k++) {                  /* (the four kittens on a ring about her: slow, or racing) */
        uint32_t ph = (uint32_t)k * 0x40000000u + t * (m == ORB_MOOD_ZOOMIES ? 2400000u : 300000u);
        int32_t x = cx + icos(ph, 92), y = cy + isin(ph, 30);
        static const uint16_t RC[4] = {C_CYAN, C_MAG, C_YEL, C_VIO};
        spr2(SPR_KITTY, 7, 6, x - 7, y - 6, RC[k]);
        if (m == ORB_MOOD_ZOOMIES) {            /* (their speed lines) */
            int32_t j;
            for (j = 1; j < 4; j++) cv_pset(cx + icos(ph - (uint32_t)j * 0x05000000u, 92), cy + isin(ph - (uint32_t)j * 0x05000000u, 30), dim565(RC[k], (uint32_t)(12 - 3 * j)));
        }
    }
    cat_draw_q8(cx, cy + bob, 512u, CAT_W, CAT_H, m == ORB_MOOD_SLEEPY ? 1 : m == ORB_MOOD_PLAYFUL ? 2 : 0,
                m == ORB_MOOD_SPARKLY);
    if (m == ORB_MOOD_SLEEPY)                   /* z z Z, rising and fading */
        for (k = 0; k < 3u; k++) {
            uint32_t q = (t / 40u + k * 30u) % 90u;
            tx(cx + 40 + (int32_t)k * 9, cy - 10 - (int32_t)q / 2, &FONT_S, k == 2u ? "Z" : "z", dim565(C_VIO, 15u - q / 7u), C_BG);
        }
    if (m == ORB_MOOD_PLAYFUL)                  /* hearts bouncing up about her */
        for (k = 0; k < 5u; k++) {
            uint32_t q = (t / 30u + k * 23u) % 60u;
            spr2(SPR_HEART, 5, 5, cx - 70 + (int32_t)k * 34, cy + 20 - (int32_t)q, dim565(C_MAG, 15u - q / 5u));
        }
    if (m == ORB_MOOD_SPARKLY)                  /* stars twinkling round her */
        for (k = 0; k < 8u; k++) {
            uint32_t ph = (uint32_t)k * 0x20000000u + 0x10000000u;
            if (((t / 160u) + k) % 3u) plus_star(cx + icos(ph, 62), cy + isin(ph, 40), t, k);
        }
    txc(120, 94, &FONT_L, ORB_MOOD_NAME[m], MC[m], C_BG);
    txc(120, 127, &FONT_S, "SEL+HOME: A NEW ONE", C_GREY, C_BG);
    cv_blit(0, 24);
}

#ifdef ORB_LEGEND
/* ---- the legend of Astralica, for the page (tools/make_legend.py films it on the host: a transparent animation and its
 * sound), scored by the engine itself (orb_intro_system). The screen is drawn whole each frame, as two canvases */
static uint32_t typed(const char *s, uint32_t t0, uint32_t t, char *out)   /* typewriter: a letter each 34 ms */
{
    uint32_t n = t < t0 ? 0u : (t - t0) / 34u, k;
    for (k = 0; s[k] && k < n && k < 30u; k++) out[k] = s[k];
    out[k] = 0;
    return k;
}
#define I_T2 2600u                              /* then came Astralica */
#define I_T3 4800u                              /* she grew (and ate the planets) */
#define I_T4 8200u                              /* in her tummy they changed */
#define I_T45 10200u                            /* she pooped them out: they formed the ORBITRIX */
#define I_T5 13600u                             /* the multiverses */
#define I_END 17200u
static const uint16_t I_EAT[4] = {5200, 5900, 6600, 7300};
static const uint16_t I_PLOP[4] = {10500, 10900, 11300, 11700};   /* each comes out; at its ring I_FLY later */
#define I_FLY 800u
static uint32_t i_grow(uint32_t u)              /* her size as she grows: x2 each 1.1 s from 1 (Q8) */
{
    uint32_t oct = u / 1100u, fr = (u % 1100u) * 256u / 1100u;
    return (256u << oct) + ((256u << oct) * fr >> 8);
}
static void i_cat(uint32_t t, uint32_t *s, uint32_t *acy)   /* how big she is and where her anchor is, without a jump */
{
    *acy = 36u;                                 /* (the anchor in half cells: 36 her middle, 30 her eyes) */
    if (t < I_T2) *s = 0;
    else if (t < I_T3) *s = 64u + (t - I_T2) * 192u / (I_T3 - I_T2);
    else if (t < I_T4) {
        uint32_t u = t - I_T3;
        *s = i_grow(u);
        *acy = u < 800u ? 36u - u * 6u / 800u : 30u;
    } else if (t < I_T4 + 700u) {               /* back out, eased, from exactly where the zoom ended */
        uint32_t u = t - I_T4, e = 700u - u, from = i_grow(I_T4 - I_T3);
        e = e * e / 700u;                       /* (700 .. 0, slowing at the end) */
        *s = 384u + (from - 384u) * e / 700u;
        *acy = 36u - 6u * e / 700u;
    } else if (t < I_T5) *s = 384u;
    else *s = 384u - (uint32_t)orb_clamp((int32_t)(t - I_T5) / 30, 0, 80);
}
static int i_planet(uint32_t t, int32_t k, int32_t *x, int32_t *y, int *front)   /* 0: gone (eaten) */
{
    uint32_t ph = t * (1500000u + (uint32_t)k * 700000u) + (uint32_t)k * 0x40000000u;
    int32_t r = 40 + k * 20;
    if (t >= I_T3) {
        if (t >= I_EAT[k]) return 0;
        r = r * (int32_t)(I_EAT[k] - t) / (int32_t)(I_EAT[k] - I_T3);
    }
    *x = 120 + icos(ph, r);
    *y = 92 + isin(ph, r / 3 + 2);
    *front = isin(ph, 1000) >= 0;
    return 1;
}
static void i_poop(int32_t x, int32_t y, uint16_t c)   /* a little cosmic swirl, as it comes out, its eyes open */
{
    uint16_t d = mix565(c, C_INK, 6), hi = mix565(c, C_TXT, 7);
    pill(x - 8, y + 3, 17, 5, c);
    pill(x - 6, y - 1, 13, 5, c);
    pill(x - 4, y - 5, 9, 5, c);
    cv_rect(x - 1, y - 7, 3, 2, c); cv_rect(x + 1, y - 9, 2, 2, c); cv_pset(x + 3, y - 9, c);   /* its tip, curled */
    cv_rect(x - 6, y + 3, 13, 1, d);
    cv_rect(x - 4, y - 1, 9, 1, d);
    cv_rect(x - 5, y + 4, 3, 1, hi); cv_rect(x - 3, y, 2, 1, hi); cv_rect(x - 2, y - 4, 2, 1, hi);   /* its shine */
    cv_rect(x - 4, y + 1, 3, 3, C_TXT); cv_rect(x + 2, y + 1, 3, 3, C_TXT);   /* eyes */
    cv_rect(x - 3, y + 2, 2, 2, C_INK); cv_rect(x + 3, y + 2, 2, 2, C_INK);
    cv_pset(x - 6, y + 5, mix565(C_MAG, c, 6)); cv_pset(x + 6, y + 5, mix565(C_MAG, c, 6));   /* blush */
}
static void i_ring_at(uint32_t t, uint32_t k, int32_t grow, int32_t *x, int32_t *y)   /* ring k's first step, now */
{
    uint32_t ph = t * 200000u * (k + 1u);
    *x = 120 + icos(ph, (60 + (int32_t)k * 16) * grow / 100);
    *y = 96 + isin(ph, (16 + (int32_t)k * 6) * grow / 100);
}
static void i_kitten(uint32_t t, uint32_t k, int32_t *x, int32_t *y)   /* a kitten out of her: flying to its ring, on it */
{
    uint32_t age = t - I_PLOP[k];
    int32_t tx0 = 120, ty0 = 92 + 34 + 4, rx, ry;
    i_ring_at(t, k, 100, &rx, &ry);
    if (age < 300u) { *x = tx0; *y = ty0 - 4 + (int32_t)age / 30; return; }   /* (a swirl, still) */
    if (age < I_FLY) {
        int32_t u = (int32_t)(age - 300u) * 256 / (int32_t)(I_FLY - 300u);
        u = u * (512 - u) / 256;                /* eased out */
        *x = tx0 + (rx - tx0) * u / 256;
        *y = ty0 + 6 + (ry - ty0 - 6) * u / 256 - (u * (256 - u) >> 11);   /* (a little hop) */
        return;
    }
    *x = rx; *y = ry;
}
static void intro_scene(uint32_t t)
{
    static const uint16_t PC[4] = {C_CYAN, C_MAG, C_YEL, C_VIO};
    static const int16_t BUB[6][2] = {{36, 34}, {204, 30}, {22, 120}, {218, 112}, {78, 150}, {164, 152}};
    int32_t cx = 120, cy = 92, k, px, py;
    uint32_t s, acy, pass;
    int front;
    char line[2][32];
    const char *l1 = "", *l2 = "";
    uint32_t t1 = 0, t2 = 0;
    i_cat(t, &s, &acy);
    /* the stars stay; as she grows, streaks stream out over them, and fade as she comes back */
    stars(240, 240, 99, 80);
    if (t >= I_T3 + 300u && t < I_T4 + 400u) {
        uint32_t q, sd = 4242u, sp = (t - I_T3) / 8u, fade = t < I_T3 + 900u ? (t - I_T3 - 300u) / 40u : t >= I_T4 ? 10u - (t - I_T4) / 40u : 15u;
        for (q = 0; q < 70u; q++) {
            uint32_t ph = orb_xs(&sd), r0 = (orb_xs(&sd) >> 24) + sp * (2u + (q & 3u));
            int32_t r = (int32_t)(r0 % 170u) + 4, len = 2 + (int32_t)sp / 40;
            uint32_t b = (6u + (q & 7u)) * (fade > 15u ? 15u : fade) / 15u;
            if (b) cv_line(cx + icos(ph, r), cy + isin(ph, r), cx + icos(ph, r + len), cy + isin(ph, r + len), dim565(C_TXT, b));
        }
    }
    /* the dim star she comes from */
    if (t < I_T2 + 400u) disc(cx, cy, 3 + (t > I_T2 ? (int32_t)(t - I_T2) / 40 : 0), t > I_T2 ? C_TXT : dim565(C_YEL, 9));
    /* the ORBITRIX: each ring grows from its kitten as it arrives, its steps lit as the orrery plays */
    if (t >= I_PLOP[0] + I_FLY && t < I_T5 + 1200u) {
        uint32_t r;
        for (r = 0; r < 4u; r++) {
            int32_t grow = t < I_PLOP[r] + I_FLY ? 0 : orb_clamp((int32_t)(t - I_PLOP[r] - I_FLY) / 4, 0, 100);
            int32_t rx = (60 + (int32_t)r * 16) * grow / 100, ryy = (16 + (int32_t)r * 6) * grow / 100;
            uint32_t q, len = sys.ring[r].len ? sys.ring[r].len : 1u;
            uint16_t c = RINGC[r + 1u];
            if (!grow) continue;
            if (t > I_T5) c = dim565(c, (uint32_t)orb_clamp(15 - (int32_t)(t - I_T5) / 80, 0, 15));
            for (q = 0; q < len; q++) {
                uint32_t ph = (uint32_t)(((uint64_t)q << 32) / len) + t * 200000u * (r + 1u);
                int lit = orb_step_on(&eng, r, q, 0) && eng.playing && eng.rs[r].cur == q;
                cv_rect(cx + icos(ph, rx) - 1, cy + 4 + isin(ph, ryy) - 1, lit ? 4 : 2, lit ? 4 : 2, lit ? C_TXT : c);
            }
        }
    }
    /* the multiverses: waves going out at each echo, and a universe a bubble each, a cat in it */
    if (t >= I_T5 + 200u) {
        uint32_t q, e = (t - I_T5 - 200u);
        for (q = 0; q < 6u; q++) {
            uint32_t at = 400u + q * 600u;      /* the delay's quarters at 100 bpm */
            int32_t age, rr;
            if (e < at) continue;
            age = (int32_t)(e - at);
            rr = 20 + age / 5;
            if (rr < 160) circle(cx, cy, rr, dim565(RINGC[1u + q % 4u], (uint32_t)orb_clamp(15 - age / 60, 2, 15)), 2);
            circle(BUB[q][0], BUB[q][1], 22, dim565(C_VIO, 9), 0);
            circle(BUB[q][0], BUB[q][1], 23, dim565(C_FRAME, 6), 3);
            cat_draw_small(BUB[q][0], BUB[q][1] + (age < 200 ? (200 - age) / 50 : 0), 128u);
        }
    }
    /* the planets behind her, her, the planets in front and the flash of one eaten */
    for (pass = 0; pass < 2u; pass++) {
        int32_t wob = t >= I_T4 + 700u && t < I_T45 ? (((t / 70u) & 1u) ? 1 : -1) : 0, push = 0;
        int blink = (t > I_T2 + 1500u && t < I_T2 + 1620u) || (t > I_T5 + 2400u && t < I_T5 + 2520u);
        for (k = 0; k < 4; k++) if (t >= I_PLOP[k] && t < I_PLOP[k] + 140u) push = 1;
        if (t >= I_T4 + 700u && t < I_T5) blink = 2;   /* (her tummy at work, then the plops: content) */
        if (pass == 1u && s) cat_draw_q8(cx + wob, cy + push, s, 48u, acy, blink, t >= I_T45 + 2200u && t < I_T5);
        if (pass == 1u && t >= I_T4 + 500u && t < I_T45 + 100u) {   /* her tummy: a glow, the planets turning in it */
            int32_t gx = cx + wob, gy = cy + 13, dx, dy, gr = 10, q;
            uint32_t u = t - I_T4 - 500u;
            if (u < 300u) gr = (int32_t)u / 30;
            for (dy = -gr; dy <= gr; dy++)
                for (dx = -gr; dx <= gr; dx++)
                    if (dx * dx + dy * dy <= gr * gr && ((dx + dy + (int32_t)(t / 60u)) & 1))
                        cv_pset(gx + dx, gy + dy, RINGC[1u + (uint32_t)((dx * dx + dy * dy) / 12 + (int32_t)(t / 90u)) % 4u]);
            for (q = 0; q < 4; q++) {
                uint32_t ph = t * 9000000u + (uint32_t)q * 0x40000000u;
                int32_t x = gx + icos(ph, 6), y = gy + isin(ph, 4);
                if (u > 1200u) spr(SPR_SPARK, 5, 5, x - 2, y - 2, ((t >> 6) + (uint32_t)q) & 1u ? C_TXT : PC[q]);
                else disc(x, y, 1, PC[q]);
            }
            for (q = -1; q <= 1; q += 2) {        /* rumble */
                int32_t z;
                for (z = 0; z < 6; z++) cv_pset(gx + q * (44 + z), gy - 2 + isin((uint32_t)z * 0x30000000u + t * 6000000u, 2), dim565(C_TXT, 9));
            }
        }
        if (t >= I_T45) {                         /* out they come: a swirl, plop!, a kitten flying to its ring */
            for (k = 0; k < 4; k++) {
                uint32_t age = t - I_PLOP[k];
                int32_t x, y;
                if (t < I_PLOP[k] || t >= I_T5 + 1200u) continue;
                i_kitten(t, (uint32_t)k, &x, &y);
                if (age < 300u) {
                    if (pass) i_poop(x, y, PC[k]);
                } else if ((pass == 1u) == (y >= cy + 4 || age < I_FLY)) {
                    uint16_t c = t > I_T5 ? dim565(PC[k], (uint32_t)orb_clamp(15 - (int32_t)(t - I_T5) / 80, 0, 15)) : PC[k];
                    if (age < 420u) disc(x, y, 6 - (int32_t)(age - 300u) / 24, dim565(C_TXT, 8));   /* (it turns) */
                    kitty(x, y, c);
                }
                if (pass && age < 450u) {         /* plop! and a puff of stardust */
                    uint32_t q;
                    tx(cx + 12, cy + 26 - (int32_t)age / 30, &FONT_S, "plop!", dim565(C_TXT, (uint32_t)orb_clamp(15 - (int32_t)age / 30, 2, 15)), C_BG);
                    for (q = 0; q < 4u; q++) {
                        uint32_t ph = 0x20000000u + q * 0x40000000u;
                        int32_t rr = 6 + (int32_t)age / 25;
                        spr(SPR_SPARK, 5, 5, cx + icos(ph, rr) - 2, cy + 36 + isin(ph, rr / 2) - 2, dim565(PC[k], (uint32_t)orb_clamp(15 - (int32_t)age / 30, 2, 15)));
                    }
                }
            }
        }
        for (k = 0; k < 4; k++) {
            if (!i_planet(t, k, &px, &py, &front)) {
                if (pass == 1u && t < I_EAT[k] + 160u) {   /* eaten: a flash at her heart */
                    int32_t fr = 18 - (int32_t)(t - I_EAT[k]) / 9;
                    disc(cx, cy + 8, fr, mix565(PC[k], C_TXT, 8));
                    disc(cx, cy + 8, fr / 2, C_TXT);
                }
                continue;
            }
            if (s && t >= I_T2 && (uint32_t)front != pass) continue;   /* (no her: all in the first pass) */
            if ((!s || t < I_T2) && pass) continue;
            disc(px, py, 4 - k / 3, PC[k]);
        }
    }
    /* the words */
    if (t < I_T2) { l1 = "IN THE BEGINNING"; t1 = 200; l2 = "THE UNIVERSE WAS SILENT"; t2 = 1100; }
    else if (t < I_T3) { l1 = "THEN CAME"; t1 = I_T2 + 100u; l2 = ""; }
    else if (t < I_T3 + 2100u) { l1 = "SHE GREW."; t1 = I_T3; l2 = "AND GREW."; t2 = I_T3 + 1000u; }
    else if (t < I_T4) { l1 = "ONE BIG ASS CAT."; t1 = I_T3 + 2100u; }
    else if (t < I_T45) { l1 = "SHE ATE ALL THE PLANETS"; t1 = I_T4 + 100u; l2 = "IN HER TUMMY THEY CHANGED"; t2 = I_T4 + 1000u; }
    else if (t < I_T5) { l1 = "AND WHEN SHE POOPED THEM OUT"; t1 = I_T45 + 100u; l2 = "THEY FORMED THE ORBITRIX"; t2 = I_PLOP[2]; }
    else { l1 = "NOW HER SONG RINGS"; t1 = I_T5 + 100u; l2 = "THROUGH THE MULTIVERSES"; t2 = I_T5 + 900u; }
    cv_rect(0, 180, 240, 60, C_BG);
    typed(l1, t1, t, line[0]);
    typed(l2, t2, t, line[1]);
    if (t >= I_T2 && t < I_T3) {                /* her name, big, letter by letter */
        char nm[12];
        typed("ASTRALICA", I_T2 + 500u, t, nm);
        txc(120, 186, &FONT_S, line[0], C_GREY, C_BG);
        txc(120, 204, &FONT_L, nm, C_LIME, C_BG);
    } else {
        txc(120, 190, &FONT_S, line[0], C_TXT, C_BG);
        txc(120, 210, &FONT_S, line[1], t >= I_T45 ? C_LIME : C_GREY, C_BG);
    }
}
static struct { uint32_t t0, cue, last; } itr;
static void intro_begin(void)
{
    uint32_t rel;
    int32_t sun = 36;                            /* C2 */
    itr.t0 = fm1_ms; itr.cue = 0; itr.last = fm1_ms - 1000u;
    fm1_input_edges(&rel);
    lcd_fill(0, 0, 240, 240, C_BG);
    orb_note_on(&eng, 1, sun * 256, 104);        /* the silent universe: a drone, C2 and G2 */
    orb_note_on(&eng, 2, (sun + 7) * 256 + 5, 92);
}
static void intro_end(void)
{
    orb_play(&eng, 0);
    orb_all_off(&eng);
    sys.macro[MAC_HORIZON] = 0;                 /* (its mix glides out: no click) */
}
static int intro_pass(void)                     /* a pass; 0: over */
{
    uint32_t rel, cue = itr.cue, t, h;
    int32_t sun = 36;
    platform_poll();
    t = fm1_ms - itr.t0;
    if (t >= I_END) return 0;
    fm1_input_edges(&rel);
    {
        /* the score */
        if (cue == 0 && t >= I_T2) { orb_part_on(&eng, ORB_A, (sun - 12) * 256 + orb_degree(&sys, 0), 127, 120); cue++; }
        if (cue == 1 && t >= I_T2 + 1100u) { orb_part_on(&eng, ORB_A, (sun - 12) * 256 + orb_degree(&sys, 4), 120, 110); cue++; }
        if (cue >= 2 && cue < 6 && t >= I_EAT[cue - 2u] - 700u) {   /* a planet eaten: its chirp lands as it goes in */
            static const int8_t DG[4] = {0, 4, 8, 6};
            orb_part_on(&eng, ORB_A, (sun - 12) * 256 + orb_degree(&sys, DG[cue - 2u]), 127, 100);
            cue++;
        }
        if (t >= 6000u && t < I_T4) sys.macro[MAC_HORIZON] = (uint8_t)orb_clamp((int32_t)(t - 6000u) * 62 / 2200, 0, 62);
        if (cue == 6 && t >= I_T4) { sys.macro[MAC_HORIZON] = 0; cue++; }
        if (cue == 7 && t >= I_T4 + 500u) {     /* her tummy gurgles: bubbles on A (its chirp long gone) */
            sys.snd[1] = SND_BLOOP; sys.snd[1].oct = 1; sys.snd[1].level = 84;
            cue++;
        }
        if (cue >= 8 && cue < 12) {
            static const uint16_t GT[4] = {600, 950, 1200, 1600};
            static const int8_t GD[4] = {0, 2, 1, 4};
            if (t >= I_T4 + GT[cue - 8u]) { orb_part_on(&eng, ORB_A, (sun - 12) * 256 + orb_degree(&sys, GD[cue - 8u]), 100, 14); cue++; }
        }
        if (cue == 12 && t >= I_T45) {          /* out they come: plops on A; B C D quiet till their kittens are there */
            uint32_t r;
            sys.snd[1] = SND_BOOP; sys.snd[1].oct = 1; sys.snd[1].level = 90;
            for (r = 1; r < ORB_NRING; r++) sys.ring[r].mute = 1;
            cue++;
        }
        if (cue >= 13 && cue < 17 && t >= I_PLOP[cue - 13u]) {   /* plop: each its ring's degree, 4:5:6:7 */
            orb_part_on(&eng, ORB_A, sun * 256 + orb_degree(&sys, sys.ring[cue - 13u].deg), 112, 18);
            cue++;
        }
        if (cue == 17 && t >= I_PLOP[0] + I_FLY) { orb_play(&eng, 1); cue++; }   /* the first is on its ring: the ORBITRIX */
        if (cue >= 18 && cue < 21 && t >= I_PLOP[cue - 17u] + I_FLY) { sys.ring[cue - 17u].mute = 0; cue++; }   /* B, C, D sing */
        if (cue == 21 && t >= I_T45 + 600u) { orb_note_off(&eng, 1); orb_note_off(&eng, 2); cue++; }   /* (gone before the meow) */
        if (cue == 22 && t >= I_T5) { orb_play(&eng, 0); cue++; }
        if (cue == 23 && t >= I_T5 + 120u) { sys.snd[0] = SND_MEOW; cue++; }
        if (cue == 24 && t >= I_T5 + 200u) { orb_note_on(&eng, 3, 79 * 256, 110); cue++; }   /* meow: mi- */
        if (cue == 25 && t >= I_T5 + 270u) { orb_glide(&eng, 3, 83 * 256); cue++; }          /* -AA- */
        if (cue == 26 && t >= I_T5 + 530u) { orb_glide(&eng, 3, 76 * 256); cue++; }          /* -ow */
        if (cue == 27 && t >= I_T5 + 760u) { orb_note_off(&eng, 3); cue++; }
        if (cue == 28 && t >= I_T5 + 1700u) { orb_note_on(&eng, 4, 83 * 256, 70); cue++; }  /* from a far universe */
        if (cue == 29 && t >= I_T5 + 1760u) { orb_glide(&eng, 4, 86 * 256); cue++; }
        if (cue == 30 && t >= I_T5 + 1960u) { orb_glide(&eng, 4, 79 * 256); cue++; }
        if (cue == 31 && t >= I_T5 + 2120u) { orb_note_off(&eng, 4); cue++; }
    }
    itr.cue = cue;
    if (fm1_ms - itr.last < 40u) return 1;     /* 25 frames a second */
    itr.last = fm1_ms;
    for (h = 0; h < 2u; h++) {                  /* the picture, in two strips */
        cv_begin(240, h ? 96u : 144u, C_BG);
        cv_oy = h ? -144 : 0;
        intro_scene(t);
        cv_oy = 0;
        cv_blit(0, h ? 144u : 0u);
    }
    return 1;
}
#endif

/* ---- MIDI in (the main loop, and app_idle while the screen draws) */
static uint32_t idle_ok, clk_n, clk_t0, clk_bpm, clk_run, clk_last;
static void rec_key(int32_t p);
static void midi_in(void)
{
    uint8_t st, d1, d2;
    int src;
    while ((src = midi_get(&st, &d1, &d2)) != 0) {
        uint32_t ty = st & 0xF0u;
        if (st == 0xF8u) {                      /* the clock: 24 a quarter; its tempo is the orrery's */
            clk_last = fm1_ms;
            if (clk_n % 24u == 0u) {
                if (clk_n) {
                    uint32_t ms = fm1_ms - clk_t0;
                    clk_bpm = ms ? (60000u + ms / 2u) / ms : 0;
                    if (clk_bpm >= 40u && clk_bpm <= 240u && clk_bpm != sys.bpm) { sys.bpm = (uint8_t)clk_bpm; drawn_bar = ~0u; }
                }
                clk_t0 = fm1_ms;
            }
            clk_n++;
            continue;
        }
        if (st == 0xFAu) { clk_n = 0; clk_run = 1; orb_play(&eng, 1); drawn_bar = ~0u; continue; }
        if (st == 0xFBu) { clk_run = 1; if (!eng.playing) orb_play(&eng, 1); drawn_bar = ~0u; continue; }
        if (st == 0xFCu) { clk_run = 0; orb_play(&eng, 0); drawn_bar = ~0u; continue; }
        if (st >= 0xF0u) continue;
        if (set.ch && (st & 15u) != set.ch - 1u) continue;
        if (ty == 0x90u && d2) {
            int32_t k = (int32_t)d1 - 53 - 12 * (int8_t)set.oct;
            int32_t p = sys.keymode == KM_HARMONIC && k >= 0 && k < 27 ? orb_key_pitch(&sys, (uint32_t)k, (int8_t)set.oct, 0)
                                                                         : orb_key_pitch_o(&eng, d1, 0, 0);
            if (focus) p -= 24 * 256;
            orb_note_on_part(&eng, (uint8_t)(128u + d1), p, d2, focus);
            if (rec && focus) rec_key(p);
            (void)src;
        } else if (ty == 0x80u || ty == 0x90u) orb_note_off(&eng, (uint8_t)(128u + d1));
        else if (ty == 0xB0u && d1 == 1u) eng.modw = (int32_t)d2 * 258;
        else if (ty == 0xB0u && (d1 == 123u || d1 == 120u)) orb_all_off(&eng);
        else if (ty == 0xD0u) eng.aft = (int32_t)d1 * 258;
    }
}
static void app_idle(void)
{
    if (idle_ok) midi_in();
}

/* ---- the panel */
static const uint8_t WHITE_KEY[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static int32_t white_index(uint32_t k)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) if (WHITE_KEY[i] == k) return (int32_t)i;
    return -1;
}
static uint32_t held_step = 0xFFu, held_used;

/* a button's pages: the first, then on round them; the hint line shows them all a moment, this one lit */
static void goto_pages(const uint8_t *pg, uint32_t n)
{
    uint32_t i, at = n;
    for (i = 0; i < n; i++) if (page == pg[i]) at = i;
    page = pg[at < n ? (at + 1u) % n : 0u];
    for (i = 0; i < n; i++) tour[i] = pg[i];
    tour_n = (uint8_t)n;
    tour_t = fm1_ms + 1600u;
    main_dirty = knobs_dirty = hint_dirty = 1;
    drawn_bar = ~0u;
}
static void goto_page3(uint32_t a, uint32_t b, uint32_t c)
{
    uint8_t pg[3] = {(uint8_t)a, (uint8_t)b, (uint8_t)c};
    goto_pages(pg, a == c ? (a == b ? 1u : 2u) : 3u);
}
static void goto_page(uint32_t a, uint32_t b) { goto_page3(a, b, a); }

/* REC: a key played into the focused ring, at its step (playing) or the next (stopped). p: the pitch played (1/256
 * semitone), the keys playing the ring's sound where the ring plays it, so the step is the note heard */
static uint32_t rec_pos;
static void rec_key(int32_t p)
{
    uint32_t r = ring_focus(), t = sys.tuning % ORB_NTUNING, n = ORB_SCALE_N[t], i, best = 0;
    orb_ring_t *g = &sys.ring[r];
    int32_t rel = p - (36 + (int32_t)(sys.sun % 12u)) * 256, oct, pos, bd = 1 << 30;
    oct = rel >= 0 ? rel / 3072 : -((3071 - rel) / 3072);
    pos = rel - oct * 3072;
    for (i = 0; i < n; i++) {                   /* the nearest degree */
        int32_t d = ORB_SCALE[t][i] - pos;
        if (d < 0) d = -d;
        if (d < bd) { bd = d; best = i; }
    }
    if (3072 - pos < bd) { oct++; best = 0; }   /* (the next octave's root nearer) */
    undo_take(r, 0);
    {
        uint32_t len = g->len ? g->len : 1u, at = eng.playing ? eng.rs[r].cur : rec_pos % len;
        int32_t deg = oct * (int32_t)n + (int32_t)best - g->deg;
        g->on[at] = g->on[at] ? g->on[at] : 1u;
        g->dg[at] = (int8_t)orb_clamp(deg, -14, 21);
        if (!eng.playing) rec_pos = (at + 1u) % len;
    }
    orb_pattern_edited(&eng, r);
    main_dirty = 1;
}

/* ---- IDEA: a fresh pattern for a ring, as its sound wants it: a kick on the beat, a snare's backbeat, hats between;
 * a pitched one FOLLOWs the chords: a bass on the chord's bass (its neighbours now and then), a melody walking on the
 * chord's notes, so whatever chord comes, it is in it; its LEN and SPEED kept */
static const uint8_t CHORD_T[ORB_NTUNING][6] = {{5, 0, 2, 4, 7, 9}, {5, 0, 1, 2, 4, 6}, {4, 0, 2, 4, 6, 0},
                                                {5, 0, 1, 2, 3, 4}, {5, 0, 2, 4, 7, 9}};   /* [0]: how many */
static int32_t chord_deg(uint32_t t, int32_t c)  /* the c-th chord tone up from the root (an octave each [0]) */
{
    int32_t n = CHORD_T[t][0], o = c >= 0 ? c / n : -((n - 1 - c) / n);
    return o * (int32_t)ORB_SCALE_N[t] + CHORD_T[t][1 + (c - o * n)];
}
static uint32_t irng = 0x9E3779B9u;
static uint8_t scaled(uint32_t h, uint32_t len) { uint32_t v = (h * len + 8u) / 16u; return (uint8_t)(v < 1u ? 1u : v > len ? len : v); }
static void idea(uint32_t r)
{
    orb_ring_t *g = &sys.ring[r];
    uint32_t len = g->len ? g->len : 1u, kind = orb_lib_kind(&sys.snd[ORB_A + r]), x = orb_xs(&irng), i, h = 0;
    uint32_t t = sys.tuning % ORB_NTUNING, n = CHORD_T[t][0];
    int32_t c = 2 + (int32_t)(x >> 24) % 3;
    undo_take(r, 1);
    for (i = 0; i < ORB_NSTEP; i++) { g->on[i] = 0; g->dg[i] = 0; }
    (void)t; (void)n;
    if (kind != LK_KICK && kind != LK_SNARE && kind != LK_HAT) { g->follow = 1; g->deg = 0; }   /* (pitched: FOLLOW) */
    switch (kind) {
    case LK_KICK: { static const uint8_t H[4] = {4, 4, 3, 5}; g->hits = scaled(H[x & 3u], len); g->rot = 0; break; }
    case LK_SNARE: g->hits = scaled(2, len); g->rot = (uint8_t)(len / 4u); break;
    case LK_HAT: {
        static const uint8_t H[4] = {8, 4, 12, 16}, R[4] = {0, 2, 0, 0};
        g->hits = scaled(H[x & 3u], len); g->rot = (uint8_t)(R[x & 3u] * len / 16u);
        break;
    }
    case LK_PERC: g->hits = scaled(3u + x % 3u, len); g->rot = (uint8_t)((x >> 8) % len); break;
    case LK_BASS: { static const uint8_t H[4] = {3, 5, 6, 7}, R[4] = {0, 0, 1, 2}; g->hits = scaled(H[x & 3u], len); g->rot = R[(x >> 2) & 3u]; break; }
    case LK_PAD: g->hits = scaled(1u + x % 3u, len); g->rot = 0; break;
    default: { static const uint8_t H[5] = {4, 5, 6, 7, 9}; g->hits = scaled(H[x % 5u], len); g->rot = (uint8_t)((x >> 8) % len); break; }
    }
    if (g->rot >= len) g->rot = 0;
    orb_euclid(g);
    for (i = 0; i < len; i++) {
        uint32_t y;
        if (!g->on[i]) continue;
        y = orb_xs(&irng);
        switch (kind) {
        case LK_KICK: if (!h) g->on[i] = 2; break;
        case LK_SNARE: g->on[i] = 2; break;
        case LK_HAT: if ((x & 3u) != 1u && (h & 1u)) g->on[i] = 2; break;
        case LK_PERC: if (!h) g->on[i] = 2; break;
        case LK_BASS: {                         /* the chord's bass, mostly; the note under it, or over it, now and then */
            uint32_t w = y % 10u;
            g->dg[i] = (int8_t)(!h || w < 6u ? 0 : w < 8u ? -1 : 1 + (int32_t)((y >> 8) & 1u));
            if (!h) g->on[i] = 2;
            break;
        }
        case LK_PAD: g->dg[i] = (int8_t)((y >> 8) % 4u); break;
        default:                                /* a walk on the chord's notes (#1 .. #6: two octaves of it) */
            if (h) c = orb_clamp(c + (int32_t)((y >> 8) % 5u) - 2, 1, 6);
            g->dg[i] = (int8_t)c;
            if (!h) g->on[i] = 2;
            break;
        }
        h++;
    }
    if (kind == LK_SNARE && (x >> 9) & 1u) {    /* a ghost before the backbeat, now and then */
        uint32_t j = (g->rot + len - 1u) % len;
        if (!g->on[j]) g->on[j] = 1;
    }
    orb_pattern_edited(&eng, r);
}
static void ring_clear(uint32_t r)
{
    orb_ring_t *g = &sys.ring[r];
    uint32_t i;
    undo_take(r, 1);
    for (i = 0; i < ORB_NSTEP; i++) { g->on[i] = 0; g->dg[i] = 0; }
    g->hits = 0;
    orb_pattern_edited(&eng, r);
}
static void ring_undo(uint32_t r)              /* the pattern and the snapshot swap: again is redo */
{
    orb_ring_t *g = &sys.ring[r];
    uint32_t i;
    if (!undo[r].ok) { say("NOTHING TO UNDO"); return; }
    for (i = 0; i < ORB_NSTEP; i++) {
        uint8_t o = g->on[i];
        int8_t d = g->dg[i];
        g->on[i] = undo[r].on[i]; g->dg[i] = undo[r].dg[i];
        undo[r].on[i] = o; undo[r].dg[i] = d;
    }
    { uint8_t a = g->len, b = g->hits, c = g->rot; g->len = undo[r].len; g->hits = undo[r].hits; g->rot = undo[r].rot;
      undo[r].len = a; undo[r].hits = b; undo[r].rot = c; }
    { uint8_t f = g->follow; int8_t d = g->deg; g->follow = undo[r].follow; g->deg = undo[r].deg;
      undo[r].follow = f; undo[r].deg = d; }
    undo_t[r] = 0;                              /* (the next edit is a new gesture) */
    undo_redo[r] = !undo_redo[r];
    orb_pattern_edited(&eng, r);
}

/* STEPS: the black keys. F# G# A# C# below: the track (A..D); D#: an IDEA; F# G# A# C# above: mute A..D; D#: CLEAR;
 * the top F#: UNDO */
static int32_t black_index(uint32_t k)
{
    uint32_t i;
    for (i = 0; i < 11u; i++) if (BLACK_KEY[i] == k) return (int32_t)i;
    return -1;
}
static void step_key(int32_t f)
{
    char s[32], *p;
    uint32_t r = ring_focus();
    if (f < 0) return;
    p = put_s(s, ORB_PART_NAME[ORB_A + (f >= BK_MA && f <= BK_MD ? (uint32_t)(f - BK_MA) : f <= BK_D ? (uint32_t)f : r)]);
    *p++ = ' ';
    if (f <= BK_D) {
        focus = ORB_A + (uint32_t)f;
        put_s(p, orb_lib_name(&sys.snd[focus]));
        drawn_bar = ~0u; knobs_dirty = 1;
    } else if (f == BK_IDEA) { idea(r); put_s(p, "AN IDEA, nya~"); cat_happy(); love_t = fm1_ms; }
    else if (f <= BK_MD) {
        orb_ring_t *g = &sys.ring[f - BK_MA];
        g->mute = !g->mute;
        put_s(p, g->mute ? "NAPS  zz" : "IS AWAKE");
    } else if (f == BK_CLEAR) { ring_clear(r); put_s(p, "ALL CLEAN"); }
    else { ring_undo(r); put_s(p, undo[r].ok ? (undo_redo[r] ? "UNDONE" : "REDONE") : "NOTHING TO UNDO"); }
    if (!(f == BK_UNDO && !undo[r].ok)) say(s);
    main_dirty = 1;
}

static void handle_keys(void)
{
    static uint32_t prev;
    static uint8_t held_id[27];
    uint32_t keys = fm1_in.notes, down = keys & ~prev, up = prev & ~keys, k;
    int32_t oct = (int8_t)set.oct;
    prev = keys;
    for (k = 0; k < 27u; k++) {
        uint32_t note = (uint32_t)(53 + (int32_t)k + 12 * oct);
        if (down >> k & 1u) {
            if (isel()) {                       /* SEL + a key: the sun moves */
                sys.sun = (uint8_t)(note % 12u);
                drawn_bar = ~0u; main_dirty = knobs_dirty = 1;
                say("THE SUN MOVES");
                held_id[k] = 0;
                continue;
            }
            if (page == PG_RING) {              /* the steps on the white keys; the black ones: tracks, IDEA, mutes .. */
                int32_t wi = white_index(k);
                held_id[k] = 0;
                if (wi >= 0) { held_step = step_page * 16u + (uint32_t)wi; held_used = 0; }
                else step_key(black_index(k));
                continue;
            }
            {   /* the keys play the focus: KEYS, or a track's sound where its ring plays it (the keys' C4 is its root) */
                int32_t p = orb_key_pitch_o(&eng, sys.keymode == KM_HARMONIC ? k : note, oct, 0);
                if (focus) p -= 24 * 256;
                held_id[k] = (uint8_t)(k + 1u);
                mew_t = fm1_ms; mew_n++;
                orb_note_on_part(&eng, held_id[k], p, 100, focus);
                midi_send(0x90u | (set.ch ? set.ch - 1u : 0u), note & 127u, 100);
                if (rec && focus) rec_key(p);
            }
        }
        if (up >> k & 1u) {
            if (page == PG_RING && held_step != 0xFFu && white_index(k) >= 0 &&
                held_step == step_page * 16u + (uint32_t)white_index(k)) {
                orb_ring_t *g = &sys.ring[ring_focus()];
                if (!held_used && held_step < g->len) {   /* a tap: rest -> hit -> accent -> rest */
                    undo_take(ring_focus(), 0);
                    if (!g->on[held_step]) love_t = fm1_ms;
                    g->on[held_step] = (uint8_t)((g->on[held_step] + 1u) % 3u);
                    orb_pattern_edited(&eng, ring_focus());
                    main_dirty = 1;
                }
                held_step = 0xFFu;
            }
            if (held_id[k]) {
                orb_note_off(&eng, held_id[k]);
                midi_send(0x80u | (set.ch ? set.ch - 1u : 0u), note & 127u, 0);
                held_id[k] = 0;
            }
        }
    }
}

static void handle_encoders(void)
{
    uint32_t k, lay = isel() ? 4u : 0u;
    int32_t e;
    static const uint8_t ENC[4] = {ENC_KNOB1, ENC_KNOB2, ENC_KNOB3, ENC_KNOB4};
    for (k = 0; k < 4u; k++) {
        if ((e = fm1_enc_take(ENC[k])) == 0) continue;
        if (page == PG_RING && k == 0u && held_step != 0xFFu) {   /* a held step: its degree */
            orb_ring_t *g = &sys.ring[ring_focus()];
            if (held_step < ORB_NSTEP) {
                if (!held_used) undo_take(ring_focus(), 0);
                g->dg[held_step] = (int8_t)orb_clamp(g->dg[held_step] + e, -14, 21);
                if (!g->on[held_step]) g->on[held_step] = 1;
                orb_pattern_edited(&eng, ring_focus());
                held_used = 1;
                {
                    char s[24], *p = put_s(s, g->follow ? "STEP NOTE " : "STEP DEG ");
                    put_note_of(p, ring_focus(), g->deg + g->dg[held_step]);
                    say(s);
                }
                main_dirty = hint_dirty = 1;
            }
            continue;
        }
        {
            const pd_t *d = &PD[page][k + lay];
            int32_t step = (e > 2 || e < -2) ? e * 2 : e;
            if (d->fmt == F_BPM || d->fmt == F_NUM || d->fmt == F_PCT || d->fmt == F_TIME) step = e > 3 || e < -3 ? e * 3 : e;
            pd_set(d, pd_get(d) + (d->hi - d->lo > 20 ? step : e));
            knobs_dirty = main_dirty = 1;
            if (d->fmt == F_BPM || d->off == SO(sun) || d->where == W_SET) drawn_bar = ~0u;
        }
    }
    if ((e = fm1_enc_take(ENC_ALGORITHM)) != 0) {
        const pd_t *d = &PD[page][8];
        if (d->where != W_NONE || d->fmt == F_SLOT) {
            pd_set(d, pd_get(d) + e);
            hint_dirty = knobs_dirty = main_dirty = 1;
            if (d->fmt == F_BPM) drawn_bar = ~0u;
        }
    }
    if ((e = fm1_enc_take(ENC_SELECT)) != 0) { /* the focus */
        int32_t f = (int32_t)focus + (e > 0 ? 1 : -1);
        focus = (uint32_t)(f < 0 ? 4 : f > 4 ? 0 : f);
        if (page == PG_RING && !focus) focus = e > 0 ? 1u : 4u;   /* (RING: a ring) */
        drawn_bar = ~0u; knobs_dirty = main_dirty = 1;
    }
    if ((e = fm1_enc_take(ENC_PRESETS)) != 0) {
        int32_t i;
        if (save_mode) {
            i = (int32_t)save_to + e;
            save_to = (uint32_t)(i < 0 ? 0 : i > (int32_t)NBLOB - 1 ? (int32_t)NBLOB - 1 : i);
        } else {
            i = (int32_t)(sys_pending != 0xFFu ? sys_pending : sysi) + e;
            i = (i % (int32_t)NSYS + (int32_t)NSYS) % (int32_t)NSYS;
            sys_pending = (uint32_t)i;
            sys_pend_t = fm1_ms;
            {
                char s[20];
                sys_label((uint32_t)i, s);
                say(s);
            }
        }
        drawn_bar = ~0u; hint_dirty = 1;
    }
}

static void handle_buttons(void)
{
    uint32_t rel, b = fm1_input_edges(&rel);
    static uint32_t sel_was;
    if (isel() != sel_was) { sel_was = isel(); knobs_dirty = 1; main_dirty = 1; }
    if (rel & (1u << BTN_SEL)) knobs_dirty = 1;
    if (!b) return;
    if (save_mode && !(b & (1u << BTN_SAVE)) && !(b & (1u << BTN_SEL))) { save_mode = 0; say("NOT SAVED"); drawn_bar = ~0u; }
    if (b & (1u << BTN_HOME)) {
        if (isel()) { cute_new(); goto_page3(PG_CUTE, PG_CUTE, PG_CUTE); drawn_bar = ~0u; main_dirty = knobs_dirty = 1; }   /* SEL+HOME: a new one */
        else goto_page3(PG_ORRERY, PG_MACRO, PG_CUTE);
    }
    if (b & (1u << BTN_SEQ)) {
        if (isel()) {
            orb_ring_t *g = &sys.ring[ring_focus()];
            g->mute = !g->mute;
            say(g->mute ? "MUTED" : "UNMUTED");
            main_dirty = knobs_dirty = 1;
        } else {
            goto_page(PG_RING, PG_TIME);
            if (!focus) focus = ORB_A;
        }
    }
    if (b & (1u << BTN_EDIT)) { static const uint8_t E[4] = {PG_SOUND, PG_ORBIT, PG_BODY, PG_COSMOS}; goto_pages(E, 4); }
    if (b & (1u << BTN_ENV)) goto_page(PG_ENV, PG_ENV);
    if (b & (1u << BTN_LFO)) goto_page(PG_MOD, PG_LFO);
    if (b & (1u << BTN_FX)) goto_page3(PG_FILTER, PG_SPACE, PG_HORIZON);
    if (b & (1u << BTN_GLO)) {
        if (isel()) {                           /* SEL+GLO: the chords out (they fade), and in again */
            sys.bg_out = !sys.bg_out;
            say(sys.bg_out ? "BACKGROUND: OUT" : sys.bg_level ? "BACKGROUND: IN" : "BACKGROUND: IN (LEVEL 0)");
            main_dirty = 1;
        } else
            goto_page(PG_BG, PG_PAD);
    }
    if (b & (1u << BTN_ARP)) goto_page(PG_KEYS, PG_KEYS);
    if (b & (1u << BTN_PLAY)) { if (!eng.playing) cat_happy(); orb_play(&eng, !eng.playing); drawn_bar = ~0u; }
    if (b & (1u << BTN_REC)) {                  /* REC: the keys into the focused track (the keys: into A) */
        char s[28];
        rec = !rec; rec_pos = 0;
        if (rec && !focus) { focus = ORB_A; knobs_dirty = 1; }
        put_s(put_s(s, rec ? "REC: KEYS INTO " : "REC OFF"), rec ? ORB_PART_NAME[focus] : "");
        say(s);
        drawn_bar = ~0u; main_dirty = 1;
    }
    if (b & (1u << BTN_OCT_DOWN)) {
        if (page == PG_RING) { if (step_page) step_page--; main_dirty = 1; }
        else if ((int8_t)set.oct > -3) { set.oct = (uint8_t)((int8_t)set.oct - 1); set_touch(); knobs_dirty = 1; }
    }
    if (b & (1u << BTN_OCT_UP)) {
        if (page == PG_RING) { if (step_page < 3u) step_page++; main_dirty = 1; }
        else if ((int8_t)set.oct < 3) { set.oct = (uint8_t)((int8_t)set.oct + 1); set_touch(); knobs_dirty = 1; }
    }
    if (b & (1u << BTN_SAVE)) {
        if (!save_mode) {
            save_mode = 1;
            save_to = sysi >= ORB_NSYSTEM ? sysi - ORB_NSYSTEM : 0u;
            say("PRESETS: WHERE  SAVE: STORE");
        } else {
            uint32_t i;
            for (i = 0; i < 11u; i++) sys.name[i] = 0;
            sys.name[0] = 'U'; sys.name[1] = (char)('1' + save_to);
            orb_all_off(&eng);
            if (blob_save(save_to, &sys, sizeof sys) == 0) { sysi = ORB_NSYSTEM + save_to; set.sysi = (uint8_t)sysi; made = 0; set_touch(); say("SAVED, mrrp!"); cat_happy(); }
            else say("SAVE FAILED");
            save_mode = 0;
        }
        drawn_bar = ~0u;
    }
    hint_dirty = 1;
}

/* ---- the LEDs: the page's button, PLAY, REC; the keys: what sounds, the sun's keys glow; on RING the steps */
static void leds(void)
{
    uint32_t k, lit = 0;
    int32_t oct = (int8_t)set.oct;
    fm1_led_key(BTN_HOME, page == PG_ORRERY || page == PG_MACRO || page == PG_CUTE);
    fm1_led_key(BTN_SEQ, page == PG_RING || page == PG_TIME);
    fm1_led_key(BTN_EDIT, page == PG_SOUND || page == PG_ORBIT || page == PG_BODY || page == PG_COSMOS);
    fm1_led_key(BTN_ENV, page == PG_ENV);
    fm1_led_key(BTN_LFO, page == PG_MOD || page == PG_LFO);
    fm1_led_key(BTN_FX, page == PG_FILTER || page == PG_SPACE || page == PG_HORIZON);
    fm1_led_key(BTN_GLO, page == PG_BG || page == PG_PAD);
    fm1_led_key(BTN_ARP, page == PG_KEYS);
    fm1_led_key(BTN_PLAY, eng.playing ? (eng.T % (4u * ORB_TPS)) < ORB_TPS * 3u : 0);
    fm1_led_key(BTN_REC, rec && ((fm1_ms >> 8) & 1u));
    fm1_led_key(BTN_SAVE, save_mode && ((fm1_ms >> 7) & 1u));
    fm1_led_key(BTN_OCT_DOWN, page == PG_RING ? step_page > 0u : oct < 0);
    fm1_led_key(BTN_OCT_UP, page == PG_RING ? step_page < 3u : oct > 0);
    if (page == PG_RING) {
        const orb_ring_t *g = &sys.ring[ring_focus()];
        uint32_t cur = eng.rs[ring_focus()].cur, i;
        for (i = 0; i < 16u; i++) {
            uint32_t st = step_page * 16u + i, kk = WHITE_KEY[i];
            int on = st < g->len && g->on[st];
            int play = eng.playing && st == cur;
            fm1_led_key(KEY_LED(kk), play ? !on : on);
            fm1_led_key_glow(KEY_LED(kk), st < g->len);
        }
        for (k = 0; k < 11u; k++) {
            uint32_t kk = BLACK_KEY[k];
            int on = 0;
            if (k <= BK_D) on = focus == ORB_A + k;
            else if (k >= BK_MA && k <= BK_MD) on = !sys.ring[k - BK_MA].mute && fm1_ms - flash_t[k - BK_MA] > 60u;
            fm1_led_key(KEY_LED(kk), on);
            fm1_led_key_glow(KEY_LED(kk), 1);
        }
        return;
    }
    for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)
        if (eng.v[k].on && eng.v[k].id >= 1u && eng.v[k].id <= 27u) lit |= 1u << (eng.v[k].id - 1u);
    for (k = 0; k < 27u; k++) {
        uint32_t note = (uint32_t)(53 + (int32_t)k + 12 * oct);
        fm1_led_key(KEY_LED(k), lit >> k & 1u);
        fm1_led_key_glow(KEY_LED(k), sys.keymode == KM_HARMONIC ? white_index(k) == 0 :
                         sys.keymode == KM_CHORD ? chord_bass_key(note) : note % 12u == sys.sun % 12u);
    }
}

static uint32_t boot_phase;                     /* 1 the legend (the host, for the page), 2 the splash, 3 playing */
static void app_start(void)
{
    if (settings_load(&set, sizeof set) || set.ver != SET_VER || (int8_t)set.oct < -3 || (int8_t)set.oct > 3 || set.ch > 16u ||
        set.sysi >= NSYS)
        set.oct = 0, set.ch = 0, set.sysi = 0, set.ver = SET_VER;
    orb_system(0, &sys);                        /* (the splash is silent; boot_finish loads the system to play) */
    orb_init(&eng, &sys, &orb_mem);
#ifdef ORB_LEGEND
    if (getenv("ORB_INTRO")) { orb_intro_system(&sys); intro_begin(); boot_phase = 1; return; }
#endif
    splash_begin(0);
    boot_phase = 2;
}
static void boot_finish(void)                   /* the system to play, once the story's tails have gone */
{
    orb_all_off(&eng);
    if (set.sysi) system_load(set.sysi);
    else { orb_system(0, &sys); orb_pattern_edited(&eng, ORB_NRING); }
    sysi = set.sysi;
    set_dirty = 0;
    lcd_fill(0, 0, 240, 240, C_BG);
    boot_phase = 3;
}
static void boot_pass(void)                     /* the splash (PLAY skips it); on the host, the legend before it */
{
#ifdef ORB_LEGEND
    if (boot_phase == 1) {
        if (intro_pass()) return;
        intro_end();
        splash_begin(1);
        boot_phase = 2;
        return;
    }
#endif
    if (!splash_pass()) boot_finish();
}
static void app_pass(void)                      /* one pass of the main loop */
{
    static uint32_t t_hint, peak_t, msg_drawn;
    platform_poll();
    handle_buttons();
    handle_encoders();
    handle_keys();
    midi_in();
    if (clk_run && fm1_ms - clk_last > 2000u) clk_run = 0;
    {   /* each track's last hit (the screens' flashes, the mute keys' wink) */
        uint32_t r;
        for (r = 0; r < ORB_NRING; r++) if (eng.rs[r].flash != flash_seen[r]) { flash_seen[r] = eng.rs[r].flash; flash_t[r] = fm1_ms; }
    }
    if (preview_t && fm1_ms >= preview_t) { orb_note_off(&eng, 99); preview_t = 0; }
    if (sys_pending != 0xFFu && fm1_ms - sys_pend_t > 450u) {   /* PRESETS: loaded once it rests */
        system_load(sys_pending);
        sys_pending = 0xFFu;
        drawn_bar = ~0u; knobs_dirty = main_dirty = hint_dirty = 1;
    }
    leds();
    idle_ok = 1;
    {
        uint32_t bar = page | focus << 4 | sysi << 8 | sys.sun << 13 | (uint32_t)sys.bpm << 17 | eng.playing << 25 |
                       rec << 26 | save_mode << 27 | save_to << 28;
        if (bar != drawn_bar) { drawn_bar = bar; draw_bar(); }
    }
    if (knobs_dirty) { knobs_dirty = 0; draw_knobs(); }
    if ((page_animated() && fm1_ms - t_main >= 40u) || main_dirty) { main_dirty = 0; t_main = fm1_ms; draw_main(); }
    if (msg_seq != msg_drawn) { msg_drawn = msg_seq; hint_dirty = 1; }
    if (hint_dirty || (msg_t && fm1_ms >= msg_t) || fm1_ms - t_hint > 500u) {
        if (msg_t && fm1_ms >= msg_t) msg_t = 0;
        hint_dirty = 0; t_hint = fm1_ms;
        draw_hint();
    }
    idle_ok = 0;
    if (fm1_ms - peak_t > 1000u) { peak_t = fm1_ms; cpu_max = 0; eng.peak = 0; }
    if (set_dirty && fm1_ms - set_t > 2000u && !eng.playing) {   /* settings: when nothing plays (an erase stalls) */
        uint32_t v, quiet = 1;
        for (v = 0; v < ORB_NV; v++) if (eng.v[v].stage) quiet = 0;
        if (quiet) { settings_save(&set, sizeof set); set_dirty = 0; }
    }
}

static void app_main(void)                      /* the FM-1 and the host: the start, the story, the splash, the loop for
                                                 * ever. The page (DINGHY_WEB, web/app/web_host.c): a pass a call */
{
#ifdef DINGHY_WEB
    static int started;
    if (!started) { started = 1; app_start(); return; }
    if (boot_phase < 3u) boot_pass();
    else app_pass();
#else
    app_start();
    while (boot_phase < 3u) boot_pass();
    for (;;) app_pass();
#endif
}
