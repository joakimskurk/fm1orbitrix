/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX's built-in systems (docs/orbitrix/PLAN.md §9): named after real resonances in the sky. Each is built
 * here from a handful of sound archetypes and ring descriptions; orb_system(i, sys) writes system i. */
#ifndef ORB_SYSTEMS_H
#define ORB_SYSTEMS_H
#include "orb_engine.h"

/* ---- the sound library: what SOUND picks by name (EDIT's first page). Each sound knows its number (.lib), so the app
 * can say what a part plays and what kind it is (its icon; what IDEA writes for it). The systems are made of them */
enum { LIB_NONE, LIB_BOOM, LIB_THUMP, LIB_TOM, LIB_PAT, LIB_CLAP, LIB_TICK, LIB_TSSS, LIB_ZAP, LIB_BOOP, LIB_SQUEAK,
       LIB_BLOOP, LIB_PURR, LIB_GROWL, LIB_SUB, LIB_SWOOP, LIB_PLUCK, LIB_PAW, LIB_GLASS, LIB_BELL, LIB_STAR, LIB_TIDAL,
       LIB_TOY, LIB_PIXEL, LIB_LEAD, LIB_FLUTE, LIB_MEOW, LIB_MEW, LIB_CLOUD, LIB_DRONE, LIB_COMET, ORB_NLIB };
enum { LK_KICK, LK_SNARE, LK_HAT, LK_PERC, LK_BASS, LK_KEYS, LK_PAD, LK_LEAD, LK_VOICE, ORB_NLK };   /* the kinds */
static const char *const ORB_LK_NAME[ORB_NLK] = {"KICK", "SNARE", "HAT", "PERC", "BASS", "MELODY", "PAD", "LEAD", "VOICE"};

/* ---- the sound archetypes (times 0..99: 0 = 1 ms, 30 = 20 ms, 50 = 150 ms, 60 = 0.4 s, 70 = 1.1 s, 80 = 3 s) */
static const orb_snd_t SND_KICK = {.ecc = 22, .sling = 30, .ang = 0, .spr = 0, .fall = 52, .fallt = 36, .a = 0, .d = 50,
    .s = 0, .r = 40, .eamt = 55, .ea = 0, .ed = 30, .vsens = 50, .level = 96, .pan = 50, .send = 0, .gate = 50, .oct = -1,
    .flags = SF_PHASE, .lib = LIB_BOOM};
static const orb_snd_t SND_SNARE = {.ecc = 30, .sling = 20, .ang = 20, .spr = 30, .dust = 62, .color = 74, .fall = 22,
    .fallt = 28, .a = 0, .d = 42, .s = 0, .r = 42, .eamt = 30, .ea = 0, .ed = 25, .vsens = 60, .level = 82, .pan = 50,
    .send = 28, .gate = 40, .oct = 1, .flags = SF_PHASE, .lib = LIB_PAT};
static const orb_snd_t SND_HAT = {.ecc = 10, .dust = 99, .color = 90, .a = 0, .d = 24, .s = 0, .r = 26, .vsens = 70,
    .level = 78, .pan = 58, .spr = 60, .send = 10, .gate = 20, .oct = 2, .lib = LIB_TICK};
static const orb_snd_t SND_BASS = {.ecc = 52, .sling = 38, .ang = 10, .spr = 10, .prec = 0, .a = 2, .d = 52, .s = 55,
    .r = 30, .eamt = 30, .ea = 0, .ed = 46, .vsens = 40, .level = 84, .pan = 50, .send = 6, .gate = 55, .oct = 0,
    .flags = SF_LEGATO, .tide = 30, .lib = LIB_PURR};
static const orb_snd_t SND_GLASS = {.ecc = 16, .moon = 38, .mratio = 2, .ang = 60, .spr = 70, .prec = 18, .a = 1, .d = 60,
    .s = 15, .r = 60, .eamt = 25, .ea = 0, .ed = 50, .vsens = 50, .level = 64, .pan = 50, .send = 55, .gate = 50, .oct = 2, .tide = 50, .lib = LIB_GLASS};
static const orb_snd_t SND_PLUCK = {.ecc = 40, .sling = 22, .ang = 30, .spr = 45, .prec = 10, .a = 0, .d = 56, .s = 10,
    .r = 52, .eamt = 40, .ea = 0, .ed = 44, .vsens = 50, .level = 72, .pan = 50, .send = 40, .gate = 50, .oct = 1, .tide = 50, .lib = LIB_PLUCK};
static const orb_snd_t SND_BELL = {.ecc = 18, .moon = 58, .mratio = 13, .ang = 45, .spr = 75, .prec = 14, .a = 0, .d = 66,
    .s = 0, .r = 66, .eamt = 40, .ea = 0, .ed = 56, .vsens = 50, .level = 60, .pan = 50, .send = 62, .gate = 50, .oct = 2, .tide = 55, .lib = LIB_BELL};
static const orb_snd_t SND_PAD = {.ecc = 30, .sling = 10, .moon = 22, .mratio = 1, .ang = 50, .spr = 82, .prec = 30,
    .a = 60, .d = 70, .s = 70, .r = 74, .eamt = 22, .ea = 62, .ed = 72, .vsens = 30, .level = 66, .pan = 50, .send = 70,
    .gate = 95, .oct = 1, .tide = 50, .lib = LIB_CLOUD};
static const orb_snd_t SND_PIXEL = {.ecc = 58, .sling = 30, .pixel = 66, .ang = 20, .spr = 40, .a = 0, .d = 38, .s = 0,
    .r = 32, .eamt = 20, .ea = 0, .ed = 30, .vsens = 50, .level = 60, .pan = 50, .send = 25, .gate = 40, .oct = 2, .tide = 40, .lib = LIB_PIXEL};
static const orb_snd_t SND_LEAD = {.ecc = 44, .sling = 26, .ang = 40, .spr = 50, .prec = 8, .a = 4, .d = 56, .s = 62,
    .r = 50, .eamt = 24, .ea = 0, .ed = 50, .vsens = 50, .level = 74, .pan = 50, .send = 42, .gate = 80, .oct = 0, .tide = 45, .lib = LIB_LEAD};
static const orb_snd_t SND_FLUTE = {.ecc = 8, .sling = 0, .ang = 70, .spr = 40, .prec = 6, .dust = 14, .color = 30,
    .a = 22, .d = 60, .s = 80, .r = 52, .eamt = 12, .ea = 30, .ed = 60, .vsens = 40, .level = 70, .pan = 50, .send = 50,
    .gate = 90, .oct = 0, .tide = 40, .lib = LIB_FLUTE};


/* the library's own (the rest are the archetypes above, and the legend's below) */
static const orb_snd_t SND_THUMP = {.ecc = 14, .sling = 20, .fall = 64, .fallt = 42, .a = 0, .d = 56, .s = 0, .r = 46,
    .eamt = 40, .ed = 34, .vsens = 50, .level = 98, .pan = 50, .gate = 60, .oct = -1, .flags = SF_PHASE, .lib = LIB_THUMP};
static const orb_snd_t SND_TOM = {.ecc = 26, .sling = 24, .ang = 10, .spr = 20, .fall = 34, .fallt = 38, .a = 0, .d = 48,
    .s = 0, .r = 44, .eamt = 35, .ed = 30, .vsens = 55, .level = 84, .pan = 50, .send = 18, .gate = 50, .oct = 0,
    .flags = SF_PHASE, .lib = LIB_TOM};
static const orb_snd_t SND_CLAP = {.ecc = 20, .sling = 10, .ang = 30, .spr = 60, .dust = 84, .color = 62, .fall = 10,
    .fallt = 20, .a = 0, .d = 38, .s = 0, .r = 40, .eamt = 10, .ed = 20, .vsens = 60, .level = 80, .pan = 50, .send = 36,
    .gate = 30, .oct = 1, .flags = SF_PHASE, .lib = LIB_CLAP};
static const orb_snd_t SND_TSSS = {.ecc = 8, .dust = 99, .color = 92, .a = 0, .d = 44, .s = 0, .r = 46, .vsens = 70,
    .level = 88, .pan = 44, .spr = 70, .send = 16, .gate = 30, .oct = 2, .lib = LIB_TSSS};
static const orb_snd_t SND_ZAP = {.ecc = 60, .sling = 40, .pixel = 80, .fall = 70, .fallt = 30, .a = 0, .d = 34, .s = 0,
    .r = 30, .eamt = 30, .ed = 26, .vsens = 50, .level = 60, .pan = 56, .send = 22, .gate = 30, .oct = 2, .flags = SF_PHASE,
    .lib = LIB_ZAP};
static const orb_snd_t SND_GROWL = {.ecc = 56, .sling = 55, .ang = 12, .spr = 10, .a = 1, .d = 50, .s = 40, .r = 30,
    .eamt = 40, .ed = 40, .vsens = 45, .level = 82, .pan = 50, .send = 6, .gate = 50, .oct = 0, .flags = SF_LEGATO,
    .tide = 30, .lib = LIB_GROWL};
static const orb_snd_t SND_SUB = {.ecc = 6, .a = 2, .d = 60, .s = 70, .r = 30, .eamt = 12, .ed = 40, .vsens = 30,
    .level = 92, .pan = 50, .gate = 70, .oct = 0, .flags = SF_LEGATO, .lib = LIB_SUB};
static const orb_snd_t SND_SWOOP = {.ecc = 52, .sling = 38, .ang = 10, .spr = 10, .a = 2, .d = 64, .s = 0, .r = 56,
    .eamt = 50, .ed = 50, .vsens = 40, .level = 86, .pan = 50, .send = 10, .gate = 90, .oct = 0, .insp = 50, .inspt = 52,
    .lib = LIB_SWOOP};
static const orb_snd_t SND_PAW = {.ecc = 22, .sling = 6, .ang = 25, .spr = 40, .a = 0, .d = 52, .s = 0, .r = 50,
    .eamt = 36, .ed = 36, .vsens = 55, .level = 78, .pan = 50, .send = 34, .gate = 50, .oct = 1, .tide = 20,
    .lib = LIB_PAW};   /* (no SF_PHASE: a chord retriggering a ringing voice would click) */
static const orb_snd_t SND_STAR = {.ecc = 12, .moon = 46, .mratio = 2, .ang = 50, .spr = 70, .prec = 10, .a = 0, .d = 62,
    .s = 0, .r = 62, .eamt = 30, .ed = 50, .vsens = 50, .level = 62, .pan = 50, .send = 60, .gate = 50, .oct = 3, .tide = 30,
    .lib = LIB_STAR};
