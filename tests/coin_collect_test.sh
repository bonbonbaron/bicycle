#!/bin/sh
# Regression: picking up plat3d's coins (W.Body:remove from inside onCollision) must not deliver the coin's own,
# still-queued half of the overlap to it ("Collision recorded for nonexistent entity N"), in either scene, while
# walking back and forth. PLAT3D_AUTOCOLLECT puts the hero on a remaining coin every few frames.
#   tests/coin_collect_test.sh <executable>   (ctest: coin-collect)
set -eu
exe=$1
out=$(mktemp)
trap 'rm -f "$out"' EXIT
export PLAT3D_AUTOCOLLECT=8 BICYCLE_SCENE_CYCLE=8:courtyard:bonus:100 BICYCLE_MAX_FRAMES=1100 BICYCLE_WINDOW_SIZE=320x180
run() { timeout 300 "$@" >"$out" 2>&1 || { echo "coin-collect: the game failed"; tail -c 3000 "$out"; exit 1; }; }
if [ -z "${DISPLAY:-}" ]; then
  command -v xvfb-run >/dev/null || { echo "coin-collect: no DISPLAY and no xvfb-run; skipping"; exit 77; }
  run xvfb-run -a -s '-screen 0 1280x720x24' "$exe" plat3d
else
  run "$exe" plat3d
fi
if grep -aq 'nonexistent entity' "$out"; then echo "coin-collect: event delivered to a removed entity"; tail -c 3000 "$out"; exit 1; fi
grep -aq 'plat3d coin 18' "$out" || { echo "coin-collect: not all 18 coins collected"; grep -a 'plat3d coin' "$out" | tail -3; tail -c 2000 "$out"; exit 1; }
echo "coin-collect: 18/18 coins, no stale events"
