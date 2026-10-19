#!/bin/sh
# tools/ids/genids.lua: ids for the repo's games, and --strict catching typos (a door to a missing scene, a
# missing spawn, an unknown sound, a bad initAnim). `ctest -R ids`.
lua="$1"; src="$2"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fail() { echo "ids: $*"; exit 1; }
# A copy of the test game.
mkdir -p "$tmp/test" "$tmp/scripts" "$tmp/include/v/common"
cp -r "$src/test/genome" "$tmp/test/" && cp "$src/test/main.lua" "$tmp/test/"
cp "$src/scripts/Genome.lua" "$src/scripts/GenomeLayout.lua" "$tmp/scripts/" && cp "$src/include/v/common/ColorPalette.h" "$tmp/include/v/common/"
list() { find "$tmp/test" "$tmp/scripts" "$tmp/include" -type f > "$tmp/list.txt"; }
gen() { "$lua" "$src/tools/ids/genids.lua" --root "$tmp" --out "$tmp" --list "$tmp/list.txt" --strict 2>"$tmp/err.txt"; }
list
gen || { cat "$tmp/err.txt"; fail "the test game should pass --strict"; }
grep -q '\["walk_up"\]=' "$tmp/ids.lua" && grep -q '\["house"\]=' "$tmp/ids.lua" && grep -q '\["from_house"\]=' "$tmp/ids.lua" &&
  grep -q '\["house"\]="music/house.mid"' "$tmp/ids.lua" && grep -q '\["collect"\]=' "$tmp/ids.lua" &&
  grep -q '\["pilgrim3d"\]="body/model/pilgrim3d.bmesh"' "$tmp/ids.lua" || fail "missing names in ids.lua"
grep -q 'enum class Anim : uint8_t' "$tmp/ids.h" && grep -q 'default_ = 0' "$tmp/ids.h" && grep -q 'hero = 1' "$tmp/ids.h" || fail "ids.h"
expect() {  # file sed-expression message
  cp "$tmp/$1" "$tmp/$1.bak"; sed -i "$2" "$tmp/$1"; list
  if gen; then fail "--strict missed: $3"; fi
  grep -q "$3" "$tmp/err.txt" || { cat "$tmp/err.txt"; fail "no '$3' in the errors"; }
  mv "$tmp/$1.bak" "$tmp/$1"
}
expect test/genome/scene/intro.lua 's/to = "house"/to = "hosue"/' "door to scene 'hosue'"
expect test/genome/scene/intro.lua 's/spawn = "entry"/spawn = "entyr"/' "spawn 'entyr'"
expect test/genome/coin.lua 's/self:sound( "collect" )/self:sound( "cllect" )/' "no sound 'cllect' in this genome"
expect test/genome/coin.lua 's/soundfont = "meadow"/soundfont = "maedow"/' "no SoundFont 'maedow'"
expect test/genome/coin.lua 's/^return coin/x = = 1\nreturn coin/' "genome doesn't load"
expect test/genome/hero.lua 's/model = "pilgrim3d"/model = "pilgrim"/' "no genome model 'pilgrim'"
expect test/genome/beetle.lua 's/initAnim = "crawl"/initAnim = "crwal"/' "initAnim 'crwal'"
expect test/genome/scene/house.lua 's/song = "house"/song = "huose"/' "no song 'huose'"
# a name in two places is ambiguous
mkdir -p "$tmp/test/genome/music/more" && cp "$tmp/test/genome/music/house.mid" "$tmp/test/genome/music/more/house.mid" && list
if gen; then fail "--strict missed an ambiguous song name"; fi
grep -q "ambiguous" "$tmp/err.txt" || { cat "$tmp/err.txt"; fail "no 'ambiguous' error"; }
echo "ids: OK"