/* the cute ones: a boop (a soft round blip falling onto its note), a toy's squeak (a quick chirp up), a bubble's bloop
 * (from far under, up), a toy piano (a tine, a little clang), a mew (her voice in short: it falls from above as her
 * mouth opens and closes) */
static const orb_snd_t SND_BOOP = {.ecc = 4, .spr = 20, .fall = 20, .fallt = 30, .a = 0, .d = 46, .s = 0, .r = 42,
    .eamt = 10, .ed = 30, .vsens = 50, .level = 86, .pan = 50, .send = 30, .gate = 40, .oct = 2, .lib = LIB_BOOP};
static const orb_snd_t SND_SQUEAK = {.ecc = 20, .sling = 20, .ang = 30, .spr = 30, .insp = 26, .inspt = 24, .a = 0,
    .d = 40, .s = 0, .r = 36, .eamt = 30, .ed = 30, .vsens = 50, .level = 90, .pan = 50, .send = 30, .gate = 30, .oct = 3,
    .lib = LIB_SQUEAK};
static const orb_snd_t SND_BLOOP = {.ecc = 8, .spr = 30, .insp = 40, .inspt = 34, .a = 0, .d = 42, .s = 0, .r = 40,
    .eamt = 20, .ed = 30, .vsens = 50, .level = 94, .pan = 50, .send = 40, .gate = 40, .oct = 3, .lib = LIB_BLOOP};
static const orb_snd_t SND_TOY = {.ecc = 34, .sling = 10, .moon = 30, .mratio = 13, .ang = 30, .spr = 40, .a = 0, .d = 54,
    .s = 0, .r = 50, .eamt = 44, .ed = 34, .vsens = 55, .level = 72, .pan = 50, .send = 30, .gate = 50, .oct = 2,
    .lib = LIB_TOY};
static const orb_snd_t SND_MEW = {.ecc = 12, .sling = 22, .ang = 35, .spr = 40, .moon = 20, .mratio = 1, .dust = 8,
    .color = 40, .fall = 24, .fallt = 52, .a = 14, .d = 56, .s = 30, .r = 48, .eamt = 70, .ea = 30, .ed = 50, .vsens = 20,
    .level = 76, .pan = 50, .send = 50, .gate = 60, .oct = 0, .lib = LIB_MEW};
static const orb_snd_t SND_TIDAL = {.ecc = 20, .moon = 20, .mratio = 2, .ang = 55, .spr = 70, .prec = 12, .a = 1, .d = 62,
    .s = 20, .r = 60, .eamt = 30, .ed = 50, .vsens = 50, .level = 66, .pan = 50, .send = 50, .gate = 60, .oct = 1,
    .tide = 95, .lib = LIB_TIDAL};

static uint8_t orb_ratio_idx(uint32_t p, uint32_t q)
{
    uint32_t i;
    for (i = 0; i < ORB_NRATIO; i++)
        if (ORB_RATIO_P[i] == p && ORB_RATIO_Q[i] == q) return (uint8_t)i;
    return ORB_RATIO_1_1;
}

/* a ring: speed p:q, LEN, HITS, ROT, its degree; then each hit's degree from mel (cycled), accents on acc's hits
 * (bit n: the n-th hit) */
typedef struct {
    uint8_t p, q, len, hits, rot;
    int8_t deg;
    uint8_t dir, logic, ltgt, prob;
    const int8_t *mel;
    uint8_t nmel;
    uint32_t acc;
} orb_rdef_t;

static void orb_ring_set(orb_ring_t *r, const orb_rdef_t *d)
{
    uint32_t i, h = 0;
    r->ratio = orb_ratio_idx(d->p, d->q);
    r->len = d->len;
    r->hits = d->hits;
    r->rot = d->rot;
    r->deg = d->deg;
    r->dir = d->dir;
    r->logic = d->logic;
    r->ltgt = d->ltgt;
    r->prob = d->prob ? d->prob : 99;
    r->mute = 0;
    for (i = 0; i < ORB_NSTEP; i++) { r->on[i] = 0; r->dg[i] = 0; }
    orb_euclid(r);
    for (i = 0; i < r->len; i++)
        if (r->on[i]) {
            if (d->nmel) r->dg[i] = d->mel[h % d->nmel];
            if (d->acc >> (h % 32u) & 1u) r->on[i] = 2;
            h++;
        }
}

static void orb_sys_clear(orb_sys_t *s, const char *name)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)s;
    for (i = 0; i < sizeof *s; i++) p[i] = 0;
    for (i = 0; i < 11u && name[i]; i++) s->name[i] = name[i];
    s->bpm = 112;
    s->cut = 99; s->res = 0; s->fmode = 0;
    s->dtime = 6; s->dfb = 35; s->dtone = 60; s->dmix = 40;
    s->rsize = 60; s->rdecay = 60; s->rdamp = 40; s->rmix = 45;
    s->ox = 50; s->oy = 50; s->orad = 20; s->orate = 4;
    s->lrate[0] = 30; s->lrate[1] = 12; s->lshape[1] = LS_DRIFT;
    s->macro[MAC_GRAVITY] = 40;
    s->hgrain = 50; s->hretro = 0; s->hshift = -12;
    s->bg_prog = 2; s->bg_change = 1; s->bg_glow = 20; s->bg_silk = 40; s->bg_width = 55; s->bg_halo = 70;   /* (LEVEL 0: up, and it plays) */
}

/* a ring written out by hand: speed p:q, its steps (x a hit, X an accent, . a rest; spaces and | to read by), each
 * hit's degree from mel (cycled), its probability (0: always). LEN and HITS are what the steps say (HITS turned later
 * spreads them again, as ever) */
static void orb_ring_pat(orb_ring_t *r, uint32_t p, uint32_t q, const char *pat, const int8_t *mel, uint32_t nmel,
                         uint8_t prob)
{
    uint32_t i, n = 0, h = 0;
    r->ratio = orb_ratio_idx(p, q);
    r->rot = 0; r->deg = 0; r->dir = DIR_FWD; r->logic = LOG_FREE; r->ltgt = 0; r->mute = 0;
    r->prob = prob ? prob : 99;
    for (i = 0; i < ORB_NSTEP; i++) { r->on[i] = 0; r->dg[i] = 0; }
    for (i = 0; pat[i] && n < ORB_NSTEP; i++) {
        if (pat[i] == ' ' || pat[i] == '|') continue;
        if (pat[i] == 'x' || pat[i] == 'X') {
            r->on[n] = pat[i] == 'X' ? 2 : 1;
            if (nmel) r->dg[n] = mel[h % nmel];
            h++;
        }
        n++;
    }
    r->len = (uint8_t)(n ? n : 1u);
    r->hits = (uint8_t)h;
}

static void orb_mod_set(orb_sys_t *s, uint32_t k, uint8_t src, uint8_t dst, int8_t amt)
{
    s->mod[k].src = src; s->mod[k].dst = dst; s->mod[k].amt = amt;
}

static void orb_hush(orb_snd_t *d);       /* (the drums whisper: after the library) */

#define ORB_NSYSTEM 20u
static const char *const ORB_SYSTEM_NAME[ORB_NSYSTEM] = {"KITTEN", "MILKY WAY", "HARMONICES", "PURRSEC", "ZOOMIES",
    "CATWALK", "SPIRAL", "LUNA NOVA", "SGR A*", "BOOP BOP", "SNOWGLOBE", "STARDUST", "ALPHA CEN", "MOON NAP", "AFTERGLOW",
    "PULSAR", "ASTRALICA", "NOM NOM", "TUMMY", "MULTIVERSE"};

/* (the made ones: a library sound on a part, centred, at a level, never sliding; a ring that FOLLOWs the chords) */
static void orb_lib_load(orb_snd_t *s, uint32_t li, uint32_t part);
static void orb_put(orb_sys_t *s, uint32_t part, uint32_t li, uint8_t level)
{
    s->snd[part].pan = 50;
    orb_lib_load(&s->snd[part], li, part);
    if (level) s->snd[part].level = level;
    s->snd[part].flags &= (uint8_t)~SF_LEGATO;
}
static void orb_ring_fol(orb_ring_t *r, uint32_t p, uint32_t q, const char *pat, const int8_t *mel, uint32_t nmel,
                         uint8_t prob)
{
    orb_ring_pat(r, p, q, pat, mel, nmel, prob);
    r->follow = 1;
}

/* KITTEN: a beat to start from, on the grid (all 1:1, so every step is where you look for it): a kick on the beat, a
 * clap's backbeat, hats in 8ths, a purring bass on the root, its fifth and 7/4; the keys a soft paw. Nothing mutates:
 * it is yours to change (STEPS: the keys; IDEA; SOUND) */
static void orb_system_kitten(orb_sys_t *s)
{
    static const int8_t ma[] = {0, 7, 4, 7}, md[] = {0, 0, 7, 0, 10, 12};
    orb_rdef_t A = {1, 1, 16, 4, 0, 0, DIR_FWD, 0, 0, 0, ma, 4, 0x1};
    orb_rdef_t B = {1, 1, 16, 2, 4, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x3};
    orb_rdef_t C = {1, 1, 16, 8, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0xAA};
    orb_rdef_t D = {1, 1, 16, 6, 0, 0, DIR_FWD, 0, 0, 0, md, 6, 0x1};
    s->tuning = 0; s->sun = 5; s->bpm = 104; s->swing = 16;
    s->bg_prog = 0;                         /* (its bass sings 7/4: under it, the sun's own chord fits) */
    s->snd[0] = SND_PAW; s->snd[0].oct = 0; s->snd[0].level = 74;
    orb_put(s, ORB_A, LIB_BOOP, 60);        /* (the beat a boop: root, fifth, third, fifth) */
    s->snd[2] = SND_CLAP;
    s->snd[3] = SND_HAT;
    s->snd[4] = SND_BASS; s->snd[4].oct = 0; s->snd[4].gate = 60; s->snd[4].level = 68;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->var = 20;
    s->dtime = 6; s->dfb = 30; s->dmix = 22; s->rmix = 30; s->rsize = 55; s->rdecay = 66;
}

/* MILKY WAY: a lullaby. A music box twinkling at 3:2 over pitched boops, a soft kick on 1 and 3, a purring bass at
 * half speed, all pentatonic (nothing can be wrong), a little swing; where the orbits meet the moons twinkle; the keys
 * mew */
