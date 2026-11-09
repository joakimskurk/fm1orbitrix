#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# The README's screens (docs/readme/screen_*.png) from the host: tests/scripts/readme.txt, and cosmos.txt for COSMOS
# and HORIZON (GW150914's)
set -e
cd "$(dirname "$0")/.."
rm -f build/host/orbitrix-*.png
sh tests/run_host.sh tests/scripts/readme.txt > /dev/null
for p in 2700:orrery 3600:steps 4300:time 5000:sound 6500:macros 7400:orbit 8000:env 8600:mod 9200:filter 9800:space \
         10400:pad 11000:keys; do
    cp "build/host/orbitrix-${p%%:*}.png" "docs/readme/screen_${p##*:}.png"
done
rm -f build/host/orbitrix-*.png
sh tests/run_host.sh tests/scripts/cosmos.txt > /dev/null
cp build/host/orbitrix-4800.png docs/readme/screen_cosmos.png
cp build/host/orbitrix-6600.png docs/readme/screen_horizon.png
echo "docs/readme: the screens"
