#!/bin/sh
# Scene changes don't leak: makes N scene changes intro -> house -> intro ... headless (BICYCLE_SCENE_CYCLE in
# scripts/Scene.lua) and checks every unload leaves nothing in the SCENE pool and its high-water mark stays flat.
#   tests/scene_lifecycle_test.sh <executable> <ascii|2d|3d> [cycles] [game] [sceneA:sceneB]   (ctest: scene-lifecycle)
# (defaults: the test game, intro:house; plat3d uses courtyard:bonus.)
# Under ASan (-DCMAKE_CXX_FLAGS=-fsanitize=address) a leak or use-after-free also fails the run.
set -eu
exe=$1
mode=$2
n=${3:-10}
game=${4:-test}
pair=${5:-intro:house}
out=$(mktemp)
trap 'rm -f "$out"' EXIT
export BICYCLE_SCENE_CYCLE="$n:$pair:24" BICYCLE_SCENE_STATS=1 BICYCLE_MAX_FRAMES=$(( n * 60 + 300 )) TERM="${TERM:-xterm-256color}"
case "$TERM" in dumb|"") TERM=xterm-256color ;; esac
case "$mode" in
  ascii)
    command -v script >/dev/null || { echo "scene-lifecycle: needs script(1) for a pty; skipping"; exit 77; }
    timeout 300 script -qec "$exe $game" /dev/null >"$out" 2>&1 || { echo "scene-lifecycle: the game failed"; tail -c 3000 "$out"; exit 1; } ;;
  *)
    export BICYCLE_WINDOW_SIZE=320x180
    if [ -z "${DISPLAY:-}" ]; then
      command -v xvfb-run >/dev/null || { echo "scene-lifecycle: no DISPLAY and no xvfb-run; skipping"; exit 77; }
      timeout 300 xvfb-run -a -s '-screen 0 1280x720x24' "$exe" "$game" >"$out" 2>&1 || { echo "scene-lifecycle: the game failed"; tail -c 3000 "$out"; exit 1; }
    else
      timeout 300 "$exe" "$game" >"$out" 2>&1 || { echo "scene-lifecycle: the game failed"; tail -c 3000 "$out"; exit 1; }
    fi ;;
esac
tr -d '\r' <"$out" | grep -ao 'bicycle scene: [a-z]* [a-z_]* scene_hw=[0-9]* scene_live=[0-9]* leaked=[0-9]*[^|+]*' >"$out.s" || true
unloads=$(grep -c ' unload ' "$out.s" || true)
if [ "$unloads" -lt "$n" ]; then echo "scene-lifecycle: only $unloads scene changes (want $n)"; cat "$out.s"; tail -c 3000 "$out"; rm -f "$out.s"; exit 1; fi
bad=$(grep ' unload ' "$out.s" | grep -v 'scene_live=0 leaked=0' || true)
[ -z "$bad" ] || { echo "scene-lifecycle: SCENE memory left after unload:"; echo "$bad"; rm -f "$out.s"; exit 1; }
# The first round trip may still raise the mark (load order); after that it must not move.
hws=$(sed -n 's/.*scene_hw=\([0-9]*\).*/\1/p' "$out.s" | tail -n +5 | sort -u | wc -l)
[ "$hws" -le 1 ] || { echo "scene-lifecycle: SCENE high-water kept growing:"; cat "$out.s"; rm -f "$out.s"; exit 1; }
a=${pair%%:*}
gen=$(grep " unload $a " "$out.s" | sed -n 's/.*general_live=\([0-9]*\).*/\1/p' | sort -u | wc -l)
[ "$gen" -le 1 ] || { echo "scene-lifecycle: GENERAL grows per cycle:"; cat "$out.s"; rm -f "$out.s"; exit 1; }
echo "scene-lifecycle: OK, $unloads changes; $(tail -n 1 "$out.s")"
rm -f "$out.s"