static void orb_system_milky(orb_sys_t *s)
{
    static const int8_t ma[] = {0, 3}, mb[] = {4, 2, 3, 4, 1}, mc[] = {5, 7, 6, 9, 7, 5, 4}, md[] = {0, -2, 3};
    orb_rdef_t A = {1, 1, 16, 2, 0, 0, DIR_FWD, 0, 0, 0, ma, 2, 0x1};
    orb_rdef_t B = {1, 1, 16, 5, 2, 0, DIR_FWD, 0, 0, 85, mb, 5, 0x1};
    orb_rdef_t C = {3, 2, 24, 7, 0, 0, DIR_FWD, 0, 0, 90, mc, 7, 0x1};
    orb_rdef_t D = {1, 2, 8, 3, 0, 0, DIR_FWD, 0, 0, 0, md, 3, 0x1};
    s->tuning = 3; s->sun = 7; s->bpm = 86; s->swing = 22;
    s->snd[0] = SND_MEW; s->snd[0].level = 58;   /* (a chord of meows adds up) */
    orb_put(s, ORB_A, LIB_GLASS, 44); s->snd[ORB_A].oct = 1;   /* (one and three: glass) */
    s->snd[2] = SND_BOOP; s->snd[2].level = 72;
    s->snd[3] = SND_STAR; s->snd[3].oct = 2; s->snd[3].level = 70;
    s->snd[4] = SND_BASS; s->snd[4].oct = 0; s->snd[4].level = 66; s->snd[4].gate = 80;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->mutate = 10; s->var = 15;
    s->dtime = 7; s->dfb = 45; s->ping = 1; s->dmix = 32; s->rmix = 55; s->rsize = 85; s->rdecay = 82;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 30);
    s->bg_level = 45; s->bg_prog = 2; s->bg_change = 1; s->bg_glow = 20; s->bg_silk = 45; s->bg_cut = 30;   /* under it all, */
    s->bg_halo = 65; s->bloom = 30;                                                                  /* all wet, ~16 s */
}

/* ---- the set: each its own kind of music, tempo, sounds and space. In every one with chords the pitched rings
 * FOLLOW them (their notes are the chord's: 0 its bass, 1 the next note up ...), so the bass and the melodies change
 * with the chords on the bar, and whatever the CHORDS are turned to, they stay in them; the keys are CHORD keys */

/* HARMONICES: 4, 5, 6, 7 hits a bar on the 4:5:6:7 chord, in the overtone scale (OTONAL): the idea itself */
static void orb_system_harmonices(orb_sys_t *s)
{   /* HARMONICES: 4, 5, 6, 7 hits a bar on the 4:5:6:7 chord, in the overtone scale (OTONAL) */
        static const int8_t mb[] = {0, 1, 0, -1, 2}, mc[] = {0, 0, 2, 0, -2, 1}, md[] = {0, 2, 0, 4, 0, -1, 1};
        orb_rdef_t A = {1, 1, 16, 4, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
        orb_rdef_t B = {5, 4, 20, 5, 0, 2, DIR_FWD, 0, 0, 0, mb, 5, 0};
        orb_rdef_t C = {3, 2, 24, 6, 0, 4, DIR_FWD, 0, 0, 0, mc, 6, 0};
        orb_rdef_t D = {7, 4, 28, 7, 0, 6, DIR_FWD, 0, 0, 90, md, 7, 0};
        s->tuning = 1; s->sun = 0; s->bpm = 96;
        s->snd[0] = SND_PAD;
        s->snd[1] = SND_BASS; s->snd[1].oct = 0; s->snd[1].gate = 70;
        s->snd[2] = SND_GLASS;
        s->snd[3] = SND_PLUCK;
        s->snd[4] = SND_BELL; s->snd[4].level = 54;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->mutate = 12; s->var = 20;
        s->dtime = 6; s->dfb = 40; s->ping = 1; s->dmix = 36; s->rmix = 50; s->rsize = 70; s->rdecay = 76;
        s->macro[MAC_GRAVITY] = 40; s->macro[MAC_ORBIT] = 15;
        orb_mod_set(s, 0, SRC_ORBX, DST_ECC, 30);
        orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 40);
        orb_mod_set(s, 2, SRC_CONJ, DST_MOON, 35);
        orb_mod_set(s, 3, SRC_CONJ, DST_TIDE, 30);          /* where the orbits meet, they tug at each other */
        }

/* PURRSEC: lo-fi in a cat cafe, 84, swung: a soft thump, a brushed pat, ticking hats that come and go, the purr on
 * the chords' bass (the royal road: Gmaj7 A6 F#m7 Bm9 on D), a little warmth, the chords dark and short; a toy piano */
static void orb_system_purrsec(orb_sys_t *s)
{
    static const int8_t keys[] = {3, 4, 2}, bass[] = {0, 0, -1, 0, 1};
    s->tuning = 0; s->sun = 2; s->bpm = 84; s->swing = 30; s->hum = 6; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_TOY, 66);
    orb_put(s, ORB_A, LIB_PAW, 52);
    orb_put(s, ORB_B, LIB_PAT, 80); s->snd[ORB_B].dust = 70; s->snd[ORB_B].color = 62;
    orb_put(s, ORB_C, LIB_TICK, 64); s->snd[ORB_C].pan = 62;
    orb_put(s, ORB_D, LIB_PURR, 52); s->snd[ORB_D].gate = 70;
    orb_ring_fol(&s->ring[0], 1, 1, "X......x ..x.....", keys, 3, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "....X... ....X..x", 0, 0, 0);
    orb_ring_pat(&s->ring[2], 1, 1, "x.xX x.x. x.xX x.xx", 0, 0, 82);
    orb_ring_fol(&s->ring[3], 1, 2, "X..x..x. x...x...", bass, 5, 0);
    s->drive = 18;
    s->dtime = 6; s->dfb = 30; s->ping = 1; s->dmix = 18; s->rmix = 22; s->rsize = 50; s->rdecay = 66;
    s->bg_level = 30; s->bg_prog = 4; s->bg_change = 1; s->bg_halo = 35; s->bg_silk = 62; s->bg_width = 50;
    s->bg_cut = 35; s->bg_glow = 10;
    orb_mod_set(s, 0, SRC_LFO2, DST_ANG, 20);
}

/* ZOOMIES: a kitten's mad minute, house at 124: four on the floor, a clap, a squeak chasing at 3:2, a growl bouncing on
 * the chords' bass and fifth, a chord a bar (Gadd9 Em7 Cadd9 D6), low; a pixel lead */
static void orb_system_zoomies(orb_sys_t *s)
{
    static const int8_t sq[] = {2, 3, 4, 3, 2, 1, 2, 4, 5, 4, 3, 2}, bass[] = {0, 2, 0, 2, 0, 1, 0, 2}, boop[] = {2, 3, 4, 3};
    s->tuning = 0; s->sun = 7; s->bpm = 124; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_PIXEL, 62);
    orb_put(s, ORB_A, LIB_BOOP, 58);
    orb_put(s, ORB_B, LIB_CLAP, 99);
    orb_put(s, ORB_C, LIB_SQUEAK, 84); s->snd[ORB_C].oct = 2; s->snd[ORB_C].send = 34;
    orb_put(s, ORB_D, LIB_GROWL, 72); s->snd[ORB_D].gate = 45; s->snd[ORB_D].a = 5;
    orb_ring_fol(&s->ring[0], 1, 1, "X...x...X...x...", boop, 4, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "....X.......X...", 0, 0, 0);
    orb_ring_fol(&s->ring[2], 3, 2, "x.xX..x.x..X x.x..x.X..x.", sq, 12, 85);
    orb_ring_fol(&s->ring[3], 1, 1, "x.x.x.x.x.x.x.x.", bass, 8, 0);
    s->dtime = 5; s->dfb = 26; s->ping = 1; s->dmix = 18; s->rmix = 22; s->rsize = 50; s->rdecay = 62;
    s->bg_level = 40; s->bg_prog = 2; s->bg_change = 0; s->bg_halo = 40; s->bg_silk = 40; s->bg_width = 55;
    s->bg_cut = 40; s->bg_glow = 8;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);
    orb_mod_set(s, 1, SRC_LFO1, DST_ANG, 20);
}

/* CATWALK: disco on the runway, 116: four on the floor, a clap, plucked chord stabs off the beat, the purr on the
 * chords' bass in sixteenths, a chord a bar (A F#m7 Dadd9 E6), glossy; a lead */
static void orb_system_catwalk(orb_sys_t *s)
{
    static const int8_t stab[] = {4, 5, 4, 6, 5, 4}, bass[] = {0, 0, 2, 0, -1, 0, 1, 2}, glass[] = {2, 3, 4, 3};
    s->tuning = 0; s->sun = 9; s->bpm = 116; s->swing = 8; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_LEAD, 60);
    orb_put(s, ORB_A, LIB_GLASS, 44); s->snd[ORB_A].oct = 1;
    orb_put(s, ORB_B, LIB_CLAP, 84);
    orb_put(s, ORB_C, LIB_PLUCK, 54); s->snd[ORB_C].send = 30;
    orb_put(s, ORB_D, LIB_PURR, 58); s->snd[ORB_D].gate = 40; s->snd[ORB_D].d = 44;
    orb_ring_fol(&s->ring[0], 1, 1, "X...x...X...x...", glass, 4, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "....X.......X...", 0, 0, 0);
    orb_ring_fol(&s->ring[2], 1, 1, "..x..x.x ..x..x.x", stab, 6, 0);
    orb_ring_fol(&s->ring[3], 1, 1, "X..xx.x. x..xx.x.", bass, 8, 0);
    s->drive = 10;
    s->dtime = 5; s->dfb = 24; s->ping = 1; s->dmix = 14; s->rmix = 26; s->rsize = 55; s->rdecay = 64;
    s->bg_level = 36; s->bg_prog = 2; s->bg_change = 0; s->bg_halo = 30; s->bg_silk = 45; s->bg_width = 60;
    s->bg_cut = 40; s->bg_glow = 10;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 20);
}

/* SPIRAL: the drawing's pattern, generative. 16 24 20 28 steps at 1:1 3:2 5:4 7:5, all SPIRAL, PROB 74, MUTATE 30:
 * it never plays the same twice, and every note is in the chord (VOYAGE: eight of them), a long halo under it */
