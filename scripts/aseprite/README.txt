Put your scripts here and restart Aseprite to see them in File > Scripts

Scripts are .lua files, you can find more information here:

  https://github.com/aseprite/api


==========================================
EXPORTING SPRITES FOR BICYCLE (sprite.lua)
==========================================

Run File > Scripts > sprite with your sprite open, or in batch mode from the repo root:

  aseprite -b path/to/thing.aseprite --script scripts/aseprite/sprite.lua

Output goes to config.lua's genomePath (default: test/genome/; override with BICYCLE_GENOME_PATH).
Files are named <sprite>_<layer>_*.lua. Keep characters and backgrounds in separate .aseprite files.

CHARACTERS (image layers)
  * Every image layer is part of the character; layers are stacked bottom to top. Transparent = see-through.
  * Tag frame ranges for animations (e.g. walk_up, walk_down, walk_left, walk_right). Untagged = "DEFAULT".
  * Frame durations and tag direction (forward/reverse/ping-pong) and repeats are honored.
  * Use it in a genome:  local hero = { sprite = "thing", initAnim = "walk_down" }

BACKGROUNDS (tilemap layers)
  1. New sprite; set the grid (View > Grid Settings) to your tile size, e.g. 4x2 px (= 8x2 ASCII chars).
  2. Add one tilemap layer (Layer > New > New Tilemap Layer) per background layer. Bottom layer = farthest back.
  3. Draw tiles and lay them out on frame 1. Anything left empty is transparent.
  4. Pick each layer's mode in Layer Properties (double-click the layer) > user data, as key=value pairs:

       mode=fixed                  part of the world; the camera scrolls over it (DEFAULT if user data is empty).
                                   The camera can roam as far as the largest fixed layer.
       mode=glued                  stays put on screen (moves with the camera): skies, frames.
       mode=parallax parallax=50   scrolls at 50% of the camera's speed (default 50 for this mode).
       mode=loop parallax=150      like parallax, but repeats endlessly past its edges (default 100).
       mode=autoloop vx=4 vy=0     drifts by itself in cells/second and repeats endlessly (default vx=4).

     Optional for any mode: parallaxy=<percent> (vertical rate; defaults to parallax), x=<cells> y=<cells> offset.
  5. Export, then use it as a scene's background:  intro.bg = "thing"
     Entities in scene.fg[i] are drawn right after background layer i (e.g. put the hero on the fixed layer).

  Example authoring script: test/aseprite/make_meadow.lua (one layer per mode).
