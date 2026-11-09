#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 joakimskurk
# ORBITRIX's engine on this computer: the tests (twice: the second time under ASan/UBSan), the renders to listen to
# (build/audio), their pictures (build/look), the bench.
#   tests/run_orb.sh             everything
#   ORB_ASAN=0                   skip the sanitizers' run
set -e
cd "$(dirname "$0")/.."
mkdir -p build/orb build/audio/tour build/audio/sweep build/audio/macro
CFLAGS="-O2 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Ifirmware/src"
python3 tools/gen_tables.py >/dev/null
cc $CFLAGS -o build/orb/orb_test tests/orb_test.c -lm
cc $CFLAGS -o build/orb/orb_render tools/orb_render.c -lm
build/orb/orb_test
if [ "${ORB_ASAN:-1}" != 0 ]; then            # again with the sanitizers: overflow, a shift of a negative, memory
    SAN="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wno-unused-function -Ifirmware/src"
    cc $SAN -o build/orb/orb_test_asan tests/orb_test.c -lm
    build/orb/orb_test_asan > build/orb/orb_test_asan.txt 2>&1 || { tail -20 build/orb/orb_test_asan.txt; exit 1; }
    cc $SAN -o build/orb/orb_render_asan tools/orb_render.c -lm
    for i in 0 1 2 3 4 5 6 7 8 9 10 11 12; do      # every system with the keys, every macro on three of them
        build/orb/orb_render_asan system $i 12 build/orb/asan.wav keys > build/orb/asan.txt 2>&1 ||
            { tail -20 build/orb/asan.txt; exit 1; }
    done
    for i in 4 7 10; do for m in 0 1 2 3 4 5 6; do
        build/orb/orb_render_asan macro $i $m build/orb/asan.wav > build/orb/asan.txt 2>&1 || { tail -20 build/orb/asan.txt; exit 1; }
    done; done
    rm -f build/orb/asan.wav
    echo "orb_test and the renders: clean under the sanitizers"
fi
rm -f build/audio/tour/*.wav build/audio/sweep/*.wav build/audio/macro/*.wav
build/orb/orb_render tour build/audio/tour 16           # every system, with a slow progression on the keys
for w in ecc sling ang moon dust pixel prec spr; do      # one held note, each orbit parameter 0 -> 99 (12 s)
    build/orb/orb_render sweep $w build/audio/sweep/sweep_$w.wav
done
for m in 0 1 2 3 4 5 6; do                               # SPIRAL DRIFT, each macro up and down (24 s)
    build/orb/orb_render macro 4 $m build/audio/macro/macro_$m.wav
done
python3 tools/orb_look.py build/audio/tour/*.wav > build/orb/look.txt
build/orb/orb_render bench