static void orb_system_spiral(orb_sys_t *s)
{
    static const int8_t ma[] = {2, 3, 4, 3, 5}, mb[] = {2, 3, 4, 3}, mc[] = {2, 1, 2, 3, 4}, md[] = {2, 3, 4, 3};
    orb_rdef_t A = {1, 1, 16, 5, 0, 0, DIR_SPIRAL, 0, 0, 74, ma, 5, 0x1};
    orb_rdef_t B = {3, 2, 24, 8, 1, 0, DIR_SPIRAL, 0, 0, 74, mb, 4, 0};
    orb_rdef_t C = {5, 4, 20, 7, 2, 0, DIR_SPIRAL, 0, 0, 74, mc, 5, 0};
    orb_rdef_t D = {7, 5, 28, 9, 3, 0, DIR_SPIRAL, 0, 0, 74, md, 4, 0};
    uint32_t r;
    s->tuning = 0; s->sun = 4; s->bpm = 112; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_TIDAL, 58);
    orb_put(s, ORB_A, LIB_PLUCK, 70);
    orb_put(s, ORB_B, LIB_GLASS, 54);
    orb_put(s, ORB_C, LIB_BELL, 52);
    orb_put(s, ORB_D, LIB_TOY, 56);
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    for (r = 0; r < ORB_NRING; r++) s->ring[r].follow = 1;
    s->mutate = 30; s->hum = 20; s->var = 37;
    s->dtime = 4; s->dfb = 44; s->ping = 1; s->dmix = 32; s->rmix = 45; s->rsize = 80; s->rdecay = 80;
    s->macro[MAC_ORBIT] = 25;
    s->ox = 74; s->oy = 38; s->orad = 26;
    orb_mod_set(s, 0, SRC_ORBX, DST_ECC, 30);
    orb_mod_set(s, 1, SRC_ORBY, DST_SPR, 35);
    orb_mod_set(s, 2, SRC_CONJ, DST_MOON, 30);
    orb_mod_set(s, 3, SRC_LFO1, DST_ANG, 25);
    s->bg_level = 40; s->bg_prog = 7; s->bg_change = 1; s->bg_halo = 70; s->bg_silk = 40; s->bg_width = 65;
    s->bg_cut = 30; s->bg_glow = 18;
}

/* LUNA NOVA: bossa nova under a new moon, 92: a rim on the clave, a shaker, a paw comping the chords' notes, the purr
 * on the bass and its fifth, jazz chords (DUSK: Am9 Fmaj7 Cadd9 Em7); the keys a flute */
static void orb_system_lunanova(orb_sys_t *s)
{
    static const int8_t comp[] = {3, 2, 3, 4, 3, 2, 4, 3, 2, 3}, bass[] = {0, 2};
    s->tuning = 0; s->sun = 0; s->bpm = 92; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_FLUTE, 66);
    orb_put(s, ORB_A, LIB_PAT, 82); s->snd[ORB_A].d = 30; s->snd[ORB_A].dust = 40; s->snd[ORB_A].color = 85;
    orb_put(s, ORB_B, LIB_TSSS, 66); s->snd[ORB_B].pan = 60;
    orb_put(s, ORB_C, LIB_PAW, 74); s->snd[ORB_C].pan = 44;
    orb_put(s, ORB_D, LIB_PURR, 74); s->snd[ORB_D].gate = 80;
    orb_ring_pat(&s->ring[0], 1, 1, "x..x..x. ..x.x...", 0, 0, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "XxxxXxxx XxxxXxxx", 0, 0, 92);
    orb_ring_fol(&s->ring[2], 1, 1, "x..x..x. ..x..x.. ..x..x.. x..x..x.", comp, 10, 0);
    orb_ring_fol(&s->ring[3], 1, 1, "X.......x.......", bass, 2, 0);
    s->dtime = 8; s->dfb = 28; s->ping = 1; s->dmix = 16; s->rmix = 35; s->rsize = 65; s->rdecay = 68;
    s->bg_level = 34; s->bg_prog = 3; s->bg_change = 1; s->bg_halo = 40; s->bg_silk = 55; s->bg_width = 60;
    s->bg_cut = 35; s->bg_glow = 12;
    orb_mod_set(s, 0, SRC_LFO2, DST_ANG, 20);
}

/* SGR A*: dub at the black hole, 72, one drop: the kick and the rim on three, a pluck skanking off the beat into long
 * echoes that tire, each a minor third lower than the last (steps, never a slide), a sub walking the chords (Cm Abmaj7
 * under G, subharmonic), a deep nebula; the keys a flute */
static void orb_system_sgra(orb_sys_t *s)
{
    static const int8_t skank[] = {3}, bass[] = {0, 0, 2, 1, 0, -1}, bell[] = {2};
    s->tuning = 2; s->sun = 7; s->bpm = 72; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_FLUTE, 60);
    orb_put(s, ORB_A, LIB_BELL, 46); s->snd[ORB_A].oct = 1;
    orb_put(s, ORB_B, LIB_PAT, 88); s->snd[ORB_B].dust = 50; s->snd[ORB_B].color = 70;
    orb_put(s, ORB_C, LIB_PLUCK, 62); s->snd[ORB_C].send = 70; s->snd[ORB_C].pan = 40;
    orb_put(s, ORB_D, LIB_SUB, 58);
    orb_ring_fol(&s->ring[0], 1, 1, "........X.......", bell, 1, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "........X.....x.", 0, 0, 0);
    orb_ring_fol(&s->ring[2], 1, 1, "..x...x...x...x.", skank, 1, 0);
    orb_ring_fol(&s->ring[3], 1, 1, "X...x..x...x.x..", bass, 6, 0);
    s->dtime = 8; s->dfb = 62; s->ping = 1; s->dmix = 36; s->tired = 3;
    s->rmix = 40; s->rsize = 85; s->rdecay = 82;
    orb_mod_set(s, 0, SRC_LFO2, DST_ANG, 25);
    s->bg_level = 40; s->bg_prog = 1; s->bg_change = 1; s->bg_halo = 60; s->bg_silk = 50; s->bg_width = 60;
    s->bg_cut = 35; s->bg_glow = 15;
}

/* BOOP BOP: chiptune at 140, dry: a kick, a zap, a pixel arpeggio up and down the chord in sixteenths, a pixel bass,
 * a chord a bar (the chords silent: only the arpeggio and the bass play them); a pixel lead */
static void orb_system_boopbop(orb_sys_t *s)
{
    static const int8_t arp[] = {2, 3, 4, 5, 4, 3, 2, 3, 2, 3, 4, 5, 6, 5, 4, 3}, bass[] = {0, 0, 2, 0, 0, 0, 2, 1},
                        bloop[] = {1, 2, 3};
    s->tuning = 4; s->sun = 0; s->bpm = 140; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_PIXEL, 60);
    orb_put(s, ORB_A, LIB_BLOOP, 60); s->snd[ORB_A].oct = 2;
    orb_put(s, ORB_B, LIB_ZAP, 92);
    orb_put(s, ORB_C, LIB_PIXEL, 78);
    orb_put(s, ORB_D, LIB_PIXEL, 88); s->snd[ORB_D].oct = 0; s->snd[ORB_D].gate = 60;
    orb_ring_fol(&s->ring[0], 1, 1, "X.......X..x....", bloop, 3, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "....X.......X...", 0, 0, 0);
    orb_ring_fol(&s->ring[2], 1, 1, "xxxxxxxxxxxxxxxx", arp, 16, 0);
    orb_ring_fol(&s->ring[3], 1, 1, "x.x.x.x.x.x.x.x.", bass, 8, 0);
    s->dtime = 3; s->dfb = 18; s->ping = 1; s->dmix = 14; s->rmix = 8; s->rsize = 40; s->rdecay = 58;
    s->bg_level = 0; s->bg_prog = 2; s->bg_change = 0; s->bg_halo = 0;
}

/* SNOWGLOBE: a music box in twelve-eight, 96 (the rings at 3:4: twelve steps a bar): a bell turning, a toy answering,
 * stars falling, a sub on the chords' bass and fifth, the chords over the sun in the bass (LULLABY), no drums */
static void orb_system_snowglobe(orb_sys_t *s)
{
    static const int8_t bell[] = {2, 3, 4, 3, 2, 1}, toy[] = {4, 3}, star[] = {2, 1, 3}, bass[] = {0, 2};
    s->tuning = 3; s->sun = 4; s->bpm = 96; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_TOY, 64);
    orb_put(s, ORB_A, LIB_BELL, 54);
    orb_put(s, ORB_B, LIB_TOY, 66); s->snd[ORB_B].pan = 64;
    orb_put(s, ORB_C, LIB_STAR, 54); s->snd[ORB_C].pan = 38;
    orb_put(s, ORB_D, LIB_SUB, 52); s->snd[ORB_D].gate = 90;
    orb_ring_fol(&s->ring[0], 3, 4, "x..x..x..x.x", bell, 6, 0);
    orb_ring_fol(&s->ring[1], 3, 4, "x.....x.....", toy, 2, 0);
    orb_ring_fol(&s->ring[2], 3, 4, "x.....x..... x.....x..x..", star, 3, 60);
    orb_ring_fol(&s->ring[3], 3, 4, "x.....x.....", bass, 2, 0);
    s->dtime = 9; s->dfb = 36; s->ping = 1; s->dmix = 24; s->rmix = 45; s->rsize = 80; s->rdecay = 80;
    s->bg_level = 44; s->bg_prog = 6; s->bg_change = 1; s->bg_halo = 70; s->bg_silk = 50; s->bg_width = 65;
    s->bg_cut = 30; s->bg_glow = 15;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);
}

/* STARDUST: synth pop in a nursery of stars, 108, on the overtone series (maj9 m7 6 add9): a thump on one and three,
 * glass arpeggiating the chord in sixteenths, stars at 3:2, a sub on the bass, a halo; the keys a tidal bell */
static void orb_system_stardust(orb_sys_t *s)
{
    static const int8_t arp[] = {3, 4, 5, 4, 3, 4, 5, 6, 5, 4, 3, 2}, star[] = {2, 3, 4, 3, 5, 4, 3, 2}, sub[] = {0, 0, 2},
                        bell[] = {1, 2};
    s->tuning = 1; s->sun = 4; s->bpm = 108; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_TIDAL, 58);
    orb_put(s, ORB_A, LIB_BELL, 44); s->snd[ORB_A].oct = 1;
    orb_put(s, ORB_B, LIB_GLASS, 48);
    orb_put(s, ORB_C, LIB_STAR, 50);
    orb_put(s, ORB_D, LIB_SUB, 54); s->snd[ORB_D].gate = 80;
    orb_ring_fol(&s->ring[0], 1, 1, "X.......X.......", bell, 2, 0);
    orb_ring_fol(&s->ring[1], 1, 1, "x.xxx.xx x.xxx.xx", arp, 12, 0);
    orb_ring_fol(&s->ring[2], 3, 2, "x..x.x...x..x..x.x...x..", star, 8, 75);
    orb_ring_fol(&s->ring[3], 1, 2, "X.......x..x....", sub, 3, 0);
    s->dtime = 8; s->dfb = 40; s->ping = 1; s->dmix = 24; s->rmix = 35; s->rsize = 75; s->rdecay = 76;
    s->bg_level = 40; s->bg_prog = 2; s->bg_change = 1; s->bg_halo = 55; s->bg_silk = 35; s->bg_width = 70;
    s->bg_cut = 30; s->bg_glow = 20; s->bloom = 30;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 35);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 25);
}

