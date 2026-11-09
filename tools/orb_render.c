/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 joakimskurk */
/* ORBITRIX's engine on this computer: the systems as WAV files (the same C as the firmware, firmware/src/orb_engine.h).
 *
 *   orb_render system N SECONDS OUT.wav [keys]   system N (0..9) playing; "keys": a slow progression on the keys too
 *   orb_render tour DIR [SECONDS]                every system -> DIR/NN_name.wav
 *   orb_render hold N NOTE SECONDS OUT.wav       system N's KEYS sound, one key held, the orrery stopped
 *   orb_render sweep WHAT OUT.wav                one held note, WHAT (ecc sling ang moon dust pixel prec) 0 -> 99
 *   orb_render macro N M OUT.wav                 system N playing, macro M (0..4) 0 -> 99 -> 0 over 24 s
 *   orb_render lib DIR                           every library sound (SOUND's list) -> DIR/NN_name.wav: a beat for a
 *                                                drum, a phrase for the rest, played as the keys play a part (A's voice)
 *   orb_render bg N PROG SECONDS OUT.wav [alone] system N with the background radiation's chords PROG (0..7, its
 *                                                level as the system has it, or 45), "alone": the rings and keys quiet
 *   orb_render cutesong MOOD SEED OUT.wav        the same of a patch CUTE makes (MOOD 0..3: SLEEPY PLAYFUL SPARKLY ZOOMIES)
 *   orb_render song N OUT.wav                    a little song from system N: its chords alone, then its parts come
 *                                                in by kind (bass, beat, melodies), a break where the keys sing the
 *                                                chords' own notes, all of it, the beat out, STOP and the tail
 *   orb_render bench                             the worst case: 8 voices, everything on; ns and x86 instructions a frame
 * Each prints the render's peak, RMS and DC. (MUTATE is the engine's own, at each ring's lap.) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __linux__
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#endif
#include "orb_systems.h"

static orb_t O;
static orb_sys_t S;
static orb_mem_t MEM;
static int16_t *wav;
static uint32_t wav_n, wav_cap;

static void emit(const int32_t *b, uint32_t n)
{
    uint32_t i;
    if (wav_n + n > wav_cap) { wav_cap = (wav_n + n) * 2u; wav = realloc(wav, wav_cap * 4u); }
    for (i = 0; i < 2u * n; i++) {
        int32_t v = b[i];
        wav[2u * wav_n + i] = (int16_t)(v > 32767 ? 32767 : v < -32767 ? -32767 : v);
    }
    wav_n += n;
}

static void run_ms(uint32_t ms)
{
    static uint32_t owed;
    int32_t blk[512];
    uint32_t t;
    for (t = 0; t < ms; t++) {
        owed += 44100u;
        while (owed >= 1000u * 256u) {
            owed -= 1000u * 256u;
            memset(blk, 0, sizeof blk);
            orb_render(&O, blk, 256);
            emit(blk, 256);
        }
    }
}

static void write_wav(const char *name)
{
    FILE *f = fopen(name, "wb");
    uint32_t bytes = wav_n * 4u, v[] = {16, 0x00020001u, 44100u, 44100u * 4u, 0x00100004u}, i;
    double sum = 0, sq = 0;
    int peak = 0;
    if (!f) { perror(name); exit(1); }
    fwrite("RIFF", 1, 4, f); bytes += 36; fwrite(&bytes, 4, 1, f); bytes -= 36;
    fwrite("WAVEfmt ", 1, 8, f); fwrite(v, 4, 5, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    fwrite(wav, 4, wav_n, f);
    fclose(f);
    for (i = 0; i < 2u * wav_n; i++) {
        int a = wav[i] < 0 ? -wav[i] : wav[i];
        if (a > peak) peak = a;
        sum += wav[i];
        sq += (double)wav[i] * wav[i];
    }
    printf("%-44s %6.1f s  peak %6.1f dBFS  rms %6.1f dBFS  dc %+.5f\n", name, wav_n / 44100.0,
           20 * log10((peak + 1) / 32768.0), 10 * log10(sq / (2.0 * wav_n + 1) / (32768.0 * 32768.0) + 1e-12),
           sum / (2.0 * wav_n + 1) / 32768.0);
    wav_n = 0;
}

static void reset(uint32_t sys)
{
    orb_system(sys, &S);
    if (getenv("ORB_SOLO")) {               /* ORB_SOLO=r: ring r alone (0..3), 4: the keys alone */
        uint32_t r, so = (uint32_t)atoi(getenv("ORB_SOLO"));
        for (r = 0; r < ORB_NRING; r++) S.ring[r].mute = r != so;
        if (so < 4u) S.snd[0].level = 0;
    }
    if (getenv("ORB_NOBG")) S.bg_level = 0;  /* ORB_NOBG=1: without the background radiation */
    orb_init(&O, &S, &MEM);
}

