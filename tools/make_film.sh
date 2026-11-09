#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
# A film of the firmware on the host: the screen at 25 frames a second (3x, nearest), the sound as it plays.
#   tools/make_film.sh [script] [out.mp4]     default: the legend of Astralica (tests/scripts/intro.txt, ORB_INTRO=1)
set -e
cd "$(dirname "$0")/.."
SCRIPT="${1:-tests/scripts/intro.txt}"
OUT="${2:-build/film/astralica.mp4}"
mkdir -p build/film build/host
rm -f build/film/f-*.ppm
cc -O2 -Wall -Wno-unused-function -DDINGHY_HOST -DAPP_VER="\"${APP_VER:-dev}\"" -Ifirmware/hal -Ifirmware/src -Ifirmware/gen \
    -o build/host/orbitrix-film firmware/src/orbitrix.c
ORB_INTRO=1 build/host/orbitrix-film build/film/f "$SCRIPT" 25 > /dev/null
ffmpeg -loglevel error -y -framerate 25 -i build/film/f-%05d.ppm -i build/film/f.wav \
    -vf "scale=720:720:flags=neighbor,format=yuv420p" -c:v libx264 -crf 18 -c:a aac -b:a 192k -shortest "$OUT"
rm -f build/film/f-*.ppm
echo "film -> $OUT"
