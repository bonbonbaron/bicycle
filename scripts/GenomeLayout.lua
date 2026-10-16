-- Where each kind of genome file lives under <game>/genome/ (README, "Genome layout"). Shared by the engine
-- (Genome.lua, Scene.lua, World3D.lua, Sound.lua) and the Aseprite exporters (scripts/aseprite/), so the
-- two always agree. Plain Lua: no engine or Aseprite API.
--
--   genome/<entity>.lua                    genomes (hero.lua, coin.lua, door.lua, ...)
--   genome/body/<sprite>.lua               sprite manifests (which layers, which animation)
--   genome/body/palette/color/             *_palette.lua      color palettes of the gray images
--   genome/body/palette/collision/         *_collset.lua, *_collSet.lua   collision palettes (tile -> collision type)
--   genome/body/graybody/                  *_pixels.bimg      gray (indexed) pixel images, sprites and tilesets
--   genome/body/graybody/animation/        *_anim_src_rects.lua, *_globalAnim.lua
--   genome/body/graybody/collision/        *_collmap.lua, *_collRects.lua  collision shapes / masks
--   genome/body/graybody/colormap/         *_colormap.lua/.png ASCII colormaps; text images (img gene, *.txt)
--   genome/tilemap/                        *_tilemap.lua, *_tileset.lua  background tile layouts and tilesets
--   genome/scene/                          scene tables (scene/bg/<name>/*.txt: text backgrounds)
--   genome/level/                          3D level / scenery lists (scene3d.lua from scripts/blender/export.py)
--   genome/model/                          meshes (.obj/.mtl, packed .bmesh), skins (.bskin), mesh textures (.bimg)
--   genome/music/                          songs (.mid) and SoundFonts (.sf2)
local Layout = {}

Layout.BODY = "body/"
Layout.COLOR = "body/palette/color/"
Layout.COLLPAL = "body/palette/collision/"
Layout.GRAY = "body/graybody/"
Layout.ANIM = "body/graybody/animation/"
Layout.COLLSHAPE = "body/graybody/collision/"
Layout.COLORMAP = "body/graybody/colormap/"
Layout.TILEMAP = "tilemap/"
Layout.SCENE = "scene/"
Layout.LEVEL = "level/"
Layout.MODEL = "model/"
Layout.MUSIC = "music/"

-- Every directory, for creating a new game's tree (scripts/new_game.sh does the same with mkdir -p).
Layout.DIRS = { Layout.COLOR, Layout.COLLPAL, Layout.ANIM, Layout.COLLSHAPE, Layout.COLORMAP, Layout.TILEMAP,
                Layout.SCENE, Layout.LEVEL, Layout.MODEL, Layout.MUSIC }

-- Suffix rules for the exporters' generated names, first match wins.
local RULES = {
  { "_pixels$", Layout.GRAY },
  { "_colormap$", Layout.COLORMAP },
  { "_anim_src_rects$", Layout.ANIM },
  { "_globalAnim$", Layout.ANIM },
  { "_collmap$", Layout.COLLSHAPE },
  { "_collRects$", Layout.COLLSHAPE },
  { "_collset$", Layout.COLLPAL },
  { "_collSet$", Layout.COLLPAL },
  { "_palette$", Layout.COLOR },
  { "_tilemap$", Layout.TILEMAP },
  { "_tileset$", Layout.TILEMAP },
}

-- The genome-relative directory (with a trailing slash) of a sprite asset by its bare name, e.g.
-- dirFor( "pilgrim_body_palette" ) == "body/palette/color/"; anything else (a sprite manifest) is in body/.
function Layout.dirFor( name )
  for _, r in ipairs( RULES ) do
    if name:find( r[1] ) then return r[2] end
  end
  return Layout.BODY
end

-- Genome-relative directories of the older gene asset folders (Gene.getAssetDir).
local GENE_DIRS = { img = Layout.COLORMAP, models = Layout.MODEL, model = Layout.MODEL, music = Layout.MUSIC,
                    scene = Layout.SCENE, level = Layout.LEVEL }
function Layout.geneDir( geneType )
  return GENE_DIRS[geneType] or ( geneType.."/" )
end

return Layout