/* ALPHA CEN: a triple star as a gamelan, 100, pentatonic (nothing can clash): two bells interlocking, one on the
 * beat and one between, a toy for the slow melody under them, a gong every half; TIDE makes them shimmer and beat
 * as a gamelan's pairs do (the planets pulling on each other); no chords */
static void orb_system_alphacen(orb_sys_t *s)
{
    static const int8_t polos[] = {0, 1, 2, 1, 0, 1, 3, 2}, sangsih[] = {2, 3, 4, 3, 2, 3, 5, 4},
                        bal[] = {0, 2, 4, 3, 2, 1, 2, 0}, gong[] = {0, 2};
    s->tuning = 3; s->sun = 2; s->bpm = 100;
    orb_put(s, ORB_KEYS, LIB_BELL, 56);
    orb_put(s, ORB_A, LIB_BELL, 50); s->snd[ORB_A].pan = 40;
    orb_put(s, ORB_B, LIB_GLASS, 52); s->snd[ORB_B].pan = 60;
    orb_put(s, ORB_C, LIB_TOY, 64); s->snd[ORB_C].oct = 1;
    orb_put(s, ORB_D, LIB_SUB, 54); s->snd[ORB_D].gate = 99; s->snd[ORB_D].d = 70; s->snd[ORB_D].moon = 30; s->snd[ORB_D].mratio = 13;
    orb_ring_pat(&s->ring[0], 1, 1, "x.x.x.x.x.x.x.x.", polos, 8, 0);
    orb_ring_pat(&s->ring[1], 1, 1, ".x.x.x.x.x.x.x.x", sangsih, 8, 0);
    orb_ring_pat(&s->ring[2], 1, 2, "x.x.x.x.x.x.x.x.", bal, 8, 0);
    orb_ring_pat(&s->ring[3], 1, 2, "X.......x.......", gong, 2, 0);
    s->macro[MAC_TIDE] = 30;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 30);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 25);
    s->dtime = 6; s->dfb = 30; s->ping = 1; s->dmix = 20; s->rmix = 40; s->rsize = 75; s->rdecay = 80;
}

/* MOON NAP: a lullaby at 66 under the sun, the subharmonic series (ORBIT: Dm Bbmaj7 Gm7 Dm/F on A): a paw rocking the
 * chord, a music box at 5:4, now and then a star, a sub on the bass, slow; a long halo, no drums; the keys her mew */
static void orb_system_moonnap(orb_sys_t *s)
{
    static const int8_t paw[] = {2, 3, 4, 5, 3}, toy[] = {3, 2, 1, 2}, star[] = {0, 1, 2, 1}, sub[] = {0, 0};
    s->tuning = 2; s->sun = 9; s->bpm = 66; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_MEW, 60);
    orb_put(s, ORB_A, LIB_PAW, 80);
    orb_put(s, ORB_B, LIB_STAR, 52);
    orb_put(s, ORB_C, LIB_TOY, 66);
    orb_put(s, ORB_D, LIB_SUB, 52); s->snd[ORB_D].gate = 99;
    orb_ring_fol(&s->ring[0], 1, 1, "x..x..x. ..x..x..", paw, 5, 92);
    orb_ring_fol(&s->ring[1], 3, 2, "x..... x..... x..... x.....", star, 4, 55);
    orb_ring_fol(&s->ring[2], 5, 4, "x...x... ....x...x...", toy, 4, 70);
    orb_ring_fol(&s->ring[3], 1, 4, "X...x...", sub, 2, 0);
    s->dtime = 9; s->dfb = 40; s->ping = 1; s->dmix = 28; s->rmix = 50; s->rsize = 90; s->rdecay = 90;
    s->bg_level = 52; s->bg_prog = 2; s->bg_change = 1; s->bg_halo = 90; s->bg_silk = 50; s->bg_width = 70;
    s->bg_cut = 30; s->bg_glow = 20; s->bloom = 30;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 30);
}

/* AFTERGLOW: the oldest light, under everything, 70, on the overtone series (ORBIT: maj9 m7 6 add9 on D): a sub at
 * half speed, glass at 5:4 and stars at 7:4 and boops at 3:2, all on the chords' notes, the halo long, the nebula
 * blooming, echoes that tire a fourth lower each time; the keys a tidal bell */
static void orb_system_afterglow(orb_sys_t *s)
{
    static const int8_t ma[] = {0, 2}, mb[] = {3, 4, 5, 4, 6}, mc[] = {5, 6, 7, 8, 6, 5, 7}, md[] = {2, 3, 4, 3};
    orb_rdef_t A = {1, 2, 8, 2, 0, 0, DIR_FWD, 0, 0, 0, ma, 2, 0x1};
    orb_rdef_t B = {5, 4, 20, 5, 0, 0, DIR_FWD, 0, 0, 90, mb, 5, 0x1};
    orb_rdef_t C = {7, 4, 28, 7, 2, 0, DIR_FWD, 0, 0, 85, mc, 7, 0};
    orb_rdef_t D = {3, 2, 24, 4, 1, 0, DIR_FWD, 0, 0, 80, md, 4, 0};
    uint32_t r;
    s->tuning = 1; s->sun = 2; s->bpm = 70; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_TIDAL, 58);
    orb_put(s, ORB_A, LIB_SUB, 56); s->snd[ORB_A].gate = 90;
    orb_put(s, ORB_B, LIB_GLASS, 62); s->snd[ORB_B].oct = 1;
    orb_put(s, ORB_C, LIB_STAR, 56); s->snd[ORB_C].oct = 2;
    orb_put(s, ORB_D, LIB_BOOP, 68); s->snd[ORB_D].oct = 1;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    for (r = 0; r < ORB_NRING; r++) s->ring[r].follow = 1;
    s->mutate = 12; s->var = 15;
    s->dtime = 7; s->dfb = 55; s->ping = 1; s->dmix = 30; s->rmix = 60; s->rsize = 95; s->rdecay = 94; s->rdamp = 50;
    s->macro[MAC_ORBIT] = 20;
    s->bg_level = 48; s->bg_prog = 2; s->bg_change = 1; s->bg_glow = 30; s->bg_silk = 40; s->bg_width = 65; s->bg_cut = 35;
    s->bg_halo = 80; s->bloom = 55; s->tired = 5;
    orb_mod_set(s, 0, SRC_LFO2, DST_ANG, 30);
    orb_mod_set(s, 1, SRC_CONJ, DST_MOON, 30);
}

/* PULSAR: minimal techno, 128: four on the floor, open hats off the beat, zaps on the chord pulsing as a pulsar does
 * (BEAM), a sub rolling on the bass, a resonant filter breathing slowly over it all, dark chords low (ECLIPSE:
 * Am9 G6 Fmaj7 Em7); a lead */
static void orb_system_pulsar(orb_sys_t *s)
{
    static const int8_t zap[] = {4, 4, 5, 4, 6}, bass[] = {0}, boop[] = {2, 1, 2, 3};
    s->tuning = 0; s->sun = 0; s->bpm = 128; s->keymode = KM_CHORD;
    orb_put(s, ORB_KEYS, LIB_LEAD, 56);
    orb_put(s, ORB_A, LIB_BOOP, 54); s->snd[ORB_A].beam = 40;
    orb_put(s, ORB_B, LIB_TSSS, 88);
    orb_put(s, ORB_C, LIB_ZAP, 74); s->snd[ORB_C].beam = 60; s->snd[ORB_C].send = 30;
    orb_put(s, ORB_D, LIB_SUB, 62); s->snd[ORB_D].gate = 40;
    orb_ring_fol(&s->ring[0], 1, 1, "X...x...X...x...", boop, 4, 0);
    orb_ring_pat(&s->ring[1], 1, 1, "..x...x...x...x.", 0, 0, 0);
    orb_ring_fol(&s->ring[2], 1, 1, "x..x..x..x..x...", zap, 5, 0);
    orb_ring_fol(&s->ring[3], 1, 1, "..xx..xx..xx..x.", bass, 1, 0);
    s->cut = 74; s->res = 30; s->lrate[0] = 8; s->lshape[0] = LS_SINE;
    orb_mod_set(s, 0, SRC_LFO1, DST_CUT, 30);
    s->dtime = 6; s->dfb = 36; s->ping = 1; s->dmix = 18; s->rmix = 20; s->rsize = 50; s->rdecay = 64;
    s->bg_level = 28; s->bg_prog = 5; s->bg_change = 1; s->bg_halo = 30; s->bg_silk = 45; s->bg_width = 55;
    s->bg_cut = 45; s->bg_glow = 8;
}

/* ---- the legend's universe: what the film of Astralica is made of (orb_intro_system), as systems to play. The dark
 * drone on the sun (held: SF_HOLD), the planets' chirps falling in from octaves under (COMET), her tummy's gurgles
 * (BLOOP), the plops (BOOP), the orrery's glass rings on the overtone series, her meow; long echoes, a vast nebula,
 * GRAVITY pulling every note into tune. Each its own tempo and rhythm */

/* ASTRALICA: the film itself, 100: the drone held on the sun, the orrery's three rings at 5:4 3:2 7:4 on 4:5:6:7 in the
 * overtone scale, the echoes and the nebula of the legend; the keys her meow */
static void orb_system_astralica(orb_sys_t *s)
{
    static const int8_t mb[] = {0, 1, 0, -1, 2}, mc[] = {0, 0, 2, 0, -2, 1}, md[] = {0, 2, 0, 4, 0, -1, 1};
    orb_rdef_t B = {5, 4, 20, 5, 0, 2, DIR_FWD, 0, 0, 0, mb, 5, 0};
    orb_rdef_t C = {3, 2, 24, 6, 0, 4, DIR_FWD, 0, 0, 0, mc, 6, 0};
    orb_rdef_t D = {7, 4, 28, 7, 0, 6, DIR_FWD, 0, 0, 90, md, 7, 0};
    s->tuning = 1; s->sun = 0; s->bpm = 100;
    orb_put(s, ORB_KEYS, LIB_MEOW, 62);
    orb_put(s, ORB_A, LIB_DRONE, 38); s->snd[ORB_A].oct = 0; s->snd[ORB_A].flags |= SF_HOLD;
    orb_put(s, ORB_B, LIB_GLASS, 0);
    orb_put(s, ORB_C, LIB_PLUCK, 0);
    orb_put(s, ORB_D, LIB_BELL, 56);
    orb_ring_pat(&s->ring[0], 1, 4, "X.......", 0, 0, 0);
    orb_ring_set(&s->ring[1], &B); orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->dtime = 9; s->dfb = 62; s->ping = 1; s->dmix = 46; s->rmix = 62; s->rsize = 92; s->rdecay = 88; s->rdamp = 50;
    s->hshift = -12; s->hretro = 25; s->hgrain = 60;
    s->macro[MAC_GRAVITY] = 72; s->macro[MAC_ORBIT] = 20;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 30);
}

