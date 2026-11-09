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
