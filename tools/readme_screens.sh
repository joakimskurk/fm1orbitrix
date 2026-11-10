#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The README's screens (docs/readme/screen_*.png) from the host: tests/scripts/readme.txt, cosmos.txt for COSMOS
# and HORIZON (ALPHA CEN's), tutorial.txt for the page's tutorial (REC)
set -e
cd "$(dirname "$0")/.."
rm -f build/host/orbitrix-*.png
sh tests/run_host.sh tests/scripts/readme.txt > /dev/null
for p in 4300:orrery 5200:steps 5900:time 6600:sound 10600:macros 11500:orbit 12100:env 12700:mod 13300:filter 13900:space \
         14500:pad 15100:keys 20100:bg 24400:cute 25700:help; do
    cp "build/host/orbitrix-${p%%:*}.png" "docs/readme/screen_${p##*:}.png"
done
rm -f build/host/orbitrix-*.png
sh tests/run_host.sh tests/scripts/cosmos.txt > /dev/null
cp build/host/orbitrix-4800.png docs/readme/screen_cosmos.png
cp build/host/orbitrix-6600.png docs/readme/screen_horizon.png
rm -f build/host/orbitrix-*.png
sh tests/run_host.sh tests/scripts/tutorial.txt > /dev/null   # (the page's tutorial: REC's count-in, recording)
cp build/host/orbitrix-3200.png docs/readme/screen_countin.png
cp build/host/orbitrix-7000.png docs/readme/screen_rec.png
python3 tools/make_header.py > /dev/null           # the header (and the repository's social preview)
echo "docs/readme: the screens, the header"