/* the keys: a progression of just chords on the sun (SOLAR), held two bars each */
static void keys_progression(uint32_t seconds)
{
    static const int8_t CH[4][4] = {{0, 7, 16, 23}, {5, 12, 21, 28}, {-3, 4, 12, 19}, {2, 9, 17, 22}};
    uint32_t bar_ms = 4u * 60000u / S.bpm, c = 0, t = 0, k;
    while (t + 2u * bar_ms <= seconds * 1000u) {
        for (k = 0; k < 4u; k++) {
            int32_t note = 48 + (int32_t)S.sun + CH[c % 4u][k];
            if (S.keymode == KM_HARMONIC) {     /* the white keys of harmonics 4 5 6 7, then 6 7 9 10, ... */
                static const uint8_t HK[4][4] = {{6, 7, 9, 11}, {9, 11, 14, 16}, {7, 9, 11, 13}, {11, 13, 16, 18}};
                orb_note_on(&O, (uint8_t)(k + 1u), orb_key_pitch(&S, HK[c % 4u][k], 1, 0), 90);
            } else
                orb_note_on(&O, (uint8_t)(k + 1u), orb_key_pitch(&S, (uint32_t)note, 0, 0), 90);
        }
        run_ms(2u * bar_ms - 150u);
        for (k = 0; k < 4u; k++) orb_note_off(&O, (uint8_t)(k + 1u));
        run_ms(150u);
        t += 2u * bar_ms;
        c++;
    }
    run_ms(seconds * 1000u - t);
}

static uint8_t *sweep_field(const char *w, orb_snd_t *s)
{
    if (!strcmp(w, "ecc")) return &s->ecc;
    if (!strcmp(w, "sling")) return &s->sling;
    if (!strcmp(w, "ang")) return &s->ang;
    if (!strcmp(w, "moon")) return &s->moon;
    if (!strcmp(w, "dust")) return &s->dust;
    if (!strcmp(w, "pixel")) return &s->pixel;
    if (!strcmp(w, "prec")) return &s->prec;
    if (!strcmp(w, "spr")) return &s->spr;
    fprintf(stderr, "sweep: ecc sling ang moon dust pixel prec spr\n");
    exit(2);
}

/* a library sound alone, dry of the system's own: as part A on the keys' voices (how the keys play a track) */
static void lib_phrase(uint32_t li)
{
    static const int8_t MEL[8] = {0, 4, 7, 12, 9, 7, 4, 0}, BASS[8] = {0, 0, 12, 0, 7, 0, 10, 12};
    const orb_lib_t *L = &ORB_LIB[li];
    uint32_t k, kind = L->kind, step = 60000u / 112u / 2u;   /* 8ths at 112 */
    orb_system(0, &S);
    { uint32_t r; for (r = 0; r < ORB_NRING; r++) S.ring[r].mute = 1; }
    S.snd[1].pan = 50;
    orb_lib_load(&S.snd[1], li, ORB_A);
    S.dmix = 30; S.rmix = 40;
    orb_init(&O, &S, &MEM);
    run_ms(50);
    for (k = 0; k < 8u; k++) {
        int32_t semi = kind <= LK_PERC ? 0 : kind == LK_BASS ? BASS[k] : MEL[k];
        uint32_t len = kind == LK_PAD ? step * 4u : kind <= LK_PERC ? step / 3u : step * 3u / 4u;
        if (kind == LK_PAD && (k & 3u)) continue;   /* (a pad: two long notes) */
        orb_note_on_part(&O, 1, (36 + (int32_t)S.sun + semi) * 256, kind == LK_KICK && (k & 1u) ? 70 : 100, ORB_A);
        run_ms(len);
        orb_note_off(&O, 1);
        run_ms((kind == LK_PAD ? step * 4u : step) - len);
    }
    run_ms(1500);
}


