/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX: an orbital groove engine for the M-VAVE FM-1. Every rhythm is an interval, and every interval a rhythm:
 * four rings circle a sun at small ratios, and the same ratios, heard as pitch, are pure intervals above it. The voice
 * is a planet on an ellipse; you hear where it is (docs/orbitrix/PLAN.md). Built on Dinghy 0.5 (docs/DINGHY.md).
 *
 *   HOME      ORRERY -> MACROS        SEQ   RING -> TIME          EDIT  ORBIT -> BODY       ENV  ENV
 *   LFO       MOD -> LFO              FX    FILTER MAP -> SPACE   GLO   ORBIT PAD           ARP  KEYS
 *   K1..K4    the four values in the row; SEL held: the second row. ALGORITHM: the page's extra (the hint line)
 *   SELECT    the focus: KEYS, A, B, C, D (the sound and the ring the pages edit)
 *   PRESETS   the system (10 built in, U1..U8 your own); SAVE: SAVE again to store it in the place PRESETS picks
 *   PLAY      the orrery; REC: the keys write into the focused ring (playing: at its step)
 *   keys      play (SOLAR: each key retuned to the sun; HARMONIC: white keys harmonics 1..16, black 17..27);
 *             SEL + a key: the sun moves there. On RING the white keys are the ring's steps (OCT +-: the page of 16):
 *             a tap toggles rest -> hit -> accent, a held step and K1 set its degree
 *   SEL + SEQ the focused ring muted / not.  OCT- OCT+: the keys' octave
 *   MIDI      notes play the keys, CC 1 is MODW, channel pressure AFT; clock and START / STOP drive the orrery */
static void app_idle(void);
#define LCD_IDLE app_idle
#ifndef APP_NAME
#define APP_NAME "ORBITRIX"
#endif
#define CV_MAX (240u * 144u)                    /* the main picture in one canvas (69 KB of the pool) */
#include "platform.c"
#include "orb_systems.h"
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
static orb_mem_t orb_mem __attribute__((section(".pool")));
static volatile uint32_t cpu_t, cpu_max;        /* the render's time (ticks) a callback: the latest, the worst */

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t t0 = fm1_ticks(), dt;
    orb_render(&eng, out, n);
    dt = fm1_ticks() - t0;
    cpu_t = dt;
    if (dt > cpu_max) cpu_max = dt;
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
static struct { uint8_t oct, ch, sysi, rsv; } set = {0, 0, 0, 0};
static uint32_t set_dirty, set_t;
static void set_touch(void) { set_dirty = 1; set_t = fm1_ms; }

/* ---- the pages and their values */
enum { PG_ORRERY, PG_MACRO, PG_RING, PG_TIME, PG_ORBIT, PG_BODY, PG_ENV, PG_MOD, PG_LFO, PG_FILTER, PG_SPACE, PG_PAD,
       PG_KEYS, NPAGE };
static const char *const PAGE_NAME[NPAGE] = {"ORRERY", "MACROS", "RING", "TIME", "ORBIT", "BODY", "ENV", "MOD", "LFO",
                                             "FILTER MAP", "SPACE", "ORBIT PAD", "KEYS"};