/* NOM NOM: the planets eaten, 88, half time: a soft thump on one and three, chirps falling in from octaves under on
 * the eighths, plops at 3:2 on the rings' own notes (4:5:6:7), the drone held under; the keys her meow */
static void orb_system_nomnom(orb_sys_t *s)
{
    static const int8_t gulp[] = {0, 4}, chirp[] = {8, 12, 16, 14}, plop[] = {8, 10, 12, 14, 12, 10};
    s->tuning = 1; s->sun = 7; s->bpm = 88;
    orb_put(s, ORB_KEYS, LIB_MEOW, 62);
    orb_put(s, ORB_A, LIB_BLOOP, 66); s->snd[ORB_A].oct = 2;   /* (a gulp on one and three) */
    orb_put(s, ORB_B, LIB_COMET, 96); s->snd[ORB_B].oct = 0; s->snd[ORB_B].send = 50;
    orb_put(s, ORB_C, LIB_BOOP, 86); s->snd[ORB_C].oct = 1;
    orb_put(s, ORB_D, LIB_DRONE, 30); s->snd[ORB_D].oct = 0; s->snd[ORB_D].flags |= SF_HOLD;
    orb_ring_pat(&s->ring[0], 1, 1, "X.......X.......", gulp, 2, 0);
    orb_ring_pat(&s->ring[1], 1, 2, "X...x.x. x.......", chirp, 4, 0);
    orb_ring_pat(&s->ring[2], 3, 2, "x.....x..x..x.....x..x..", plop, 6, 80);
    orb_ring_pat(&s->ring[3], 1, 4, "X...............", 0, 0, 0);
    s->dtime = 7; s->dfb = 55; s->ping = 1; s->dmix = 40; s->rmix = 55; s->rsize = 90; s->rdecay = 86; s->rdamp = 50;
    s->macro[MAC_GRAVITY] = 72;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 30);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 25);
}

/* TUMMY: her tummy, glowing and gurgling, 76: bubbles gurgling up in a syncopated sixteenth, boops at 5:4 answering,
 * glints of glass high at 7:4, the drone held low; the planets tug at each other (TIDE) so it all wobbles; she mews */
static void orb_system_tummy(orb_sys_t *s)
{
    static const int8_t gurgle[] = {0, 2, 1, 4, 0, 3}, boop[] = {4, 2, 6, 2}, glint[] = {8, 10, 12, 14};
    s->tuning = 1; s->sun = 3; s->bpm = 76;
    orb_put(s, ORB_KEYS, LIB_MEW, 0);
    orb_put(s, ORB_A, LIB_BLOOP, 99); s->snd[ORB_A].oct = 1;
    orb_put(s, ORB_B, LIB_BOOP, 88); s->snd[ORB_B].oct = 1;
    orb_put(s, ORB_C, LIB_GLASS, 60);
    orb_put(s, ORB_D, LIB_DRONE, 32); s->snd[ORB_D].oct = 0; s->snd[ORB_D].flags |= SF_HOLD;
    orb_ring_pat(&s->ring[0], 1, 1, "x..xx...x.x..x..", gurgle, 6, 72);
    orb_ring_pat(&s->ring[1], 5, 4, "x...x.x.....x...x...", boop, 4, 64);
    orb_ring_pat(&s->ring[2], 7, 4, "x......x.....x..x.......x...", glint, 4, 70);
    orb_ring_pat(&s->ring[3], 1, 4, "X.......", 0, 0, 0);
    s->macro[MAC_TIDE] = 20; s->macro[MAC_GRAVITY] = 60;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 40);
    orb_mod_set(s, 1, SRC_LFO2, DST_ECC, 25);
    s->dtime = 6; s->dfb = 48; s->ping = 1; s->dmix = 34; s->rmix = 50; s->rsize = 80; s->rdecay = 82;
}

/* MULTIVERSE: her meow, echoing from universe to universe, 60, no beat: the drone held for four bars at a time, a
 * meow now and then whose echoes come back a fourth lower each time (TIRED LIGHT), bells at 7:4, a star at 3:2, the
 * sun's own chord far under in a long halo; the keys a tidal bell */
static void orb_system_multiverse(orb_sys_t *s)
{
    static const int8_t meow[] = {7, 9, 6}, bell[] = {4, 6, 9}, star[] = {12, 10};
    s->tuning = 1; s->sun = 7; s->bpm = 60;
    orb_put(s, ORB_KEYS, LIB_TIDAL, 58);
    orb_put(s, ORB_A, LIB_DRONE, 36); s->snd[ORB_A].oct = 0; s->snd[ORB_A].flags |= SF_HOLD;
    orb_put(s, ORB_B, LIB_MEOW, 70); s->snd[ORB_B].oct = 1; s->snd[ORB_B].send = 70;
    orb_put(s, ORB_C, LIB_BELL, 50);
    orb_put(s, ORB_D, LIB_STAR, 70);
    orb_ring_pat(&s->ring[0], 1, 4, "X...............", 0, 0, 0);
    orb_ring_pat(&s->ring[1], 1, 4, "x.......x.......", meow, 3, 70);
    orb_ring_pat(&s->ring[2], 7, 4, "x......x...........x........", bell, 3, 80);
    orb_ring_pat(&s->ring[3], 3, 2, "x.......................", star, 2, 70);
    s->dtime = 10; s->dfb = 70; s->ping = 1; s->dmix = 50; s->tired = 5;
    s->rmix = 70; s->rsize = 96; s->rdecay = 99; s->rdamp = 45; s->bloom = 40;
    s->macro[MAC_GRAVITY] = 72;
    s->bg_level = 26; s->bg_prog = 0; s->bg_halo = 90; s->bg_silk = 55; s->bg_width = 70; s->bg_cut = 30; s->bg_glow = 25;
}

static void orb_system(uint32_t i, orb_sys_t *s)
{
    static void (*const F[ORB_NSYSTEM])(orb_sys_t *) = {orb_system_kitten, orb_system_milky, orb_system_harmonices,
        orb_system_purrsec, orb_system_zoomies, orb_system_catwalk, orb_system_spiral, orb_system_lunanova,
        orb_system_sgra, orb_system_boopbop, orb_system_snowglobe, orb_system_stardust, orb_system_alphacen,
        orb_system_moonnap, orb_system_afterglow, orb_system_pulsar, orb_system_astralica, orb_system_nomnom,
        orb_system_tummy, orb_system_multiverse};
    static const uint8_t MOOD_OF[ORB_NSYSTEM] = {1, 0, 2, 1, 3, 1, 2, 1, 1, 3, 0, 2, 2, 0, 2, 3, 2, 1, 1, 0};   /* (CUTE's) */
    uint32_t r;
    orb_sys_clear(s, ORB_SYSTEM_NAME[i % ORB_NSYSTEM]);
    F[i % ORB_NSYSTEM](s);
    s->mood = MOOD_OF[i % ORB_NSYSTEM];
    for (r = ORB_A; r <= ORB_D; r++) orb_hush(&s->snd[r]);
}

/* ---- the intro: the legend of Astralica (orbitrix.c's intro() plays it). Its own system: a drone on the keys, the
 * chirp of a planet eaten on A, the orrery's three rings on B C D (HARMONICES's), and then the keys become her meow */
static const orb_snd_t SND_DRONE = {.ecc = 22, .moon = 30, .mratio = 1, .ang = 40, .spr = 85, .prec = 45, .dust = 12,
    .color = 20, .a = 72, .d = 80, .s = 85, .r = 74, .eamt = 15, .ea = 70, .ed = 80, .vsens = 20, .level = 74, .pan = 50,
    .send = 72, .gate = 99, .oct = 0, .tide = 30, .lib = LIB_DRONE};
static const orb_snd_t SND_CHIRP = {.ecc = 50, .sling = 40, .ang = 10, .spr = 30, .insp = 80, .inspt = 64, .a = 2,
    .d = 72, .s = 0, .r = 64, .eamt = 70, .ea = 0, .ed = 56, .vsens = 30, .level = 96, .pan = 50, .send = 45, .gate = 99,
    .oct = 0, .lib = LIB_COMET};
static const orb_snd_t SND_MEOW = {.ecc = 14, .sling = 25, .ang = 35, .spr = 45, .moon = 30, .mratio = 1, .dust = 10,
    .color = 40, .a = 26, .d = 62, .s = 55, .r = 56, .eamt = 75, .ea = 44, .ed = 58, .vsens = 0, .level = 92, .pan = 50,
    .send = 78, .gate = 90, .oct = 0, .lib = LIB_MEOW};


/* the library, in SOUND's order (by kind: drums, bass, keys, leads, pads). oct: its register, as a ring plays it (the
 * ring's root at C2, 12 x oct above); on the keys' part two octaves less, so the keyboard plays it where a ring does */
