#!/bin/sh
# scripts/aseprite/adddoor.lua and addsong.lua in batch mode on a copy of the test game: the drop-down choices,
# and that the scene files they write still load (as Lua) with the new door, spawn and music.
# Needs Aseprite: BICYCLE_ASEPRITE=/path/to/aseprite (else skipped).
src="$1"
A="${BICYCLE_ASEPRITE:-}"
[ -n "$A" ] && [ -x "$A" ] || { echo "aseprite-scene-scripts: set BICYCLE_ASEPRITE to run; skipping"; exit 77; }
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/test/aseprite" && cp -r "$src/test/genome" "$tmp/test/" && cp "$src/test/aseprite/house.aseprite" "$tmp/test/aseprite/"
export BICYCLE_REPO="$tmp/"
run() { "$A" -b "$@" 2>&1; }
fail() { echo "aseprite-scene-scripts: $*"; exit 1; }
out=$(run --script-param game=test --script-param list=1 --script "$src/scripts/aseprite/adddoor.lua")
echo "$out" | grep -qx "scene house" && echo "$out" | grep -qx "spawn house entry" &&
  echo "$out" | grep -qx "spawn intro from_house" || fail "door choices: $out"
out=$(run --script-param game=test --script-param list=1 --script "$src/scripts/aseprite/addsong.lua")
echo "$out" | grep -qx "song music/house.mid" && echo "$out" | grep -qx "soundfont music/meadow.sf2" || fail "song choices: $out"
run --script-param game=test --script-param scene=house --script-param to=intro --script-param spawn=from_house \
  --script-param x=140 --script-param y=36 --script-param w=6 --script-param h=10 --script-param on=press \
  --script-param return_spawn=from_intro --script "$src/scripts/aseprite/adddoor.lua" >/dev/null || fail "adddoor failed"
run --script-param game=test --script-param scene=house --script-param song=music/meadow.mid \
  --script-param transition=cut --script "$src/scripts/aseprite/addsong.lua" >/dev/null || fail "addsong failed"
out=$(run --script-param game=test --script-param list=1 --script-param to=house --script "$src/scripts/aseprite/adddoor.lua")
echo "$out" | grep -qx "spawn house from_intro" || fail "return spawn missing: $out"
f="$tmp/test/genome/scene/house.lua"
grep -q 'door = { to = "intro", spawn = "from_house", w = 6, h = 10, on = "press" }' "$f" || fail "door not written"
grep -q 'house.music = { soundfont = "music/meadow.sf2", song = "music/meadow.mid", transition = "cut" }' "$f" || fail "music not written"
# The entities layer (addentity.lua / sceneentities.lua) on the house background.
cp "$src/test/genome/scene/house.lua" "$f"
ase="$tmp/test/aseprite/house.aseprite"
out=$(run "$ase" --script-param list=1 --script "$src/scripts/aseprite/addentity.lua")
echo "$out" | grep -qx "genome coin" && ! echo "$out" | grep -qx "genome intro" || fail "genome choices: $out"
echo "$out" | grep -qx "entity hero#1 hero 20 40" && echo "$out" | grep -qx "entity door#1 door 2 36" || fail "house entities: $out"
run "$ase" --script-param sync=1 --script "$src/scripts/aseprite/addentity.lua" >/dev/null
cmp -s "$f" "$src/test/genome/scene/house.lua" || { diff "$f" "$src/test/genome/scene/house.lua"; fail "sync isn't idempotent"; }
run "$ase" --script-param genome=coin --script-param x=60 --script-param y=40 --script-param motion=spin \
  --script "$src/scripts/aseprite/addentity.lua" >/dev/null || fail "addentity failed"
grep -q '{ genome = "coin", pos = { 60, 40 }, motion = { period = 2, type = "spin" } },' "$f" || fail "entity not written"
run "$ase" --script-param to=intro --script-param spawn=from_house --script-param x=140 --script-param y=36 \
  --script-param w=6 --script-param h=10 --script "$src/scripts/aseprite/adddoor.lua" >/dev/null || fail "adddoor (slice) failed"
grep -q '{ genome = "door", pos = { 140, 36 }, door = { h = 10, spawn = "from_house", to = "intro", w = 6 } },' "$f" || fail "door slice not written"
n=$(grep -c "BEGIN entities" "$f"); [ "$n" = 1 ] || fail "$n generated blocks"
echo "aseprite-scene-scripts: OK"