enum { W_SYS, W_SND, W_RING, W_RA, W_RB, W_RC, W_RD, W_MOD, W_SET, W_NONE };
enum { F_NUM, F_SIGNED, F_TIME, F_RATIO, F_DIR, F_LOGIC, F_DEG, F_TUNING, F_NOTE, F_KMODE, F_MRATIO, F_DT, F_SYNC,
       F_SHAPE, F_SRC, F_DST, F_OCT, F_BPM, F_ONOFF, F_PAN, F_LRATE0, F_LRATE1, F_TRIG, F_CH, F_SLOT, F_PCT };
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
                   {"ROT A", W_RA, F_NUM, RO(rot), 0, 63}, {"ROT B", W_RB, F_NUM, RO(rot), 0, 63},
                   {"ROT C", W_RC, F_NUM, RO(rot), 0, 63}, {"ROT D", W_RD, F_NUM, RO(rot), 0, 63}, P_BPM},
    [PG_MACRO] = {{"CHAOS", W_SYS, F_PCT, SO(macro[MAC_CHAOS]), 0, 99}, {"SPECTR", W_SYS, F_PCT, SO(macro[MAC_SPECTRAL]), 0, 99},
                  {"GRAVITY", W_SYS, F_PCT, SO(macro[MAC_GRAVITY]), 0, 99}, {"ORBIT", W_SYS, F_PCT, SO(macro[MAC_ORBIT]), 0, 99},
                  {"FRACT", W_SYS, F_PCT, SO(macro[MAC_FRACT]), 0, 99}, {"MUTATE", W_SYS, F_PCT, SO(mutate), 0, 99},
                  {"HUM", W_SYS, F_PCT, SO(hum), 0, 99}, {"VAR", W_SYS, F_PCT, SO(var), 0, 99}, P_BPM},
    [PG_RING] = {{"RATIO", W_RING, F_RATIO, RO(ratio), 0, ORB_NRATIO - 1}, {"LEN", W_RING, F_NUM, RO(len), 1, 64},
                 {"HITS", W_RING, F_NUM, RO(hits), 0, 64}, {"ROT", W_RING, F_NUM, RO(rot), 0, 63},
                 {"DEG", W_RING, F_DEG, RO(deg), -14, 21}, {"DIR", W_RING, F_DIR, RO(dir), 0, ORB_NDIR - 1},
                 {"PROB", W_RING, F_PCT, RO(prob), 0, 99}, {"LOGIC", W_RING, F_LOGIC, RO(logic), 0, 13}, P_BPM},
    [PG_TIME] = {P_BPM, {"SWING", W_SYS, F_PCT, SO(swing), 0, 99}, {"MUTATE", W_SYS, F_PCT, SO(mutate), 0, 99},
                 {"HUM", W_SYS, F_PCT, SO(hum), 0, 99}, {"VAR", W_SYS, F_PCT, SO(var), 0, 99},
                 {"GATE", W_SND, F_PCT, NO(gate), 1, 99}, {"MUTE", W_RING, F_ONOFF, RO(mute), 0, 1},
                 {"TRIG", W_SND, F_TRIG, NO(flags), 0, 3}, P_BPM},
    [PG_ORBIT] = {{"ECC", W_SND, F_NUM, NO(ecc), 0, 99}, {"SLING", W_SND, F_NUM, NO(sling), 0, 99},
                  {"ANG", W_SND, F_NUM, NO(ang), 0, 99}, {"SPR", W_SND, F_NUM, NO(spr), 0, 99},
                  {"PREC", W_SND, F_NUM, NO(prec), 0, 99}, {"MOON", W_SND, F_NUM, NO(moon), 0, 99},
                  {"RATIO", W_SND, F_MRATIO, NO(mratio), 0, ORB_NMOON - 1}, {"DUST", W_SND, F_NUM, NO(dust), 0, 99}, P_BPM},
    [PG_BODY] = {{"FALL", W_SND, F_NUM, NO(fall), 0, 99}, {"FALL T", W_SND, F_TIME, NO(fallt), 0, 99},
                 {"COLOR", W_SND, F_NUM, NO(color), 0, 99}, {"PIXEL", W_SND, F_NUM, NO(pixel), 0, 99},
                 {"LEVEL", W_SND, F_NUM, NO(level), 0, 99}, {"PAN", W_SND, F_PAN, NO(pan), 0, 99},
                 {"SEND", W_SND, F_NUM, NO(send), 0, 99}, {"OCT", W_SND, F_OCT, NO(oct), -3, 3},
                 {"TRIG", W_SND, F_TRIG, NO(flags), 0, 3}},
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
    [PG_PAD] = {{"X", W_SYS, F_NUM, SO(ox), 0, 99}, {"Y", W_SYS, F_NUM, SO(oy), 0, 99},
                {"RAD", W_SYS, F_NUM, SO(orad), 0, 50}, {"PERIOD", W_SYS, F_SYNC, SO(orate), 0, ORB_NSYNC - 1},
                P_NONE, P_NONE, P_NONE, P_NONE, P_BPM},
    [PG_KEYS] = {{"MODE", W_SYS, F_KMODE, SO(keymode), 0, 1}, {"TUNING", W_SYS, F_TUNING, SO(tuning), 0, ORB_NTUNING - 1},
                 {"SUN", W_SYS, F_NOTE, SO(sun), 0, 11}, {"OCT", W_SET, F_OCT, 0, -3, 3},
                 {"GRAVITY", W_SYS, F_PCT, SO(macro[MAC_GRAVITY]), 0, 99}, {"MIDI CH", W_SET, F_CH, 1, 0, 16},
                 P_NONE, P_NONE, P_BPM},
};
static uint32_t page = PG_ORRERY, focus = ORB_A, slot;   /* focus: 0 KEYS, 1..4 A..D */
static uint32_t ring_focus(void) { return focus ? focus - 1u : 0u; }
static uint8_t *pd_ptr(const pd_t *d)
{
    switch (d->where) {
    case W_SYS: return (uint8_t *)&sys + d->off;
    case W_SND: return (uint8_t *)&sys.snd[focus] + d->off;
    case W_RING: return (uint8_t *)&sys.ring[ring_focus()] + d->off;
    case W_RA: case W_RB: case W_RC: case W_RD: return (uint8_t *)&sys.ring[d->where - W_RA] + d->off;
    case W_MOD: return (uint8_t *)&sys.mod[slot % ORB_NMOD] + d->off;
    case W_SET: return (uint8_t *)&set + d->off;
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
    *p = (uint8_t)v;
    if (d->where == W_SET) set_touch();
    if (d->where >= W_RING && d->where <= W_RD && (d->off == RO(len) || d->off == RO(hits) || d->off == RO(rot))) {
        orb_ring_t *r = (orb_ring_t *)(p - d->off);
        if (r->hits > r->len) r->hits = r->len;
        if (r->rot >= r->len) r->rot = (uint8_t)(r->len - 1u);
        orb_euclid(r);
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
    case F_DEG: put_deg(s, v); break;
    case F_TUNING: put_s(s, ORB_TUNING_NAME[v % ORB_NTUNING]); break;
    case F_NOTE: put_s(s, NOTE_NAME[v % 12]); break;
    case F_KMODE: put_s(s, v ? "HARMON" : "SOLAR"); break;
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
    case F_BPM: case F_PCT: case F_NUM: default: put_i(s, v, 0); break;
    }
}
static uint32_t pd_fill(const pd_t *d)          /* 0..1024: how full its gauge is */
{
    int32_t v = pd_get(d), span = d->hi - d->lo;
    if (d->fmt == F_LRATE0 || d->fmt == F_LRATE1) span = 99;
    return span > 0 ? (uint32_t)((v - d->lo) * 1024 / span) : 0u;
}

/* ---- the systems: 10 built in, U1..U8 in the blobs */
#define NSYS (ORB_NSYSTEM + NBLOB)
static uint32_t sysi, sys_pending = 0xFFu, sys_pend_t, msg_t;
static char msg[32];
static void say(const char *s) { put_s(msg, s); msg_t = fm1_ms + 1400u; }
static int user_load(uint32_t u, orb_sys_t *dst) { return blob_load(u, dst, sizeof *dst); }
static void system_load(uint32_t i)
{
    static orb_sys_t tmp;
    if (i >= ORB_NSYSTEM) {
        if (user_load(i - ORB_NSYSTEM, &tmp)) { say("EMPTY"); return; }
    } else
        orb_system(i, &tmp);
    orb_all_off(&eng);
    sys = tmp;
    sysi = i;
    set.sysi = (uint8_t)i;
    set_touch();
}
static void sys_label(uint32_t i, char *s)
{
    if (i < ORB_NSYSTEM) { put_s(s, ORB_SYSTEM_NAME[i]); return; }
    s[0] = 'U'; s[1] = (char)('1' + i - ORB_NSYSTEM); s[2] = 0;
}

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
    pill(2, 2, x, 18, C_LIME);
    tx(8, 3, &FONT_S, PAGE_NAME[page], C_INK, C_LIME);
    if (page != PG_ORRERY && page != PG_MACRO && page < PG_FILTER) {   /* the focus chip */
        pill(x + 6, 2, 18, 18, RINGC[focus]);
        txc(x + 15, 3, &FONT_S, focus ? ORB_PART_NAME[focus] : "K", C_INK, RINGC[focus]);
        x += 24;
    }
    if (save_mode) {
        char *p = put_s(s, "SAVE> ");
        sys_label(ORB_NSYSTEM + save_to, p);
        tx(x + 8, 3, &FONT_S, s, C_MAG, C_BG);
    } else {
        sys_label(sysi, s);
        if (text_w(&FONT_S, s) < 240 - x - 74) tx(x + 8, 3, &FONT_S, s, C_GREY, C_BG);
    }
    {   /* right: the sun and the tempo */
        char *p = put_s(s, NOTE_NAME[sys.sun % 12u]);
        int32_t w;
        *p++ = ' ';
        p = put_u(p, sys.bpm);
        w = text_w(&FONT_S, s);
        tx(240 - w - (eng.playing ? 14 : 4), 3, &FONT_S, s, C_TXT, C_BG);
        disc(240 - w - (eng.playing ? 14 : 4) - 8, 11, 3, C_YEL);
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
    else {
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
    static const int16_t RAD[4] = {26, 40, 54, 68};
    int32_t cx = 80, cy = 72;
    uint32_t r, i;
    cv_begin(240, 144, C_BG);
    stars(240, 144, 77, 46);
    for (r = 0; r < ORB_NRING; r++) {
        const orb_ring_t *g = &sys.ring[r];
        uint32_t len = g->len ? g->len : 1u, fl = eng.rs[r].flash;
        uint16_t c = RINGC[r + 1u];
        int32_t rad = RAD[r];
        if (fl != flash_seen[r]) { flash_seen[r] = fl; flash_t[r] = fm1_ms; }
        circle(cx, cy, rad, dim565(c, g->mute ? 2u : 4u), 3);
        for (i = 0; i < len; i++) {
            uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)i << 32) / len);
            int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad);
            if (g->on[i]) {
                uint16_t cc = g->mute ? dim565(c, 5) : (g->on[i] == 2u ? mix565(c, C_TXT, 8) : c);
                cv_rect(x - 2, y - 2, 4, 4, cc);
            } else
                cv_pset(x, y, dim565(c, 6));
        }
        if (eng.playing) {                      /* the planet: between its last step and the next */
            const orb_rstate_t *R = &eng.rs[r];
            uint32_t idx = R->cur, ph = 0xC0000000u + (uint32_t)(((uint64_t)idx << 32) / len)
                         + (uint32_t)(((uint64_t)ring_progress(r) << 16) / len);
            int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad), big = fm1_ms - flash_t[r] < 120u;
            if (big) disc(x, y, 5, dim565(C_TXT, 6));
            disc(x, y, 3, big ? C_TXT : c);
            if (conj_t && fm1_ms - conj_t < 160u && big) cv_line(cx, cy, x, y, mix565(c, C_TXT, 10));
        }
    }
    if (eng.conj_count != conj_seen) { conj_seen = eng.conj_count; conj_t = fm1_ms; }
    {   /* the sun: it flares at a conjunction */
        int32_t fl = conj_t && fm1_ms - conj_t < 200u ? (int32_t)(200u - (fm1_ms - conj_t)) / 50 : 0;
        disc(cx, cy, 14 + fl, dim565(C_YEL, 4));
        disc(cx, cy, 11 + fl / 2, C_YEL);
        txc(cx, cy - 8, &FONT_S, NOTE_NAME[sys.sun % 12u], C_INK, C_YEL);
    }
    /* the lanes: each ring's ratio and length */
    for (r = 0; r < ORB_NRING; r++) {
        const orb_ring_t *g = &sys.ring[r];
        char s[16], *p = s;
        int32_t y = 8 + (int32_t)r * 34;
        uint16_t c = RINGC[r + 1u], bg = C_BG;
        frame(158, y, 80, 30, g->mute ? C_FDIM : dim565(c, 9));
        circle(168, y + 9, 4, c, 0);
        if (fm1_ms - flash_t[r] < 100u) disc(168, y + 9, 3, c);
        p = put_s(p, ORB_RATIO_NAME[g->ratio % ORB_NRATIO]);
        tx(178, y + 2, &FONT_S, s, g->mute ? C_DIMT : C_TXT, bg);
        p = put_u(s, g->len);
        p = put_s(p, " ");
        p = put_deg(p, g->deg);
        tx(164, y + 15, &FONT_S, s, g->mute ? C_DIMT : dim565(c, 12), bg);
        if (g->mute) tx(222, y + 2, &FONT_S, "M", C_MAG, bg);
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
static void draw_macros(void)
{
    uint32_t m;
    static const uint8_t ORDER[5] = {MAC_CHAOS, MAC_SPECTRAL, MAC_GRAVITY, MAC_FRACT, MAC_ORBIT};
    static const uint16_t MC[5] = {C_VIO, C_MAG, C_YEL, C_CYAN, C_LIME};
    cv_begin(240, 144, C_BG);
    stars(240, 144, 31, 30);
    for (m = 0; m < 5u; m++) {
        uint32_t id = ORDER[m], v = sys.macro[id], t = fm1_ms;
        int32_t x = 2 + (int32_t)m * 47, cx = x + 22, w = 44;
        uint16_t c = MC[m];
        char s[8];
        frame(x, 4, w, 136, C_FRAME);
        switch (id) {                           /* the icons */
        case MAC_CHAOS: { uint32_t k; for (k = 0; k < 40u; k++) { uint32_t ph = k * 0x0D000000u + t * 400000u; int32_t rr = (int32_t)k / 3; cv_pset(cx + icos(ph, rr), 30 + isin(ph, rr), k & 1u ? c : C_TXT); } break; }
        case MAC_SPECTRAL: { int32_t k; for (k = 0; k < 5; k++) cv_rect(cx - 10 + k * 5, 38 - (k == 2 ? 18 : k & 1 ? 13 : 8), 3, k == 2 ? 18 : k & 1 ? 13 : 8, k == 2 ? C_TXT : c); break; }
        case MAC_GRAVITY: {                     /* three orbits round a nucleus */
            uint32_t k, ph;
            for (k = 0; k < 3u; k++)
                for (ph = 0; ph < 64u; ph++) {
                    uint32_t a = ph << 26, rot = k * 0x55555555u;
                    int32_t ex = icos(a, 13), ey = isin(a, 5);
                    cv_pset(cx + ((ex * orb_cos(rot) - ey * orb_sin(rot)) >> 15), 30 + ((ex * orb_sin(rot) + ey * orb_cos(rot)) >> 15), c);
                }
            disc(cx, 30, 3, C_TXT);
            break;
        }
        case MAC_FRACT: { int32_t k; for (k = 0; k < 5; k++) cv_rect(cx - 12 + k * 5, 40 - k * 4, 5, 4, k & 1 ? c : C_TXT); break; }
        default: { circle(cx, 30, 6, C_TXT, 0); { uint32_t ph; for (ph = 0; ph < 64u; ph++) { uint32_t a = ph << 26; cv_pset(cx + icos(a, 15), 30 + isin(a, 5), c); } } break; }
        }
        arc(cx, 76, 15, v * 1024u / 99u, c, C_FDIM);
        disc(cx + icos(0x60000000u + v * (0xC0000000u / 99u), 11), 76 + isin(0x60000000u + v * (0xC0000000u / 99u), 11), 1, C_TXT);
        put_s(put_u(s, v), "%");
        txc(cx, 98, &FONT_S, s, C_TXT, C_BG);
        pill(x + 1, 118, w - 2, 18, c);
        {
            static const char *const SHORT[ORB_NMACRO] = {"CHAOS", "SPECT", "GRAV", "FRACT", "ORBIT"};
            txc(cx, 119, &FONT_S, SHORT[id], C_INK, c);
        }
    }
    cv_blit(0, 24);
}

/* RING: the focused ring, big; each hit's distance from the centre is its degree; the 16 keys' page marked */
static uint32_t step_page;
static void draw_ring(void)
{
    uint32_t r = ring_focus(), len, i;
    const orb_ring_t *g = &sys.ring[r];
    uint16_t c = RINGC[r + 1u];
    int32_t cx = 82, cy = 72;
    len = g->len ? g->len : 1u;
    cv_begin(240, 144, C_BG);
    stars(240, 144, 5, 30);
    circle(cx, cy, 52, dim565(c, 5), 2);
    circle(cx, cy, 30, dim565(c, 3), 4);
    for (i = 0; i < len; i++) {
        uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)i << 32) / len);
        int32_t rad = 52 + (g->on[i] ? orb_clamp(g->dg[i], -7, 7) * 2 : 0);
        int32_t x = cx + icos(ph, rad), y = cy + isin(ph, rad);
        int pg = i / 16u == step_page;
        if (pg) cv_pset(cx + icos(ph, 62), cy + isin(ph, 62), C_LIME);
        if (g->on[i]) {
            cv_line(cx + icos(ph, 30), cy + isin(ph, 30), x, y, dim565(c, 4));
            cv_rect(x - 2, y - 2, 5, 5, g->on[i] == 2u ? C_TXT : c);
        } else
            cv_rect(x - 1, y - 1, 2, 2, dim565(c, 7));
    }
    if (eng.playing) {
        uint32_t ph = 0xC0000000u + (uint32_t)(((uint64_t)eng.rs[r].cur << 32) / len);
        disc(cx + icos(ph, 40), cy + isin(ph, 40), 3, C_TXT);
    }
    disc(cx, cy, 9, C_YEL);
    txc(cx, cy - 8, &FONT_S, ORB_PART_NAME[r + 1u], C_INK, C_YEL);
    {   /* the side: ratio, logic, steps page */
        char s[20], *p;
        frame(160, 6, 78, 132, dim565(c, 9));
        tx(166, 10, &FONT_S, "SPEED", C_GREY, C_BG);
        tx(166, 26, &FONT_S, ORB_RATIO_NAME[g->ratio % ORB_NRATIO], c, C_BG);
        tx(166, 46, &FONT_S, "LOGIC", C_GREY, C_BG);
        if (g->logic == LOG_FREE || g->logic == LOG_CONJ) put_s(s, ORB_LOGIC_NAME[g->logic % ORB_NLOGIC]);
        else { p = put_s(s, ORB_LOGIC_NAME[g->logic % ORB_NLOGIC]); *p++ = ' '; *p++ = (char)('A' + (g->ltgt & 3u)); *p = 0; }
        tx(166, 62, &FONT_S, s, C_TXT, C_BG);
        tx(166, 82, &FONT_S, "KEYS", C_GREY, C_BG);
        p = put_u(s, step_page * 16u + 1u);
        *p++ = '-';
        put_u(p, step_page * 16u + 16u);
        tx(166, 98, &FONT_S, s, C_LIME, C_BG);
        if (g->mute) tx(166, 118, &FONT_S, "MUTED", C_MAG, C_BG);
        else if (rec) tx(166, 118, &FONT_S, "REC", C_MAG, C_BG);
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
        cv_rect(x, 2, 2, 136, C_LIME);
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
    disc(cx + ((ef * R) >> 16) + (((ef * R) >> 15) >> 1), cy, 4, C_YEL);   /* the sun, at the focus */
    {   /* the planet, going round (fast at the near point) */
        uint32_t M = fm1_ms * 1500000u, row = (uint32_t)e >> 11, col = M >> 23;
        uint32_t E = M + ((uint32_t)ORB_KEP[row][col] << 16);
        int32_t b = 32767 - ((ef * ef) >> 16);
        disc(cx + (((orb_cos(E) + ef / 2) * R) >> 15), cy + (((orb_sin(E) * b) >> 15) * R >> 15), 3, C_TXT);
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
            disc(232, y, 3, C_TXT);
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
        disc(px, py, 5, C_LIME);
        disc(px, py, 2, C_INK);
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
    uint32_t k, w = 0;
    int32_t keyoct = (int8_t)set.oct;
    cv_begin(240, 144, C_BG);
    for (k = 0; k < 27u; k++) {
        uint32_t note = 53u + k, white = 0, i;
        int32_t et, p = orb_key_pitch(&sys, sys.keymode == KM_HARMONIC ? k : note, keyoct, &et), dev = p - et;
        int32_t x, kw, ky, kh;
        uint16_t c;
        char s[8];
        for (i = 0; i < 16u; i++) if (WK[i] == k) { white = 1; w = i; }
        if (white) { x = 2 + (int32_t)w * 15; kw = 14; ky = 60; kh = 80; }
        else { x = 2 + (int32_t)w * 15 + 10; kw = 10; ky = 60; kh = 48; }
        c = (note % 12u) == sys.sun % 12u ? C_YEL : white ? C_TXT : C_GREY;
        if (!white) continue;
        cv_rect(x, ky, kw, kh, dim565(c, 12));
        if (sys.keymode == KM_HARMONIC) {
            put_u(s, ORB_WHITE_H[k]);
            txc(x + kw / 2, ky + kh - 18, &FONT_S, s, C_INK, dim565(c, 12));
        } else if (dev) {
            int32_t h = dev / 3;                /* 1/256 semitone: ~1 px a cent */
            cv_rect(x + 3, 40 - (h > 0 ? h : 0), kw - 6, h > 0 ? h : -h, dev > 0 ? C_CYAN : C_MAG);
        }
    }
    w = 0;
    for (k = 0; k < 27u; k++) {                 /* the black keys over them */
        uint32_t note = 53u + k, i, white = 0;
        int32_t et, p = orb_key_pitch(&sys, sys.keymode == KM_HARMONIC ? k : note, keyoct, &et), dev = p - et, x;
        for (i = 0; i < 16u; i++) if (WK[i] == k) { white = 1; w = i; }
        if (white) continue;
        x = 2 + (int32_t)w * 15 + 10;
        cv_rect(x, 60, 10, 48, (note % 12u) == sys.sun % 12u ? C_YEL : C_FDIM);
        if (sys.keymode == KM_HARMONIC) {
            char s[4];
            put_u(s, ORB_WHITE_H[k]);
            tx(x - 2, 86, &FONT_S, s, C_TXT, C_FDIM);
        } else if (dev) {
            int32_t h = dev / 3;
            cv_rect(x + 2, 40 - (h > 0 ? h : 0), 6, h > 0 ? h : -h, dev > 0 ? C_CYAN : C_MAG);
        }
    }
    cv_rect(2, 40, 236, 1, C_FDIM);
    tx(4, 2, &FONT_S, sys.keymode == KM_HARMONIC ? "HARMONICS OF THE SUN" : "CENTS OFF EQUAL TEMPERED", C_GREY, C_BG);
    cv_blit(0, 24);
}

static void draw_main(void)
{
    switch (page) {
    case PG_ORRERY: draw_orrery(); break;
    case PG_MACRO: draw_macros(); break;
    case PG_RING: draw_ring(); break;
    case PG_TIME: draw_time(); break;
    case PG_ORBIT: case PG_BODY: draw_orbit(); break;
    case PG_ENV: draw_env(); break;
    case PG_MOD: draw_mod(); break;
    case PG_LFO: draw_lfo(); break;
    case PG_FILTER: draw_filter(); break;
    case PG_SPACE: draw_space(); break;
    case PG_PAD: draw_pad(); break;
    default: draw_keys_page(); break;
    }
}
static int page_animated(void)
{
    return page == PG_ORRERY || page == PG_MACRO || page == PG_RING || page == PG_TIME || page == PG_ORBIT ||
           page == PG_BODY || page == PG_MOD || page == PG_LFO || page == PG_PAD;
}

static void splash(void)
{
    uint32_t t0 = fm1_ms, f;
    lcd_fill(0, 0, 240, 240, C_BG);
    for (f = 0; fm1_ms - t0 < 1600u && f < 400u; f++) {
        uint32_t t = fm1_ms - t0, k, t1 = fm1_ms;
        cv_begin(240, 144, C_BG);
        stars(240, 144, 1, 60);
        disc(120, 56, 22, C_VIO);
        disc(114, 50, 8, mix565(C_VIO, C_TXT, 6));
        for (k = 0; k < 128u; k++) {            /* the ring, tilted, in front below and behind above */
            uint32_t ph = k << 25;
            int32_t x = 120 + icos(ph, 40), y = 56 + isin(ph, 9) - icos(ph, 4);
            if (isin(ph, 9) > 0 || (x - 120) * (x - 120) + (y - 56) * (y - 56) > 22 * 22)
                cv_rect(x, y, 2, 2, k & 1u ? C_YEL : C_MAG);
        }
        {
            uint32_t ph = t * 4000000u;
            disc(120 + icos(ph, 56), 56 + isin(ph, 14), 3, C_CYAN);
        }
        txc(120, 92, &FONT_L, "ORBITRIX", C_LIME, C_BG);
        cv_rect(64, 126, 112, 2, C_MAG);
        cv_blit(0, 30);
        if (f == 0u) {
            cv_begin(240, 40, C_BG);
            txc(120, 2, &FONT_S, "AN ORBITAL GROOVE ENGINE", C_GREY, C_BG);
            txc(120, 22, &FONT_S, "v" APP_VER, C_DIMT, C_BG);
            cv_blit(0, 184);
        }
        do platform_poll(); while (fm1_ms - t1 < 30u);   /* ~30 frames a second */
    }
}

/* ---- MIDI in (the main loop, and app_idle while the screen draws) */
static uint32_t idle_ok, clk_n, clk_t0, clk_bpm, clk_run, clk_last;
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
                                                                         : orb_key_pitch(&sys, d1, 0, 0);
            orb_note_on(&eng, (uint8_t)(128u + d1), p, d2);
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

static void goto_page(uint32_t a, uint32_t b)   /* a button with two pages: the first, then between them */
{
    page = page == a ? b : page == b ? a : a;
    main_dirty = knobs_dirty = hint_dirty = 1;
    drawn_bar = ~0u;
}

/* REC: a key played into the focused ring, at its step (playing) or the next (stopped) */
static uint32_t rec_pos;
static void rec_key(uint32_t note)
{
    uint32_t r = ring_focus(), t = sys.tuning % ORB_NTUNING, n = ORB_SCALE_N[t], i, best = 0;
    orb_ring_t *g = &sys.ring[r];
    int32_t semi = ((int32_t)note - 36 - (int32_t)sys.sun - 12 * sys.snd[r + 1u].oct), oct, pos, bd = 1 << 30;
    oct = semi >= 0 ? semi / 12 : -((11 - semi) / 12);
    pos = semi - oct * 12;
    for (i = 0; i < n; i++) {                   /* the nearest degree */
        int32_t d = ORB_SCALE[t][i] - pos * 256;
        if (d < 0) d = -d;
        if (d < bd) { bd = d; best = i; }
    }
    {
        uint32_t len = g->len ? g->len : 1u, at = eng.playing ? eng.rs[r].cur : rec_pos % len;
        int32_t deg = oct * (int32_t)n + (int32_t)best - g->deg;
        g->on[at] = g->on[at] ? g->on[at] : 1u;
        g->dg[at] = (int8_t)orb_clamp(deg, -14, 21);
        if (!eng.playing) rec_pos = (at + 1u) % len;
    }
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
            if (page == PG_RING) {              /* the steps */
                int32_t wi = white_index(k);
                held_id[k] = 0;
                if (wi >= 0) { held_step = step_page * 16u + (uint32_t)wi; held_used = 0; }
                continue;
            }
            {
                int32_t p = orb_key_pitch(&sys, sys.keymode == KM_HARMONIC ? k : note, oct, 0);
                held_id[k] = (uint8_t)(k + 1u);
                orb_note_on(&eng, held_id[k], p, 100);
                midi_send(0x90u | (set.ch ? set.ch - 1u : 0u), note & 127u, 100);
                if (rec) rec_key(note);
            }
        }
        if (up >> k & 1u) {
            if (page == PG_RING && held_step != 0xFFu && white_index(k) >= 0 &&
                held_step == step_page * 16u + (uint32_t)white_index(k)) {
                orb_ring_t *g = &sys.ring[ring_focus()];
                if (!held_used && held_step < g->len) {   /* a tap: rest -> hit -> accent -> rest */
                    g->on[held_step] = (uint8_t)((g->on[held_step] + 1u) % 3u);
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
                g->dg[held_step] = (int8_t)orb_clamp(g->dg[held_step] + e, -14, 21);
                if (!g->on[held_step]) g->on[held_step] = 1;
                held_used = 1;
                {
                    char s[24], *p = put_s(s, "STEP DEG ");
                    put_deg(p, g->deg + g->dg[held_step]);
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
    if (isel() != sel_was) { sel_was = isel(); knobs_dirty = 1; }
    if (rel & (1u << BTN_SEL)) knobs_dirty = 1;
    if (!b) return;
    if (save_mode && !(b & (1u << BTN_SAVE)) && !(b & (1u << BTN_SEL))) { save_mode = 0; say("NOT SAVED"); drawn_bar = ~0u; }
    if (b & (1u << BTN_HOME)) goto_page(PG_ORRERY, PG_MACRO);
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
    if (b & (1u << BTN_EDIT)) goto_page(PG_ORBIT, PG_BODY);
    if (b & (1u << BTN_ENV)) goto_page(PG_ENV, PG_ENV);
    if (b & (1u << BTN_LFO)) goto_page(PG_MOD, PG_LFO);
    if (b & (1u << BTN_FX)) goto_page(PG_FILTER, PG_SPACE);
    if (b & (1u << BTN_GLO)) goto_page(PG_PAD, PG_PAD);
    if (b & (1u << BTN_ARP)) goto_page(PG_KEYS, PG_KEYS);
    if (b & (1u << BTN_PLAY)) { orb_play(&eng, !eng.playing); drawn_bar = ~0u; }
    if (b & (1u << BTN_REC)) { rec = !rec; rec_pos = 0; say(rec ? "REC: KEYS INTO THE RING" : "REC OFF"); drawn_bar = ~0u; main_dirty = 1; }
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
            if (blob_save(save_to, &sys, sizeof sys) == 0) { sysi = ORB_NSYSTEM + save_to; set.sysi = (uint8_t)sysi; set_touch(); say("SAVED"); }
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
    fm1_led_key(BTN_HOME, page == PG_ORRERY || page == PG_MACRO);
    fm1_led_key(BTN_SEQ, page == PG_RING || page == PG_TIME);
    fm1_led_key(BTN_EDIT, page == PG_ORBIT || page == PG_BODY);
    fm1_led_key(BTN_ENV, page == PG_ENV);
    fm1_led_key(BTN_LFO, page == PG_MOD || page == PG_LFO);
    fm1_led_key(BTN_FX, page == PG_FILTER || page == PG_SPACE);
    fm1_led_key(BTN_GLO, page == PG_PAD);
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
        for (k = 0; k < 27u; k++) if (white_index(k) < 0) { fm1_led_key(KEY_LED(k), 0); fm1_led_key_glow(KEY_LED(k), 0); }
        return;
    }
    for (k = ORB_NV - ORB_NKV; k < ORB_NV; k++)
        if (eng.v[k].on && eng.v[k].id >= 1u && eng.v[k].id <= 27u) lit |= 1u << (eng.v[k].id - 1u);
    for (k = 0; k < 27u; k++) {
        uint32_t note = (uint32_t)(53 + (int32_t)k + 12 * oct);
        fm1_led_key(KEY_LED(k), lit >> k & 1u);
        fm1_led_key_glow(KEY_LED(k), sys.keymode == KM_HARMONIC ? white_index(k) == 0 : note % 12u == sys.sun % 12u);
    }
}

static void app_main(void)
{
    uint32_t r, laps[ORB_NRING] = {0}, mrng = 0x1234567u, t_hint = 0, peak_t = 0;
    if (settings_load(&set, sizeof set) || (int8_t)set.oct < -3 || (int8_t)set.oct > 3 || set.ch > 16u || set.sysi >= NSYS)
        set.oct = 0, set.ch = 0, set.sysi = 0;
    orb_system(0, &sys);
    orb_init(&eng, &sys, &orb_mem);
    if (set.sysi) system_load(set.sysi);
    set_dirty = 0;
    splash();
    lcd_fill(0, 0, 240, 240, C_BG);
    for (;;) {
        platform_poll();
        handle_buttons();
        handle_encoders();
        handle_keys();
        midi_in();
        if (clk_run && fm1_ms - clk_last > 2000u) clk_run = 0;
        for (r = 0; r < ORB_NRING; r++)         /* MUTATE: at each ring's lap */
            if (eng.rs[r].laps != laps[r]) {
                uint32_t amt = sys.mutate + sys.macro[MAC_CHAOS] / 2u;
                laps[r] = eng.rs[r].laps;
                orb_mutate(&sys.ring[r], amt > 99u ? 99u : amt, sys.var, &mrng);
                if (amt) main_dirty = 1;
            }
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
}
