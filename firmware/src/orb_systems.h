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

static void orb_mod_set(orb_sys_t *s, uint32_t k, uint8_t src, uint8_t dst, int8_t amt)
{
    s->mod[k].src = src; s->mod[k].dst = dst; s->mod[k].amt = amt;
}

#define ORB_NSYSTEM 16u
static const char *const ORB_SYSTEM_NAME[ORB_NSYSTEM] = {"KITTEN", "MILKY WAY", "HARMONICES", "GALILEAN", "TRAPPIST", "KIRKWOOD",
    "SPIRAL DRIFT", "PULSAR", "NEBULA", "ROCHE", "LAGRANGE", "ACCRETION", "GW150914", "ALPHA CEN", "SGR A*", "AFTERGLOW"};

/* KITTEN: a beat to start from, on the grid (all 1:1, so every step is where you look for it): a kick on the beat, a
 * clap's backbeat, hats in 8ths, a purring bass on the root, its fifth and 7/4; the keys a soft paw. Nothing mutates:
 * it is yours to change (STEPS: the keys; IDEA; SOUND) */
static void orb_system_kitten(orb_sys_t *s)
{
    static const int8_t md[] = {0, 0, 7, 0, 10, 12};
    orb_rdef_t A = {1, 1, 16, 4, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
    orb_rdef_t B = {1, 1, 16, 2, 4, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x3};
    orb_rdef_t C = {1, 1, 16, 8, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0xAA};
    orb_rdef_t D = {1, 1, 16, 6, 0, 0, DIR_FWD, 0, 0, 0, md, 6, 0x1};
    s->tuning = 0; s->sun = 5; s->bpm = 104; s->swing = 16;
    s->bg_prog = 0;                         /* (its bass sings 7/4: under it, the sun's own chord fits) */
    s->snd[0] = SND_PAW; s->snd[0].oct = 0; s->snd[0].level = 74;
    s->snd[1] = SND_KICK;
    s->snd[2] = SND_CLAP;
    s->snd[3] = SND_HAT;
    s->snd[4] = SND_BASS; s->snd[4].oct = 0; s->snd[4].gate = 60; s->snd[4].level = 68;   /* (under the kick) */
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->var = 20;
    s->dtime = 6; s->dfb = 30; s->dmix = 22; s->rmix = 30; s->rsize = 55;
}

/* MILKY WAY: a lullaby. A music box twinkling at 3:2 over pitched boops, a soft kick on 1 and 3, a purring bass at
 * half speed, all pentatonic (nothing can be wrong), a little swing; where the orbits meet the moons twinkle; the keys
 * mew */
static void orb_system_milky(orb_sys_t *s)
{
    static const int8_t mb[] = {4, 2, 3, 4, 1}, mc[] = {5, 7, 6, 9, 7, 5, 4}, md[] = {0, -2, 3};
    orb_rdef_t A = {1, 1, 16, 2, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
    orb_rdef_t B = {1, 1, 16, 5, 2, 0, DIR_FWD, 0, 0, 85, mb, 5, 0x1};
    orb_rdef_t C = {3, 2, 24, 7, 0, 0, DIR_FWD, 0, 0, 90, mc, 7, 0x1};
    orb_rdef_t D = {1, 2, 8, 3, 0, 0, DIR_FWD, 0, 0, 0, md, 3, 0x1};
    s->tuning = 3; s->sun = 7; s->bpm = 86; s->swing = 22;
    s->snd[0] = SND_MEW; s->snd[0].level = 58;   /* (a chord of meows adds up) */
    s->snd[1] = SND_KICK; s->snd[1].level = 88;
    s->snd[2] = SND_BOOP; s->snd[2].level = 72;
    s->snd[3] = SND_STAR; s->snd[3].oct = 2; s->snd[3].level = 70;
    s->snd[4] = SND_BASS; s->snd[4].oct = 0; s->snd[4].level = 66; s->snd[4].gate = 80;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->mutate = 10; s->var = 15;
    s->dtime = 7; s->dfb = 45; s->ping = 1; s->dmix = 32; s->rmix = 55; s->rsize = 85; s->rdecay = 74;
    orb_mod_set(s, 0, SRC_CONJ, DST_MOON, 30);
    orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 30);
    s->bg_level = 45; s->bg_prog = 2; s->bg_change = 1; s->bg_glow = 20; s->bg_silk = 45; s->bg_cut = 30;   /* under it all, */
    s->bg_halo = 65; s->bloom = 30;                                                                  /* all wet, ~16 s */
}

/* AFTERGLOW: the oldest light, under everything. The background radiation's chords on the overtone series (ORBIT: maj9,
 * m7, 6, h9, two bars each), the nebula blooming, echoes that tire, a fourth lower each time; slow orbits of glass at
 * 5:4 and a music box at 7:4, boops at 3:2, a sub at half speed; the keys a tidal bell */
static void orb_system_afterglow(orb_sys_t *s)
{
    static const int8_t mb[] = {4, 6, 7, 9, 11}, mc[] = {8, 10, 12, 14, 11, 9, 13}, md[] = {2, 4, 6, 4};
    orb_rdef_t A = {1, 2, 8, 2, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
    orb_rdef_t B = {5, 4, 20, 5, 0, 0, DIR_FWD, 0, 0, 90, mb, 5, 0x1};
    orb_rdef_t C = {7, 4, 28, 7, 2, 0, DIR_FWD, 0, 0, 85, mc, 7, 0};
    orb_rdef_t D = {3, 2, 24, 4, 1, 0, DIR_FWD, 0, 0, 80, md, 4, 0};
    s->tuning = 1; s->sun = 2; s->bpm = 70;
    s->snd[0] = SND_TIDAL; s->snd[0].oct = 0; s->snd[0].level = 58;
    s->snd[1] = SND_SUB; s->snd[1].oct = -1; s->snd[1].level = 70; s->snd[1].gate = 90;
    s->snd[2] = SND_GLASS; s->snd[2].oct = 1; s->snd[2].level = 64;
    s->snd[3] = SND_STAR; s->snd[3].oct = 2; s->snd[3].level = 60;
    s->snd[4] = SND_BOOP; s->snd[4].oct = 1; s->snd[4].level = 58;
    orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
    orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
    s->mutate = 12; s->var = 15;
    s->dtime = 7; s->dfb = 55; s->ping = 1; s->dmix = 30; s->rmix = 60; s->rsize = 95; s->rdecay = 80; s->rdamp = 50;
    s->macro[MAC_GRAVITY] = 60; s->macro[MAC_ORBIT] = 20;
    s->bg_level = 48; s->bg_prog = 2; s->bg_change = 1; s->bg_glow = 30; s->bg_silk = 40; s->bg_width = 65; s->bg_cut = 35;
    s->bg_halo = 80; s->bloom = 55; s->tired = 5;                                                  /* (all wet, ~24 s) */
    orb_mod_set(s, 0, SRC_LFO2, DST_ANG, 30);
    orb_mod_set(s, 1, SRC_CONJ, DST_MOON, 30);
}

static void orb_system(uint32_t i, orb_sys_t *s)
{
    orb_sys_clear(s, ORB_SYSTEM_NAME[i % ORB_NSYSTEM]);
    if (i % ORB_NSYSTEM == 0u) { orb_system_kitten(s); return; }
    if (i % ORB_NSYSTEM == 1u) { orb_system_milky(s); return; }
    if (i % ORB_NSYSTEM == 15u) { orb_system_afterglow(s); return; }
    switch (i % ORB_NSYSTEM - 2u) {           /* (the sky's: HARMONICES 0 ..) */
    case 0: {   /* HARMONICES: 4, 5, 6, 7 hits a bar on the 4:5:6:7 chord, in the overtone scale (OTONAL) */
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
        s->dtime = 6; s->dfb = 40; s->ping = 1; s->dmix = 36; s->rmix = 50; s->rsize = 70; s->rdecay = 66;
        s->macro[MAC_GRAVITY] = 40; s->macro[MAC_ORBIT] = 15;
        orb_mod_set(s, 0, SRC_ORBX, DST_ECC, 30);
        orb_mod_set(s, 1, SRC_LFO2, DST_ANG, 40);
        orb_mod_set(s, 2, SRC_CONJ, DST_MOON, 35);
        orb_mod_set(s, 3, SRC_CONJ, DST_TIDE, 30);          /* where the orbits meet, they tug at each other */
        break;
    }
    case 1: {   /* GALILEAN: Io, Europa, Ganymede's 1:2:4: four on the floor, a bass in 8ths, a pad, hats that avoid the kick */
        static const int8_t mb[] = {0, 0, 7, 0, 10, 3, 0, 5}, mc[] = {0, -2};
        orb_rdef_t A = {1, 1, 16, 4, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0};
        orb_rdef_t B = {1, 2, 8, 5, 1, 0, DIR_FWD, 0, 0, 0, mb, 8, 0x5};
        orb_rdef_t C = {1, 4, 4, 1, 0, 4, DIR_FWD, 0, 0, 0, mc, 2, 0};
        orb_rdef_t D = {2, 1, 32, 13, 2, 0, DIR_FWD, LOG_NOT, 0, 92, 0, 0, 0x2492};
        s->tuning = 0; s->sun = 5; s->bpm = 124; s->swing = 18; s->hum = 8;
        s->snd[0] = SND_LEAD;
        s->snd[1] = SND_KICK;
        s->snd[2] = SND_BASS; s->snd[2].oct = 0; s->snd[2].gate = 45; s->snd[2].d = 44; s->snd[2].s = 30;
        s->snd[3] = SND_PAD; s->snd[3].level = 52; s->snd[3].oct = 1;
        s->snd[4] = SND_HAT;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->dtime = 6; s->dfb = 30; s->dmix = 28; s->rmix = 34;
        s->cut = 82; s->res = 30; s->fmode = 0;
        s->orate = 2; s->orad = 30;
        orb_mod_set(s, 0, SRC_ORBY, DST_CUT, 40);
        orb_mod_set(s, 1, SRC_LFO1, DST_SPR, 25);
        break;
    }
    case 2: {   /* TRAPPIST: a resonant chain, 1:1 3:2 5:3 8:5, glassy plucks that mutate */
        static const int8_t ma[] = {0, 2, 4, 2, 7}, mb[] = {4, 6, 4, 7, 2, 9, 6}, mc[] = {7, 9, 11}, md[] = {11, 9, 7, 4, 2};
        orb_rdef_t A = {1, 1, 16, 5, 0, 0, DIR_FWD, 0, 0, 0, ma, 5, 0x1};
        orb_rdef_t B = {3, 2, 24, 7, 2, 0, DIR_FWD, 0, 0, 0, mb, 7, 0};
        orb_rdef_t C = {5, 3, 15, 5, 1, 0, DIR_PING, 0, 0, 85, mc, 3, 0};
        orb_rdef_t D = {8, 5, 16, 5, 3, 0, DIR_FWD, LOG_NOT, 1, 80, md, 5, 0};
        s->tuning = 1; s->sun = 2; s->bpm = 104;
        s->snd[0] = SND_GLASS; s->snd[0].oct = 1;
        s->snd[1] = SND_PLUCK; s->snd[1].oct = 0;
        s->snd[2] = SND_GLASS;
        s->snd[3] = SND_BELL; s->snd[3].mratio = 3;
        s->snd[4] = SND_PIXEL; s->snd[4].pixel = 40; s->snd[4].oct = 2; s->snd[4].level = 50;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->snd[0].level = 80; s->snd[1].level = 88; s->snd[2].level = 84; s->snd[3].level = 80; s->snd[4].level = 66;
        s->mutate = 35; s->var = 30;
        s->dtime = 4; s->dfb = 45; s->ping = 1; s->dmix = 40; s->rmix = 52;
        orb_mod_set(s, 0, SRC_LFO1, DST_ECC, 20);
        orb_mod_set(s, 1, SRC_STEP, DST_ANG, 30);
        break;
    }
    case 3: {   /* KIRKWOOD: the asteroid gaps. A is Jupiter (1:3); B, C, D go 3, 5/2 and 2 times as fast: the gaps' ratios.
                 * Pixel percussion over a pentatonic blip melody */
        static const int8_t mb[] = {0, 2, 4, 3, 1, 4, 2}, mc[] = {0, 0, 5}, md[] = {4, 3, 2, 0, 1};
        orb_rdef_t A = {1, 3, 4, 3, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
        orb_rdef_t B = {1, 1, 16, 7, 1, 5, DIR_FWD, 0, 0, 90, mb, 7, 0x11};
        orb_rdef_t C = {5, 6, 10, 4, 2, 0, DIR_FWD, LOG_NOT, 0, 85, mc, 3, 0x9};
        orb_rdef_t D = {2, 3, 8, 5, 0, 5, DIR_SPIRAL, 0, 0, 85, md, 5, 0};
        s->tuning = 3; s->sun = 7; s->bpm = 96;
        s->snd[0] = SND_PIXEL; s->snd[0].oct = 1; s->snd[0].s = 55; s->snd[0].d = 56; s->snd[0].r = 44;
        s->snd[0].pixel = 50; s->snd[0].level = 64;
        s->snd[1] = SND_KICK; s->snd[1].pixel = 30;
        s->snd[2] = SND_PIXEL; s->snd[2].oct = 1; s->snd[2].d = 48; s->snd[2].level = 70;
        s->snd[3] = SND_SNARE; s->snd[3].pixel = 55; s->snd[3].color = 64;
        s->snd[4] = SND_PIXEL; s->snd[4].oct = 2; s->snd[4].sling = 60; s->snd[4].d = 44; s->snd[4].level = 58;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->macro[MAC_FRACT] = 35; s->mutate = 20; s->var = 25;
        s->dtime = 4; s->dfb = 42; s->ping = 1; s->dmix = 32; s->rmix = 30; s->rsize = 45;
        s->cut = 86; s->res = 40; s->fmode = 4;
        orb_mod_set(s, 0, SRC_RND, DST_PIXEL, 30);
        orb_mod_set(s, 1, SRC_LFO1, DST_CUT, 20);
        break;
    }
    case 4: {   /* SPIRAL DRIFT: the drawing's pattern. 16 24 20 28 at 1:1 3:2 5:4 7:5, all SPIRAL, PROB 74, MUTATE 42 */
        static const int8_t ma[] = {0, 0, 4, 0, 7}, mb[] = {7, 9, 12, 9}, mc[] = {4, 2, 4, 7, 5}, md[] = {10, 12, 14, 12};
        orb_rdef_t A = {1, 1, 16, 5, 0, 0, DIR_SPIRAL, 0, 0, 74, ma, 5, 0x1};
        orb_rdef_t B = {3, 2, 24, 8, 1, 0, DIR_SPIRAL, 0, 0, 74, mb, 4, 0};
        orb_rdef_t C = {5, 4, 20, 7, 2, 0, DIR_SPIRAL, 0, 0, 74, mc, 5, 0};
        orb_rdef_t D = {7, 5, 28, 9, 3, 0, DIR_SPIRAL, 0, 0, 74, md, 4, 0};
        s->tuning = 0; s->sun = 4; s->bpm = 120;
        s->snd[0] = SND_LEAD;
        s->snd[1] = SND_BASS;
        s->snd[2] = SND_GLASS; s->snd[2].oct = 1;
        s->snd[3] = SND_PLUCK;
        s->snd[4] = SND_BELL; s->snd[4].mratio = 11; s->snd[4].level = 50;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->mutate = 42; s->hum = 22; s->var = 37;
        s->dtime = 4; s->dfb = 48; s->ping = 1; s->dmix = 38; s->rmix = 55; s->rsize = 80; s->rdecay = 72;
        s->macro[MAC_ORBIT] = 30; s->macro[MAC_SPECTRAL] = 10;
        s->ox = 74; s->oy = 38; s->orad = 26;
        orb_mod_set(s, 0, SRC_ORBX, DST_ECC, 35);
        orb_mod_set(s, 1, SRC_ORBY, DST_SPR, 40);
        orb_mod_set(s, 2, SRC_RND, DST_MOON, 25);
        orb_mod_set(s, 3, SRC_LFO1, DST_ANG, 30);
        orb_mod_set(s, 4, SRC_CONJ, DST_TIDE, 60);
        s->snd[2].tide = 70; s->snd[3].tide = 70; s->snd[4].tide = 80;
        break;
    }
    case 5: {   /* PULSAR: drums. An infall kick, a dust snare, pixel hats that keep out of the kick's way, a bass */
        static const int8_t md[] = {0, 0, 12, 7, 0, 10};
        orb_rdef_t A = {1, 1, 16, 4, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0};
        orb_rdef_t B = {1, 1, 16, 2, 4, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x3};
        orb_rdef_t C = {2, 1, 32, 11, 1, 0, DIR_FWD, LOG_NOT, 0, 90, 0, 0, 0x111};
        orb_rdef_t D = {1, 2, 8, 3, 1, -12, DIR_FWD, 0, 0, 0, md, 6, 0x1};
        s->tuning = 0; s->sun = 9; s->bpm = 128; s->swing = 24; s->hum = 12;
        s->snd[0] = SND_PIXEL; s->snd[0].oct = 1;
        s->snd[1] = SND_KICK;
        s->snd[2] = SND_SNARE; s->snd[2].beam = 55; s->snd[2].mratio = 4;
        s->snd[3] = SND_HAT; s->snd[3].pixel = 35;
        s->snd[4] = SND_BASS; s->snd[4].oct = 1; s->snd[4].sling = 55; s->snd[4].gate = 40;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->macro[MAC_FRACT] = 25;
        s->dtime = 5; s->dfb = 25; s->dmix = 20; s->rmix = 24; s->rsize = 35;
        s->drive = 20;
        orb_mod_set(s, 0, SRC_LFO1, DST_SLING, 25);
        break;
    }
    case 6: {   /* NEBULA: slow and wide, UTONAL, moons, a deep NEBULA; long polymeter cycles */
        static const int8_t ma[] = {0, -1, 0, -3}, mb[] = {2, 4, 3}, mc[] = {4, 6, 5, 7}, md[] = {7, 9, 8};
        orb_rdef_t A = {1, 3, 5, 2, 0, 0, DIR_FWD, 0, 0, 0, ma, 4, 0x1};
        orb_rdef_t B = {1, 2, 7, 3, 1, 0, DIR_FWD, 0, 0, 0, mb, 3, 0};
        orb_rdef_t C = {2, 3, 9, 4, 2, 0, DIR_FWD, 0, 0, 0, mc, 4, 0};
        orb_rdef_t D = {3, 4, 11, 3, 3, 0, DIR_FWD, 0, 0, 80, md, 3, 0};
        s->tuning = 2; s->sun = 3; s->bpm = 76;
        s->snd[0] = SND_PAD; s->snd[0].oct = 0;
        s->snd[1] = SND_PAD; s->snd[1].oct = 0; s->snd[1].a = 50; s->snd[1].level = 72;
        s->snd[2] = SND_PAD; s->snd[2].moon = 40; s->snd[2].mratio = 2;
        s->snd[3] = SND_BELL; s->snd[3].mratio = 12; s->snd[3].a = 30; s->snd[3].d = 72;
        s->snd[4] = SND_GLASS; s->snd[4].a = 40; s->snd[4].d = 74; s->snd[4].level = 50;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->mutate = 18; s->var = 25;
        s->dtime = 8; s->dfb = 60; s->ping = 1; s->dmix = 34; s->rmix = 72; s->rsize = 95; s->rdecay = 90; s->rdamp = 55;
        s->macro[MAC_GRAVITY] = 70; s->macro[MAC_ORBIT] = 40;
        s->cut = 78; s->res = 20;
        orb_mod_set(s, 0, SRC_LFO2, DST_ECC, 40);
        orb_mod_set(s, 1, SRC_ORBX, DST_SPR, 50);
        orb_mod_set(s, 2, SRC_ORBY, DST_CUT, 30);
        orb_mod_set(s, 3, SRC_LFO2, DST_HORIZON, 45);       /* now and then the cloud slides into the horizon */
        s->hretro = 25; s->hgrain = 70;
        break;
    }
    case 7: {   /* ROCHE: torn apart. Bells off whole ratios, dust, CHAOS */
        static const int8_t ma[] = {0, 3, 7, 10}, mb[] = {4, 6}, mc[] = {7, 8, 11}, md[] = {2, 5, 9, 12};
        orb_rdef_t A = {5, 4, 20, 6, 0, 0, DIR_FWD, 0, 0, 0, ma, 4, 0x1};
        orb_rdef_t B = {4, 3, 16, 5, 1, 0, DIR_FWD, LOG_XOR, 0, 80, mb, 2, 0};
        orb_rdef_t C = {7, 5, 14, 5, 2, 0, DIR_PING, 0, 0, 75, mc, 3, 0};
        orb_rdef_t D = {3, 2, 12, 5, 1, 0, DIR_RND, LOG_CONJ, 0, 90, md, 4, 0};
        s->tuning = 0; s->sun = 11; s->bpm = 88;
        s->snd[0] = SND_BELL; s->snd[0].mratio = 14;
        s->snd[1] = SND_BELL; s->snd[1].mratio = 11; s->snd[1].oct = 1;
        s->snd[2] = SND_BELL; s->snd[2].mratio = 12; s->snd[2].dust = 30; s->snd[2].color = 80;
        s->snd[3] = SND_BELL; s->snd[3].mratio = 15; s->snd[3].oct = 2;
        s->snd[4] = SND_GLASS; s->snd[4].mratio = 16; s->snd[4].moon = 60;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->snd[0].level = 80; s->snd[1].level = 82; s->snd[2].level = 80; s->snd[3].level = 80; s->snd[4].level = 78;
        s->macro[MAC_CHAOS] = 55; s->mutate = 30; s->var = 40;
        s->macro[MAC_TIDE] = 35;
        { uint32_t k; for (k = 0; k < ORB_NPART; k++) s->snd[k].tide = 70; }
        s->dtime = 7; s->dfb = 55; s->ping = 1; s->dmix = 40; s->rmix = 60; s->rsize = 85;
        orb_mod_set(s, 0, SRC_RND, DST_ECC, 40);
        orb_mod_set(s, 1, SRC_CONJ, DST_DUST, 50);
        break;
    }
    case 8: {   /* LAGRANGE: the keys are the sun's harmonics (HARMONIC), over a calm orbit */
        static const int8_t mc[] = {4, 2, 6, 4, 5};
        orb_rdef_t A = {1, 1, 16, 3, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0x1};
        orb_rdef_t B = {1, 2, 8, 2, 0, 0, DIR_FWD, 0, 0, 0, 0, 0, 0};
        orb_rdef_t C = {3, 2, 24, 5, 3, 0, DIR_FWD, 0, 0, 0, mc, 5, 0};
        orb_rdef_t D = {2, 1, 32, 9, 1, 0, DIR_FWD, LOG_NOT, 0, 85, 0, 0, 0};
        s->tuning = 1; s->sun = 7; s->bpm = 92; s->keymode = KM_HARMONIC;
        s->snd[0] = SND_FLUTE; s->snd[0].oct = 0;
        s->snd[1] = SND_BASS; s->snd[1].sling = 10; s->snd[1].ecc = 40;
        s->snd[2] = SND_PAD; s->snd[2].level = 54;
        s->snd[3] = SND_GLASS;
        s->snd[4] = SND_HAT; s->snd[4].level = 40; s->snd[4].color = 80;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->dtime = 9; s->dfb = 45; s->ping = 1; s->dmix = 34; s->rmix = 58; s->rsize = 78;
        s->macro[MAC_ORBIT] = 20;
        orb_mod_set(s, 0, SRC_MODW, DST_PITCH, 6);
        orb_mod_set(s, 1, SRC_LFO1, DST_PITCH, 2);
        break;
    }
    case 10: {  /* GW150914: the first merger heard. Notes chirp up from octaves below onto the overtones and ring down;
                 * every second bar the whole falls a little way into the horizon; the orbits tug where they meet */
        static const int8_t ma[] = {0, 0, 4}, mb[] = {4, 6, 7, 6}, mc[] = {2, 4, 6, 9, 7}, md[] = {0};
        orb_rdef_t A = {1, 2, 8, 2, 0, 0, DIR_FWD, 0, 0, 0, ma, 3, 0x1};
        orb_rdef_t B = {1, 1, 16, 3, 2, 8, DIR_FWD, 0, 0, 0, mb, 4, 0};
        orb_rdef_t C = {3, 2, 24, 5, 1, 0, DIR_FWD, 0, 0, 0, mc, 5, 0};
        orb_rdef_t D = {2, 1, 32, 7, 3, 0, DIR_FWD, LOG_NOT, 0, 85, md, 1, 0};
        s->tuning = 1; s->sun = 9; s->bpm = 84;
        s->snd[0] = SND_PAD; s->snd[0].insp = 25; s->snd[0].inspt = 62; s->snd[0].a = 30;
        s->snd[1] = SND_BASS; s->snd[1].insp = 62; s->snd[1].inspt = 63; s->snd[1].d = 72; s->snd[1].s = 0; s->snd[1].r = 64;
        s->snd[1].eamt = 60; s->snd[1].ed = 56; s->snd[1].gate = 99; s->snd[1].level = 92;
        s->snd[2] = SND_BELL; s->snd[2].insp = 40; s->snd[2].inspt = 50; s->snd[2].level = 70;
        s->snd[3] = SND_GLASS; s->snd[3].tide = 70; s->snd[3].level = 74;
        s->snd[4] = SND_HAT; s->snd[4].beam = 60; s->snd[4].mratio = 2; s->snd[4].level = 50;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->mutate = 15; s->var = 20;
        s->dtime = 8; s->dfb = 45; s->ping = 1; s->dmix = 34; s->rmix = 58; s->rsize = 85; s->rdecay = 76;
        s->macro[MAC_TIDE] = 20;
        s->lrate[0] = 45; s->lsync[0] = 1; s->lshape[0] = LS_SAW;   /* LFO 1: a ramp over 2 bars */
        s->hshift = -12; s->hretro = 15; s->hgrain = 55;
        orb_mod_set(s, 0, SRC_LFO1, DST_HORIZON, 32);
        orb_mod_set(s, 1, SRC_CONJ, DST_TIDE, 60);
        orb_mod_set(s, 2, SRC_ORBX, DST_ECC, 25);
        break;
    }
    case 11: {  /* ALPHA CEN: a triple star. Three rings pull hard on each other (TIDE): the three-body problem, tuned */
        static const int8_t ma[] = {0, 7, 4, 7}, mb[] = {4, 3, 5}, mc[] = {7, 10, 9, 12};
        orb_rdef_t A = {1, 1, 16, 5, 0, 0, DIR_FWD, 0, 0, 0, ma, 4, 0x1};
        orb_rdef_t B = {4, 3, 16, 5, 2, 0, DIR_FWD, 0, 0, 0, mb, 3, 0};
        orb_rdef_t C = {5, 3, 20, 6, 1, 0, DIR_PING, 0, 0, 90, mc, 4, 0};
        orb_rdef_t D = {1, 4, 4, 1, 0, -12, DIR_FWD, 0, 0, 0, 0, 0, 0};
        s->tuning = 0; s->sun = 2; s->bpm = 108;
        s->snd[0] = SND_LEAD; s->snd[0].tide = 60;
        s->snd[1] = SND_PLUCK; s->snd[1].tide = 90; s->snd[1].oct = 0; s->snd[1].level = 80;
        s->snd[2] = SND_GLASS; s->snd[2].tide = 90; s->snd[2].level = 78;
        s->snd[3] = SND_BELL; s->snd[3].tide = 90; s->snd[3].mratio = 2; s->snd[3].level = 70;
        s->snd[4] = SND_PAD; s->snd[4].tide = 99; s->snd[4].oct = 0; s->snd[4].level = 70; s->snd[4].gate = 99;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->macro[MAC_TIDE] = 50; s->macro[MAC_CHAOS] = 20; s->mutate = 25; s->var = 30;
        s->dtime = 6; s->dfb = 40; s->ping = 1; s->dmix = 30; s->rmix = 48;
        orb_mod_set(s, 0, SRC_LFO2, DST_TIDE, 50);
        orb_mod_set(s, 1, SRC_CONJ, DST_ECC, 35);
        break;
    }
    case 12: {  /* SGR A*: the black hole at the centre. Deep, slow, UTONAL; the pad's point dips into the horizon and the
                 * sound slides in, redshifted, stretched, some of it backwards */
        static const int8_t ma[] = {0, -1, 0, -3}, mb[] = {4, 5, 7}, mc[] = {2, 4}, md[] = {7, 9, 11, 8};
        orb_rdef_t A = {1, 2, 7, 3, 0, 0, DIR_FWD, 0, 0, 0, ma, 4, 0x1};
        orb_rdef_t B = {2, 3, 9, 4, 1, 0, DIR_FWD, 0, 0, 0, mb, 3, 0};
        orb_rdef_t C = {1, 3, 5, 2, 0, 0, DIR_FWD, 0, 0, 0, mc, 2, 0};
        orb_rdef_t D = {3, 4, 11, 5, 2, 0, DIR_SPIRAL, 0, 0, 80, md, 4, 0};
        s->tuning = 2; s->sun = 6; s->bpm = 70;
        s->snd[0] = SND_PAD; s->snd[0].oct = 0;
        s->snd[1] = SND_BASS; s->snd[1].insp = 30; s->snd[1].inspt = 72; s->snd[1].d = 74; s->snd[1].s = 40; s->snd[1].gate = 90;
        s->snd[2] = SND_BELL; s->snd[2].mratio = 12; s->snd[2].a = 20;
        s->snd[3] = SND_PAD; s->snd[3].level = 60; s->snd[3].moon = 45; s->snd[3].mratio = 11;
        s->snd[4] = SND_GLASS; s->snd[4].dust = 35; s->snd[4].color = 85; s->snd[4].beam = 50; s->snd[4].level = 56;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->mutate = 18; s->var = 20;
        s->macro[MAC_GRAVITY] = 75; s->macro[MAC_ORBIT] = 30;
        s->dtime = 10; s->dfb = 55; s->ping = 1; s->dmix = 30; s->rmix = 70; s->rsize = 96; s->rdecay = 88; s->rdamp = 60;
        s->ox = 50; s->oy = 40; s->orad = 40; s->orate = 1;          /* the pad's point: round once in 8 bars */
        s->hshift = -24; s->hretro = 40; s->hgrain = 70;
        orb_mod_set(s, 0, SRC_ORBY, DST_HORIZON, -60);            /* the point low: deep in */
        orb_mod_set(s, 1, SRC_LFO2, DST_ECC, 35);
        orb_mod_set(s, 2, SRC_ORBX, DST_SPR, 40);
        break;
    }
    default: {  /* ACCRETION: GRAVITY high: every note falls into tune, the rings glide */
        static const int8_t ma[] = {0, 7, 5, 3, 0, 10}, mb[] = {7, 4, 9, 7, 12}, mc[] = {4, 3}, md[] = {12, 14, 11, 16};
        orb_rdef_t A = {1, 2, 8, 6, 0, -12, DIR_FWD, 0, 0, 0, ma, 6, 0x1};
        orb_rdef_t B = {1, 1, 16, 6, 1, 0, DIR_FWD, 0, 0, 0, mb, 5, 0};
        orb_rdef_t C = {3, 4, 6, 2, 0, 0, DIR_FWD, 0, 0, 0, mc, 2, 0};
        orb_rdef_t D = {3, 2, 24, 5, 2, 0, DIR_FWD, LOG_AND, 1, 0, md, 4, 0};
        s->tuning = 0; s->sun = 1; s->bpm = 100;
        s->snd[0] = SND_LEAD; s->snd[0].ecc = 50; s->snd[0].sling = 35;
        s->snd[1] = SND_BASS; s->snd[1].oct = 1; s->snd[1].gate = 90; s->snd[1].s = 70;
        s->snd[2] = SND_PLUCK; s->snd[2].flags = SF_LEGATO; s->snd[2].gate = 85; s->snd[2].s = 40;
        s->snd[3] = SND_PAD; s->snd[3].level = 56;
        s->snd[4] = SND_BELL; s->snd[4].level = 50;
        orb_ring_set(&s->ring[0], &A); orb_ring_set(&s->ring[1], &B);
        orb_ring_set(&s->ring[2], &C); orb_ring_set(&s->ring[3], &D);
        s->macro[MAC_GRAVITY] = 88; s->mutate = 15;
        s->dtime = 6; s->dfb = 40; s->dmix = 32; s->rmix = 50;
        orb_mod_set(s, 0, SRC_CONJ, DST_ECC, 40);
        orb_mod_set(s, 1, SRC_LFO1, DST_ANG, 25);
        break;
    }
    }
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
    s->dtime = 9; s->dfb = 62; s->ping = 1; s->dmix = 46; s->rmix = 62; s->rsize = 92; s->rdecay = 84; s->rdamp = 50;
    s->hshift = -12; s->hretro = 25; s->hgrain = 60;
    s->macro[MAC_GRAVITY] = 72; s->macro[MAC_ORBIT] = 20;
    orb_mod_set(s, 0, SRC_CONJ, DST_TIDE, 30);
}
#endif