/* ---- a little song: the system's parts by kind enter one after another, the keys sing the chords' own notes */
static void run_to(uint32_t tick)           /* render until the orrery's clock reaches tick */
{
    int32_t blk[512];
    uint32_t guard = 0;
    while ((int32_t)(O.T - tick) < 0 && guard++ < 1000000u) { memset(blk, 0, sizeof blk); orb_render(&O, blk, 256); emit(blk, 256); }
}
static int32_t chord_tone(uint32_t tick, uint32_t k)   /* the k-th note (from the top's fourth down) of the chord then */
{
    uint32_t t = S.tuning % ORB_NTUNING, p = S.bg_prog % ORB_NBGPROG, n = ORB_BGP_N[t][p], c, i, nu = 0;
    int32_t sun = (int32_t)(S.sun % 12u), base = (48 + 12 * S.bg_temp + sun - (sun > 6 ? 12 : 0)) * 256, up[8];
    const int16_t *V;
    c = (tick + 2u * ORB_TPS) / ((16u * ORB_TPS) << (S.bg_change & 3u)) % n;
    V = ORB_BGP_V[t][p][c];
    for (i = 1; i < ORB_NBG; i++) if (V[i] != -32768) up[nu++] = base + V[i];
    i = nu > 4u ? nu - 4u + k : k;
    return up[i < nu ? i : nu - 1u];
}
static int32_t cute_mood = -1;
static uint32_t cute_seed;
static void song(uint32_t sysn)
{
    enum { G_BASS = 1, G_BEAT = 2, G_MEL = 4, G_KEYS = 8 };
    static const uint8_t SECT[7] = {0, G_BASS, G_BASS | G_BEAT, G_BASS | G_BEAT | G_MEL, G_MEL | G_KEYS,
                                    G_BASS | G_BEAT | G_MEL | G_KEYS, G_BASS | G_MEL};
    static const uint8_t BARS[2][7] = {{2, 2, 4, 4, 4, 4, 2}, {2, 2, 2, 4, 2, 4, 2}};   /* (slow ones: shorter) */
    /* the keys' phrase, two bars of 16ths: when each note starts, how long, which chord note (0 the top's fourth) */
    static const uint8_t PH_AT[2][8] = {{0, 3, 6, 8, 16, 19, 22, 24}, {0, 6, 8, 16, 22, 24, 0, 0}};
    static const uint8_t PH_LEN[2][8] = {{3, 3, 2, 7, 3, 3, 2, 7}, {6, 2, 7, 6, 2, 7, 0, 0}};
    static const uint8_t PH_K[2][8] = {{1, 2, 3, 2, 1, 0, 1, 3}, {2, 3, 1, 1, 0, 2, 0, 0}};
    uint32_t grp[ORB_NRING], r, sec, slow, nph, bar = 0, nbeat = 0, step, off_at[4] = {0, 0, 0, 0}, nk = 0, mels = 0;
    if (cute_mood >= 0) { orb_cute(&S, (uint32_t)cute_mood, cute_seed); orb_init(&O, &S, &MEM); }
    else reset(sysn);
    slow = S.bpm < 90u;
    nph = slow ? 6u : 8u;
    for (r = 0; r < ORB_NRING; r++) {
        uint32_t k = orb_lib_kind(&S.snd[ORB_A + r]);
        grp[r] = k == LK_BASS ? G_BASS : k == LK_KICK || k == LK_SNARE || k == LK_HAT ? G_BEAT : G_MEL;
        if (grp[r] == G_BEAT) nbeat++;
    }
    if (!nbeat)                             /* (no beat: the first melody comes in where it would) */
        for (r = 0; r < ORB_NRING; r++) if (grp[r] == G_MEL && !mels++) grp[r] = G_BEAT;
    orb_play(&O, 1);
    for (sec = S.bg_level ? 0u : 1u; sec < 7u; sec++) {
        uint32_t b;
        for (b = 0; b < BARS[slow][sec]; b++, bar++) {
            for (r = 0; r < ORB_NRING; r++) S.ring[r].mute = !(SECT[sec] & grp[r]);
            for (step = 0; step < 16u; step++) {
                uint32_t s = bar * 16u + step, ph = ((b & 1u) ? 16u : 0u) + step, q;
                run_to(s * ORB_TPS > 256u ? s * ORB_TPS - 256u : 0u);
                for (q = 0; q < 4u; q++) if (off_at[q] && off_at[q] <= s) { orb_note_off(&O, (uint8_t)(q + 1u)); off_at[q] = 0; }
                if (SECT[sec] & G_KEYS)
                    for (q = 0; q < nph; q++)
                        if (PH_AT[slow][q] == ph) {
                            uint32_t id = nk++ % 3u;
                            if (off_at[id]) orb_note_off(&O, (uint8_t)(id + 1u));
                            orb_note_on(&O, (uint8_t)(id + 1u), chord_tone(s * ORB_TPS, PH_K[slow][q]), q & 1u ? 84 : 100);
                            off_at[id] = s + PH_LEN[slow][q];
                        }
            }
        }
    }
    run_to(bar * 16u * ORB_TPS - 256u);
    for (r = 0; r < 4u; r++) orb_note_off(&O, (uint8_t)(r + 1u));
    orb_play(&O, 0);
    {   /* the tail: until the chords and their halo are silent (at most 40 s), and a breath more */
        uint32_t ms = 0;
        while ((O.bg_on || O.g_on) && ms < 40000u) { run_ms(100); ms += 100u; }
        run_ms(1500);
    }
}

