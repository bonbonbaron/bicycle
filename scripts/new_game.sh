#!/bin/sh
# Creates a new game's genome tree (README, "Genome layout"; scripts/GenomeLayout.lua):
#   scripts/new_game.sh mygame
set -e
[ -n "$1" ] || { echo "usage: $0 <game>"; exit 1; }
G="$1"
mkdir -p "$G"/genome/body/palette/color "$G"/genome/body/palette/collision \
         "$G"/genome/body/graybody/animation "$G"/genome/body/graybody/collision "$G"/genome/body/graybody/colormap \
         "$G"/genome/tilemap "$G"/genome/scene "$G"/genome/level "$G"/genome/model "$G"/genome/body/model "$G"/genome/music "$G"/aseprite "$G"/blender
[ -e "$G/main.lua" ] || cat > "$G/main.lua" <<LUA
require('bicycle')
local Scene = require('Scene')
-- Edit $G/genome/scene/start.lua; add art with scripts/aseprite/sprite.lua (BICYCLE_GENOME_PATH=$G/genome/).
Scene.start( "start" )
LUA
[ -e "$G/genome/scene/start.lua" ] || cat > "$G/genome/scene/start.lua" <<LUA
local start = {}
start.name = "Start"
start.spawns = { default = { 10, 10 } }
start.fg = { [1] = {} }
return start
LUA
echo "created $G/ (build it with -DBICYCLE_GAMES=$G)"