typedef struct { const char *name; uint8_t kind; const orb_snd_t *snd; int8_t oct; } orb_lib_t;
static const orb_lib_t ORB_LIB[ORB_NLIB] = {
    [LIB_NONE] = {"-", LK_KEYS, 0, 0},
    [LIB_BOOM] = {"BOOM", LK_KICK, &SND_KICK, -1}, [LIB_THUMP] = {"THUMP", LK_KICK, &SND_THUMP, -1},
    [LIB_TOM] = {"TOM", LK_PERC, &SND_TOM, 0}, [LIB_PAT] = {"PAT", LK_SNARE, &SND_SNARE, 1},
    [LIB_CLAP] = {"CLAP", LK_SNARE, &SND_CLAP, 1}, [LIB_TICK] = {"TICK", LK_HAT, &SND_HAT, 2},
    [LIB_TSSS] = {"TSSS", LK_HAT, &SND_TSSS, 2}, [LIB_ZAP] = {"ZAP", LK_PERC, &SND_ZAP, 2},
    [LIB_BOOP] = {"BOOP", LK_PERC, &SND_BOOP, 2}, [LIB_SQUEAK] = {"SQUEAK", LK_PERC, &SND_SQUEAK, 3},
    [LIB_BLOOP] = {"BLOOP", LK_PERC, &SND_BLOOP, 3},
    [LIB_PURR] = {"PURR", LK_BASS, &SND_BASS, 0}, [LIB_GROWL] = {"GROWL", LK_BASS, &SND_GROWL, 0},
    [LIB_SUB] = {"SUB", LK_BASS, &SND_SUB, 0}, [LIB_SWOOP] = {"SWOOP", LK_BASS, &SND_SWOOP, 0},
    [LIB_PLUCK] = {"PLUCK", LK_KEYS, &SND_PLUCK, 1}, [LIB_PAW] = {"PAW", LK_KEYS, &SND_PAW, 1},
    [LIB_GLASS] = {"GLASS", LK_KEYS, &SND_GLASS, 2}, [LIB_BELL] = {"BELL", LK_KEYS, &SND_BELL, 2},
    [LIB_STAR] = {"STAR", LK_KEYS, &SND_STAR, 3}, [LIB_TIDAL] = {"TIDAL", LK_KEYS, &SND_TIDAL, 1},
    [LIB_TOY] = {"TOY", LK_KEYS, &SND_TOY, 2},
    [LIB_PIXEL] = {"PIXEL", LK_LEAD, &SND_PIXEL, 2}, [LIB_LEAD] = {"LEAD", LK_LEAD, &SND_LEAD, 2},
    [LIB_FLUTE] = {"FLUTE", LK_LEAD, &SND_FLUTE, 2}, [LIB_MEOW] = {"MEOW", LK_VOICE, &SND_MEOW, 2},
    [LIB_MEW] = {"MEW", LK_VOICE, &SND_MEW, 2},
    [LIB_CLOUD] = {"CLOUD", LK_PAD, &SND_PAD, 1}, [LIB_DRONE] = {"DRONE", LK_PAD, &SND_DRONE, 1},
    [LIB_COMET] = {"COMET", LK_PAD, &SND_CHIRP, 1}};
/* library sound li into s, for part (ORB_KEYS or a ring): at its own level (the library is balanced), the part's PAN kept */
static void orb_lib_load(orb_snd_t *s, uint32_t li, uint32_t part)
{
    uint8_t pan = s->pan;
    int32_t oct;
    if (!li || li >= ORB_NLIB) return;
    *s = *ORB_LIB[li].snd;
    oct = ORB_LIB[li].oct - (part == ORB_KEYS ? 2 : 0);
    s->oct = (int8_t)(oct < -3 ? -3 : oct > 3 ? 3 : oct);
    s->pan = pan;
    s->lib = (uint8_t)li;
}
static uint32_t orb_lib_kind(const orb_snd_t *s) { return ORB_LIB[s->lib < ORB_NLIB ? s->lib : 0].kind; }
/* the drums whisper (D28): under a space a minute long a snare's or a hat's noise would fill it, so they play at a
 * quarter of their level (-12 dB) and send nothing into it: the space is the notes'. (No system has a kick) */
static void orb_hush(orb_snd_t *d)
{
    uint32_t k = orb_lib_kind(d);
    if (k != LK_SNARE && k != LK_HAT) return;
    d->level = (uint8_t)(d->level / 4u < 8u ? 8u : d->level / 4u);
    d->send = 0;
}

static const char *orb_lib_name(const orb_snd_t *s) { return s->lib && s->lib < ORB_NLIB ? ORB_LIB[s->lib].name : "OWN"; }

static void orb_intro_system(orb_sys_t *s)
{
    static const int8_t mb[] = {0, 1, 0, -1, 2}, mc[] = {0, 0, 2, 0, -2, 1}, md[] = {0, 2, 0, 4, 0, -1, 1};
    orb_rdef_t A = {1, 1, 16, 0, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0};
    orb_rdef_t B = {5, 4, 20, 5, 0, 2, DIR_FWD, 0, 0, 0, mb, 5, 0};
    orb_rdef_t C = {3, 2, 24, 6, 0, 4, DIR_FWD, 0, 0, 0, mc, 6, 0};
    orb_rdef_t D = {7, 4, 28, 7, 0, 6, DIR_FWD, 0, 0, 90, md, 7, 0};
    orb_sys_clear(s, "ASTRALICA");
    s->tuning = 1; s->sun = 0; s->bpm = 100;
    s->snd[0] = SND_DRONE;
    s->snd[1] = SND_CHIRP;
    s->snd[2] = SND_GLASS;
    s->snd[3] = SND_PLUCK;
    s->snd[4] = SND_BELL; s->snd[4].level = 56;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->ring[0].mute = 1;                    /* A only sounds when a planet is eaten (orb_part_on) */
    s->dtime = 9; s->dfb = 62; s->ping = 1; s->dmix = 46; s->rmix = 62; s->rsize = 92; s->rdamp = 50;
    s->rdecay = 45;                         /* (6.6 s, as the film was made: D28's law) */
    s->hshift = -12; s->hretro = 25; s->hgrain = 60;
    s->macro[MAC_GRAVITY] = 72; s->macro[MAC_ORBIT] = 20;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 30);
}
/* ---- CUTE: a whole patch at once, in a mood. Each mood has its sounds by role, its tunings and chords, its speeds,
 * its space, and rhythms written by hand (a bank for each role: a few to pick from); every pitched ring FOLLOWs the
 * chords and the keys are CHORD keys, so whatever is made, and whatever is played on it, is in the chord sounding;
 * no kick (the beat a pitched pulse), the snares and hats whispering, the space long.
 * orb_mood turns a patch to a mood: its tempo, sounds, space and chords, the patterns kept */
enum { ORB_MOOD_SLEEPY, ORB_MOOD_PLAYFUL, ORB_MOOD_SPARKLY, ORB_MOOD_ZOOMIES, ORB_NMOOD };
static const char *const ORB_MOOD_NAME[ORB_NMOOD] = {"SLEEPY", "PLAYFUL", "SPARKLY", "ZOOMIES"};
enum { RO_PULSE, RO_SNARE, RO_MEL, RO_SPARK, RO_COUNT, RO_BASS };   /* a ring's role (the pulse: on the beat, pitched,
                                                                     * where a kick would be: no kick, D28) */
typedef struct {
    uint8_t bpm_lo, bpm_hi, swing_hi, halo_lo, halo_hi, level, change, mutate;
    uint8_t role[ORB_NRING];                /* A..D's roles */
    uint8_t tunings[3], progs[4];           /* (0xFF: none more) */
    uint8_t snd[6][4];                      /* each role's sounds to pick from (LIB_*, 0: none more) */
    uint8_t keys[4];
    uint8_t ratio_p[ORB_NRING], ratio_q[ORB_NRING];
    uint8_t dtime, dfb, dmix, rmix, rsize, rdecay;
} orb_mood_t;
static const orb_mood_t ORB_MOOD[ORB_NMOOD] = {
    {62, 76, 0, 85, 95, 52, 1, 6, {RO_MEL, RO_SPARK, RO_COUNT, RO_BASS}, {3, 2, 0}, {6, 1, 3, 0xFF},
     {{LIB_GLASS}, {LIB_PAT}, {LIB_PAW, LIB_TOY, LIB_BELL}, {LIB_STAR}, {LIB_TOY, LIB_GLASS}, {LIB_SUB, LIB_PURR}},
     {LIB_MEW, LIB_TIDAL, LIB_BELL}, {1, 3, 5, 1}, {1, 2, 4, 4}, 9, 40, 28, 50, 90, 90},
    {96, 112, 26, 50, 65, 38, 1, 10, {RO_PULSE, RO_SNARE, RO_MEL, RO_BASS}, {0, 3, 0xFF}, {2, 4, 1, 0xFF},
     {{LIB_BOOP, LIB_BLOOP}, {LIB_CLAP, LIB_PAT}, {LIB_BOOP, LIB_SQUEAK, LIB_BLOOP, LIB_TOY}, {LIB_STAR}, {LIB_TOY},
      {LIB_PURR, LIB_GROWL}},
     {LIB_TOY, LIB_PAW, LIB_PIXEL, LIB_MEW}, {1, 1, 1, 1}, {1, 1, 1, 1}, 6, 32, 22, 30, 60, 66},
    {88, 104, 10, 70, 85, 46, 1, 12, {RO_PULSE, RO_SPARK, RO_COUNT, RO_BASS}, {1, 3, 0}, {2, 3, 7, 4},
     {{LIB_BELL, LIB_GLASS}, {LIB_PAT}, {LIB_GLASS}, {LIB_STAR}, {LIB_GLASS, LIB_BELL}, {LIB_SUB}},
     {LIB_TIDAL, LIB_BELL, LIB_STAR}, {1, 3, 5, 1}, {1, 2, 4, 2}, 8, 46, 30, 40, 80, 82},
    {120, 136, 0, 35, 50, 36, 0, 8, {RO_PULSE, RO_SNARE, RO_SPARK, RO_BASS}, {0, 4, 0xFF}, {2, 4, 5, 0xFF},
     {{LIB_BOOP, LIB_BLOOP}, {LIB_CLAP}, {LIB_SQUEAK}, {LIB_SQUEAK, LIB_ZAP, LIB_PIXEL, LIB_BLOOP}, {LIB_ZAP}, {LIB_GROWL, LIB_PURR}},
     {LIB_PIXEL, LIB_LEAD}, {1, 1, 3, 1}, {1, 1, 2, 1}, 5, 26, 18, 22, 50, 60},
};
/* the rhythms, by role and mood (16 steps unless the role runs at 3:2 (24) or 5:4 (20); x a hit, X an accent) */
static const char *const ORB_CUTE_PULSE[ORB_NMOOD][3] = {
    {"X.......x.......", "X...............", "X.......X......."},
    {"X......x..x.....", "X..x..x...x.....", "X.....x.X......."},
    {"X.........x.....", "X.......X.....x.", "X...........x..."},
    {"X...x...X...x...", "X...x...X...x..x", "X...x...X..xx..."}};
static const char *const ORB_CUTE_SNARE[ORB_NMOOD][3] = {
    {"........x.......", "........x.......", "........x......."},
    {"....X.......X...", "....X.......X..x", "....X..x....X..."},
    {"........X.......", "....x.......X...", "........X......."},
    {"....X.......X...", "....X.......X.x.", "....X...x...X..."}};
static const char *const ORB_CUTE_MEL[ORB_NMOOD][3] = {
    {"x..x..x...x..x..", "x.....x...x.....", "x..x....x..x...."},
    {"x..x..x.x..x.x..", "x.x..x.x..x.x...", "x..x.x..x..x..x."},
    {"x..x.x...x..x..x", "x.x...x.x...x.x.", "x...x.x...x.x..."},
    {"x.x.x..xx.x..x.x", "xx.x.x..x.x.x.x.", "x.xx.x.xx.x..x.x"}};