int main(int argc, char **argv)
{
    if (argc >= 5 && !strcmp(argv[1], "system")) {
        reset((uint32_t)atoi(argv[2]));
        orb_play(&O, 1);
        if (argc > 5 && !strcmp(argv[5], "keys")) keys_progression((uint32_t)atoi(argv[3]));
        else run_ms((uint32_t)atoi(argv[3]) * 1000u);
        orb_play(&O, 0);
        run_ms(1500);
        write_wav(argv[4]);
    } else if (argc >= 6 && !strcmp(argv[1], "bg")) {
        uint32_t r;
        orb_system((uint32_t)atoi(argv[2]), &S);
        S.bg_prog = (uint8_t)(atoi(argv[3]) % ORB_NBGPROG);
        if (!S.bg_level) S.bg_level = 45;
        if (getenv("ORB_BGLEVEL")) S.bg_level = (uint8_t)atoi(getenv("ORB_BGLEVEL"));
        if (getenv("ORB_HALO")) S.bg_halo = (uint8_t)atoi(getenv("ORB_HALO"));
        if (argc > 6 && !strcmp(argv[6], "alone")) {
            for (r = 0; r < ORB_NRING; r++) S.ring[r].mute = 1;
            S.snd[0].level = 0;
        }
        orb_init(&O, &S, &MEM);
        orb_play(&O, 1);
        run_ms((uint32_t)atoi(argv[4]) * 1000u);
        orb_play(&O, 0);
        run_ms(4000u + (S.bg_halo ? ORB_HALO_SEC[S.bg_halo % 100u] * 500u : 0u));   /* (HALO: its tail too) */
        write_wav(argv[5]);
    } else if (argc >= 5 && !strcmp(argv[1], "cutesong")) {   /* a song of a CUTE patch: mood, seed */
        cute_mood = atoi(argv[2]) % ORB_NMOOD;
        cute_seed = (uint32_t)atoi(argv[3]);
        song(0);
        printf("cute %s, seed %u: %u bpm, %s, %s, %s / %s / %s / %s, keys %s\n", ORB_MOOD_NAME[cute_mood], cute_seed, S.bpm,
               ORB_TUNING_NAME[S.tuning], ORB_BGPROG_NAME[S.bg_prog], orb_lib_name(&S.snd[1]), orb_lib_name(&S.snd[2]),
               orb_lib_name(&S.snd[3]), orb_lib_name(&S.snd[4]), orb_lib_name(&S.snd[0]));
        write_wav(argv[4]);
    } else if (argc >= 4 && !strcmp(argv[1], "song")) {
        song((uint32_t)atoi(argv[2]));
        write_wav(argv[3]);
    } else if (argc >= 3 && !strcmp(argv[1], "tour")) {
        uint32_t i, secs = argc > 3 ? (uint32_t)atoi(argv[3]) : 16u;
        for (i = 0; i < ORB_NSYSTEM; i++) {
            char name[512], low[16];
            uint32_t k;
            for (k = 0; ORB_SYSTEM_NAME[i][k] && k < 15u; k++) {
                char ch = ORB_SYSTEM_NAME[i][k];
                low[k] = (ch >= 'A' && ch <= 'Z') ? (char)(ch + 32) : (ch >= '0' && ch <= '9') ? ch : '_';   /* (a file name) */
            }
            low[k] = 0;
            snprintf(name, sizeof name, "%s/%02u_%s.wav", argv[2], i, low);
            reset(i);
            orb_play(&O, 1);
            keys_progression(secs);
            orb_play(&O, 0);
            run_ms(1500);
            write_wav(name);
        }
    } else if (argc >= 6 && !strcmp(argv[1], "hold")) {
        reset((uint32_t)atoi(argv[2]));
        run_ms(100);
        orb_note_on(&O, 1, orb_key_pitch(&S, (uint32_t)atoi(argv[3]), 0, 0), 100);
        run_ms((uint32_t)atoi(argv[4]) * 1000u);
        orb_note_off(&O, 1);
        run_ms(2000);
        write_wav(argv[5]);
    } else if (argc >= 4 && !strcmp(argv[1], "sweep")) {
        uint32_t v;
        uint8_t *f;
        reset(0);
        S.snd[0] = SND_LEAD;
        S.snd[0].eamt = 0; S.snd[0].a = 10; S.snd[0].s = 99; S.snd[0].r = 40; S.snd[0].send = 0; S.snd[0].prec = 0;
        S.rmix = 0; S.dmix = 0;
        f = sweep_field(argv[2], &S.snd[0]);
        *f = 0;
        if (!strcmp(argv[2], "moon")) S.snd[0].mratio = 13;
        orb_note_on(&O, 1, orb_key_pitch(&S, 45, 0, 0), 100);
        for (v = 0; v < 100u; v++) { *f = (uint8_t)v; run_ms(120); }
        orb_note_off(&O, 1);
        run_ms(1000);
        write_wav(argv[3]);
    } else if (argc >= 5 && !strcmp(argv[1], "macro")) {
        uint32_t m = (uint32_t)atoi(argv[3]) % ORB_NMACRO, v, base;
        reset((uint32_t)atoi(argv[2]));
        base = S.macro[m];
        orb_play(&O, 1);
        for (v = 0; v < 100u; v++) { S.macro[m] = (uint8_t)(base + (99u - base) * v / 99u); run_ms(120); }
        for (v = 100; v-- > 0;) { S.macro[m] = (uint8_t)(base + (99u - base) * v / 99u); run_ms(120); }
        orb_play(&O, 0);
        run_ms(1500);
        write_wav(argv[4]);
    } else if (argc >= 3 && !strcmp(argv[1], "lib")) {
        uint32_t li;
        for (li = 1; li < ORB_NLIB; li++) {
            char name[512], low[16];
            uint32_t k;
            for (k = 0; ORB_LIB[li].name[k] && k < 15u; k++) low[k] = (char)(ORB_LIB[li].name[k] + 32);
            low[k] = 0;
            snprintf(name, sizeof name, "%s/%02u_%s.wav", argv[2], li, low);
            lib_phrase(li);
            write_wav(name);
        }
    } else if (argc >= 2 && !strcmp(argv[1], "bench")) {
        /* the worst case: every voice sounding with moon, sling, dust, pixel, TIDE and BEAM; the filter, delay, NEBULA
         * and HORIZON on */
        uint32_t k, blocks = getenv("ORB_BLOCKS") ? (uint32_t)atoi(getenv("ORB_BLOCKS")) : 2000u;
        int32_t blk[512];
        clock_t t0;
        double ns;
        reset(4);
        for (k = 0; k < ORB_NPART; k++) {
            orb_snd_t *s = &S.snd[k];
            s->moon = 50; s->sling = 50; s->dust = 30; s->pixel = 10; s->a = 0; s->s = 99; s->d = 60; s->gate = 99;
            s->send = 60; s->tide = 99; s->beam = 50;
        }
        S.cut = 60; S.res = 50; S.drive = 30;
        S.macro[MAC_TIDE] = 99; S.macro[MAC_HORIZON] = 60; S.hretro = 30;
        S.bg_level = 80; S.bg_glow = 80; S.bg_cut = 30; S.bg_width = 60; S.bg_silk = 40; S.bg_prog = 7; S.bloom = 80; S.tired = 6;   /* (the cosmic ones) */
        orb_play(&O, 1);
        for (k = 0; k < 4u; k++) orb_note_on(&O, (uint8_t)(k + 1u), orb_key_pitch(&S, 50u + 4u * k, 0, 0), 100);
        for (k = 0; k < 200u; k++) { memset(blk, 0, sizeof blk); orb_render(&O, blk, 256); }
        t0 = clock();
        for (k = 0; k < blocks; k++) { memset(blk, 0, sizeof blk); orb_render(&O, blk, 256); }
        ns = (double)(clock() - t0) / CLOCKS_PER_SEC * 1e9 / (blocks * 256.0);
#ifdef __linux__
        {   /* instructions a frame (Dinghy's yardstick: sloopDX's engines fit at ~1000-1500 here) */
            struct perf_event_attr a;
            long fd;
            long long c = 0;
            memset(&a, 0, sizeof a);
            a.type = PERF_TYPE_HARDWARE; a.size = sizeof a; a.config = PERF_COUNT_HW_INSTRUCTIONS;
            a.exclude_kernel = 1; a.exclude_hv = 1; a.disabled = 1;
            fd = syscall(SYS_perf_event_open, &a, 0, -1, -1, 0);
            if (fd >= 0) {
                ioctl(fd, PERF_EVENT_IOC_RESET, 0); ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
                for (k = 0; k < blocks; k++) { memset(blk, 0, sizeof blk); orb_render(&O, blk, 256); }
                ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
                if (read(fd, &c, sizeof c) == sizeof c) printf("bench: %.0f x86 instructions a frame\n", (double)c / (blocks * 256.0));
            } else printf("bench: no perf counter here\n");
        }
#endif
        for (k = 0; k < ORB_NV; k++) if (!O.v[k].stage) printf("bench: voice %u silent\n", k);
        printf("bench: %.1f ns a frame on this computer (8 voices, everything on)\n", ns);
    } else {
        fprintf(stderr, "usage: see the top of tools/orb_render.c\n");
        return 2;
    }
    return 0;
}
