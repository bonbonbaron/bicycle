# UI fonts (2D and 3D modes)

`ui.ttf` and `ui-bold.ttf` are DejaVu Sans and DejaVu Sans Bold (https://dejavu-fonts.github.io/), under the
Bitstream Vera / DejaVu license (free to use, embed and redistribute; see `LICENSE-DejaVu.txt`). The pixel modes
rasterize them with stb_truetype (`include/third_party/stb_truetype.h`, public domain) for dialogue, menus and HUD
labels. Replace them with any TTF of the same names to restyle a game; ASCII mode doesn't use them.