static const char *const ORB_CUTE_SPARK[3] = {   /* (3:2: 24 steps) */
    "x..x.x...x..x..x.x...x..", "x.x...x..x.x...x..x...x.", "x.....x.....x.....x....."};
static const char *const ORB_CUTE_CHASE[3] = {   /* (3:2: 24 steps, the zoomies) */
    "x.xX..x.x..X x.x..x.X..x.", "xx.x.x..x.x. x.x..xx.x.x.", "x.x.x..x.xx. x..x.x.x..x."};
static const char *const ORB_CUTE_COUNT[3] = {   /* (5:4: 20 steps) */
    "x...x...x...x...x...", "x...x.x.....x...x...", "x...x.......x...x..."};
static const char *const ORB_CUTE_BASS[ORB_NMOOD][3] = {
    {"X...x...", "X.......", "X...x.x."},                                       /* (1:4: 8 steps) */
    {"X..x..x.x...x...", "X.....x.x..x....", "X..x....X..x...."},
    {"X.......x..x....", "X.......X.......", "X.....x.x......."},               /* (1:2) */
    {"x.x.x.x.x.x.x.x.", "x.xxx.x.x.xxx.x.", "x.x.xx.xx.x.x.x."}};

static uint32_t orb_pick_end(uint32_t *rng, const uint8_t *list, uint32_t n, uint32_t end)   /* one of a list */
{
    uint32_t k = 0;
    while (k < n && list[k] != end) k++;
    if (!k) return list[0];
    return list[orb_xs(rng) % k];
}
static uint32_t orb_pick(uint32_t *rng, const uint8_t *list, uint32_t n) { return orb_pick_end(rng, list, n, 0u); }   /* (sounds: 0 ends) */
static uint32_t orb_pick_ff(uint32_t *rng, const uint8_t *list, uint32_t n) { return orb_pick_end(rng, list, n, 0xFFu); }

/* FOLLOW on or off for ring r, its notes kept where they were (each step to the nearest note of the other kind: the
 * chord's notes, of the progression's first chord, or the tuning's degrees) */
static void orb_follow_set(orb_sys_t *s, uint32_t r, uint32_t on)
{
    orb_ring_t *g = &s->ring[r % ORB_NRING];
    uint32_t id = ORB_BGP_C[s->tuning % ORB_NTUNING][s->bg_prog % ORB_NBGPROG][0], i;
    if (!g->follow == !on) return;
    for (i = 0; i < ORB_NSTEP; i++) {
        int32_t d = g->deg + g->dg[i], p = g->follow ? orb_chord_tone(id, d) : orb_degree(s, d), best = 0, bd = 1 << 30, k;
        if (g->on[i] & ORB_ON_FIX) { g->dg[i] = (int8_t)orb_clamp(d, -14, 21); continue; }   /* (REC's: the tuning's) */
        for (k = -10; k <= 20; k++) {
            int32_t q = on ? orb_chord_tone(id, k) : orb_degree(s, k), e = q > p ? q - p : p - q;
            if (e < bd) { bd = e; best = k; }
        }
        g->dg[i] = (int8_t)best;
    }
    g->deg = 0;
    g->follow = (uint8_t)(on != 0u);
}
static void orb_cute_snd(orb_sys_t *s, uint32_t part, uint32_t li, uint32_t role)
{
    s->snd[part].pan = 50;
    orb_lib_load(&s->snd[part], li, part);
    if (part != ORB_KEYS) orb_hush(&s->snd[part]);   /* (the drums whisper) */
    if (role == RO_BASS) { s->snd[part].flags &= (uint8_t)~SF_LEGATO; s->snd[part].level = (uint8_t)(s->snd[part].level * 8u / 10u); }
    if (role == RO_SPARK) s->snd[part].pan = 38;   /* (a little apart) */
    if (role == RO_COUNT) s->snd[part].pan = 62;
}
/* the moods' space, chords and tempo into s (its patterns, its sun and tuning kept) */
static void orb_mood_space(orb_sys_t *s, uint32_t mood, uint32_t *rng)
{
    const orb_mood_t *m = &ORB_MOOD[mood % ORB_NMOOD];
    s->mood = (uint8_t)(mood % ORB_NMOOD);
    s->bpm = (uint8_t)(m->bpm_lo + orb_xs(rng) % (uint32_t)(m->bpm_hi - m->bpm_lo + 1u));
    s->swing = (uint8_t)(m->swing_hi ? orb_xs(rng) % (m->swing_hi + 1u) : 0u);
    s->mutate = m->mutate;
    s->bg_prog = (uint8_t)orb_pick_ff(rng, m->progs, 4);
    s->bg_change = m->change;
    s->bg_level = m->level;
    s->bg_halo = (uint8_t)(m->halo_lo + orb_xs(rng) % (uint32_t)(m->halo_hi - m->halo_lo + 1u));
    s->bg_silk = 45; s->bg_width = 60; s->bg_cut = 30; s->bg_glow = 18; s->bg_temp = 0; s->bg_out = 0;
    s->bloom = mood == ORB_MOOD_ZOOMIES ? 0 : 30;
    s->dtime = m->dtime; s->dfb = m->dfb; s->dmix = m->dmix; s->ping = 1;
    s->rmix = m->rmix; s->rsize = m->rsize; s->rdecay = m->rdecay;
    s->keymode = KM_CHORD;
}
static void orb_cute_mel(orb_ring_t *r, uint32_t p, uint32_t q, const char *pat, uint32_t *rng, int32_t lo, int32_t hi,
                         uint8_t prob)
{
    int8_t mel[24];
    uint32_t i;
    int32_t d = lo + 1 + (int32_t)(orb_xs(rng) % 3u);
    for (i = 0; i < 24u; i++) {             /* a walk on the chord's notes: steps of one or two, now up, now down */
        static const int8_t ST[6] = {1, -1, 2, -2, 1, -1};
        mel[i] = (int8_t)d;
        d += ST[orb_xs(rng) % 6u];
        if (d < lo) d = lo + 1;
        if (d > hi) d = hi - 1;
    }
    orb_ring_pat(r, p, q, pat, mel, 24, prob);
    r->follow = 1;
}
/* a whole new patch in a mood, from seed */
static void orb_cute(orb_sys_t *s, uint32_t mood, uint32_t seed)
{
    const orb_mood_t *m = &ORB_MOOD[mood % ORB_NMOOD];
    uint32_t rng = seed * 2654435761u + 0x9E3779B9u, r, v = 0;
    mood %= ORB_NMOOD;
    orb_sys_clear(s, ORB_MOOD_NAME[mood]);
    orb_xs(&rng);
    s->tuning = (uint8_t)orb_pick_ff(&rng, m->tunings, 3);
    s->sun = (uint8_t)(orb_xs(&rng) % 12u);
    orb_mood_space(s, mood, &rng);
    s->var = 20;
    orb_cute_snd(s, ORB_KEYS, orb_pick(&rng, m->keys, 4), RO_MEL);
    for (r = 0; r < ORB_NRING; r++) {
        uint32_t role = m->role[r], p = m->ratio_p[r], q = m->ratio_q[r], k = orb_xs(&rng) % 3u;
        orb_ring_t *g = &s->ring[r];
        orb_cute_snd(s, ORB_A + r, orb_pick(&rng, m->snd[role], 4), role);
        switch (role) {
        case RO_PULSE: {                    /* (on the beat: the chord's notes over its bass) */
            static const int8_t P[3][4] = {{1, 2, 1, 3}, {2, 1, 2, 2}, {1, 3, 2, 1}};
            orb_ring_pat(g, p, q, ORB_CUTE_PULSE[mood][k], P[orb_xs(&rng) % 3u], 4, 0);
            g->follow = 1;
            break;
        }
        case RO_SNARE: orb_ring_pat(g, p, q, ORB_CUTE_SNARE[mood][k], 0, 0, 0); break;
        case RO_MEL: orb_cute_mel(g, p, q, ORB_CUTE_MEL[mood][k], &rng, 1, 6, mood == ORB_MOOD_SLEEPY ? 90 : 0); break;
        case RO_SPARK:
            orb_cute_mel(g, p, q, mood == ORB_MOOD_ZOOMIES ? ORB_CUTE_CHASE[k] : ORB_CUTE_SPARK[k], &rng, 0, 5,
                         mood == ORB_MOOD_ZOOMIES ? 85 : 70);
            break;
        case RO_COUNT: orb_cute_mel(g, p, q, ORB_CUTE_COUNT[k], &rng, 1, 5, 80); break;
        default: {                          /* the bass: the chord's bass, its neighbours now and then */
            static const int8_t B[3][8] = {{0, 0, -1, 0, 0, 1, 0, -1}, {0, 0, 0, 2, 0, -1, 0, 1}, {0, -1, 0, 0, 1, 0, 0, 2}};
            orb_ring_pat(g, p, q, ORB_CUTE_BASS[mood][k], B[orb_xs(&rng) % 3u], 8, 0);
            g->follow = 1;
            break;
        }
        }
        v++;
    }
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);   /* (where the orbits meet, the moons twinkle) */
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 25);
    (void)v;
}
/* a patch turned to a mood: its tempo, swing, space and chords; its sounds by kind (a kick the mood's pulse, a bass its
 * bass, the pitched ones its melodies, the keys its keys); its patterns, sun and tuning kept */
static void orb_mood(orb_sys_t *s, uint32_t mood)
{
    const orb_mood_t *m = &ORB_MOOD[mood % ORB_NMOOD];
    uint32_t rng = (uint32_t)s->sun * 7919u + mood * 104729u + 1u, r;
    mood %= ORB_NMOOD;
    orb_mood_space(s, mood, &rng);
    orb_cute_snd(s, ORB_KEYS, orb_pick(&rng, m->keys, 4), RO_MEL);
    for (r = 0; r < ORB_NRING; r++) {
        uint32_t k = orb_lib_kind(&s->snd[ORB_A + r]), role, li;
        role = k == LK_KICK ? RO_PULSE : k == LK_SNARE ? RO_SNARE : k == LK_HAT ? RO_SNARE : k == LK_BASS ? RO_BASS :
               (r & 1u) ? RO_SPARK : RO_MEL;
        li = k == LK_HAT ? (mood == ORB_MOOD_SLEEPY ? LIB_TICK : LIB_TSSS) : orb_pick(&rng, m->snd[role], 4);
        orb_cute_snd(s, ORB_A + r, li, role);
        if (mood == ORB_MOOD_SLEEPY && role == RO_SNARE) s->snd[ORB_A + r].level /= 2;   /* (hushed) */
        if (role != RO_SNARE && k != LK_HAT) orb_follow_set(s, r, 1);
    }
}

#endif
