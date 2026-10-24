#!/bin/sh
# Regression: ffi.C.setVelX/Y/Z from a genome must be declared (scripts/bicycle.lua cdef) and must move the entity
# (Kinematics raises TERM, else run()'s clamp zeroes the new velocity). Runs a copy of the test game whose hero
# walks left with setVelX and checks its x drops.   tests/velx_test.sh <executable> <source dir>   (ctest: set-vel-x)
set -eu
exe=$(realpath "$1"); src=$(realpath "$2")
dir=$(mktemp -d); trap 'rm -rf "$dir"' EXIT
for f in "$src"/*; do ln -s "$f" "$dir/"; done
rm "$dir/test"; cp -r "$src/test" "$dir/test"
hero="$dir/test/genome/hero.lua"
sed -i 's/ffi\.C\.setVelocity(self\.id, -self\.stepSize \* self\.STEP, 0)/ffi.C.setVelX(self.id, -self.stepSize * self.STEP)/' "$hero"
grep -q 'setVelX(self.id, -self.stepSize' "$hero" || { echo "set-vel-x: hero.lua changed; update this test"; exit 1; }
sed -i 's/^function hero:onTimer(timerId, timeoutType)/&\n  io.stderr:write("VELX_POS ", box(self.id).pos.x, "\\n")/' "$hero"
cd "$dir"
BICYCLE_SCRIPTED_INPUT="a@10-60" BICYCLE_MAX_FRAMES=70 BICYCLE_WINDOW_SIZE=320x180 TERM=xterm timeout 120 \
  script -qec "'$exe' test 2>err.log" /dev/null >/dev/null 2>&1 || true
if grep -aq "missing declaration" err.log; then grep -a "missing declaration" err.log; exit 1; fi
first=$(grep -a VELX_POS err.log | head -1 | cut -d' ' -f2); last=$(grep -a VELX_POS err.log | tail -1 | cut -d' ' -f2)
[ -n "$first" ] && [ -n "$last" ] || { echo "set-vel-x: no hero position logged"; tail -c 2000 err.log; exit 1; }
[ "$last" -lt "$first" ] || { echo "set-vel-x: hero did not move left ($first -> $last)"; exit 1; }
echo "set-vel-x: hero x $first -> $last"
