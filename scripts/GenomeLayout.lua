-- Where each kind of genome file lives under <game>/genome/ (README, "Genome layout"). Shared by the engine
-- (Genome.lua, Scene.lua, World3D.lua, Sound.lua) and the Aseprite exporters (scripts/aseprite/), so the
-- two always agree. Plain Lua: no engine or Aseprite API.
--
--   genome/<entity>.lua                    genomes (hero.lua, coin.lua, door.lua, ...): everything about one kind of
--                                          thing (sprite or model, animation, collision, sounds, motion, behavior);
--                                          scenes only say which genomes go where (README, "Genomes")
--   genome/body/<sprite>.lua               sprite manifests (which layers, which animation)
--   genome/body/model/                     3D bodies of genomes: <model>.obj/.mtl (packed .bmesh), .bskin, textures
--   genome/body/palette/color/             *_palette.lua      color palettes of the gray images
--   genome/body/palette/collision/         *_collset.lua, *_collSet.lua   collision palettes (tile -> collision type)
--   genome/body/graybody/                  *_pixels.bimg      gray (indexed) pixel images, sprites and tilesets
--   genome/body/graybody/animation/        *_anim_src_rects.lua, *_globalAnim.lua
--   genome/body/graybody/collision/        *_collmap.lua, *_collRects.lua  collision shapes / masks
--   genome/body/graybody/colormap/         *_colormap.lua/.png ASCII colormaps; text images (img gene, *.txt)
--   genome/tilemap/                        *_tilemap.lua, *_tileset.lua  background tile layouts and tilesets
--   genome/scene/                          scene tables (scene/bg/<name>/*.txt: text backgrounds)
--   genome/level/                          3D levels from Blender (tools/blender): the level meshes, the entities placed
--                                          in it (genome + position + per-instance parameters) and its spawn points
--   genome/model/                          level meshes (.obj/.mtl, packed .bmesh) and their textures (.bimg)
--   genome/music/                          songs (.mid) and SoundFonts (.sf2), shared by scenes (music) and genomes (sfx)
--
-- Asset references are bare names, never folders or extensions: soundfont = "meadow", song = "house",
-- model = "coin3d", level = "courtyard", sprite = "pilgrim". Layout.path( kind, name ) turns one into its file;
-- the build-time ID generator (tools/ids/genids.lua) checks every reference and records the files it found
-- (gen/ids.lua: ids.paths[game][kind][name]) so a name in a subfolder (music/sfx/coin.sf2 -> "coin") resolves too.
-- Older genome-relative paths ("music/meadow.sf2") still work.
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
Layout.BODYMODEL = "body/model/"

-- Every directory, for creating a new game's tree (scripts/new_game.sh does the same with mkdir -p).
Layout.DIRS = { Layout.COLOR, Layout.COLLPAL, Layout.ANIM, Layout.COLLSHAPE, Layout.COLORMAP, Layout.TILEMAP,
                Layout.SCENE, Layout.LEVEL, Layout.MODEL, Layout.MUSIC, Layout.BODYMODEL }

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
  -- Bare-name references: kind -> { dir, extensions (first = the one the engine asks for), what }. The ID generator
-- scans each dir (recursively) for these extensions; a name found twice is an error ("ambiguous").
Layout.REFS = {
  soundfont = { dir = Layout.MUSIC, exts = { ".sf2" }, what = "SoundFont" },
  song      = { dir = Layout.MUSIC, exts = { ".mid", ".midi" }, what = "song" },
  model     = { dir = Layout.BODYMODEL, exts = { ".bmesh", ".obj" }, what = "genome model" },  -- .obj -> .bmesh when packed
  level     = { dir = Layout.LEVEL, exts = { ".lua" }, what = "level" },
  scene     = { dir = Layout.SCENE, exts = { ".lua" }, what = "scene" },
  sprite    = { dir = Layout.BODY, exts = { ".lua" }, what = "sprite", flat = true },
  genome    = { dir = "", exts = { ".lua" }, what = "genome", flat = true },
}

-- True if ref is an old-style genome-relative path rather than a bare name ("music/meadow.sf2").
function Layout.isPath( ref )
  return type( ref ) == "string" and ( ref:find( "/", 1, true ) ~= nil or ref:find( "%.%a+$" ) ~= nil )
end

-- The bare name of a file for a kind: "music/sfx/coin.sf2" -> "coin" (nil if it isn't that kind's file).
function Layout.nameOf( kind, rel )
  local r = Layout.REFS[kind]
  if not r or rel:sub( 1, #r.dir ) ~= r.dir then return nil end
  local rest = rel:sub( #r.dir + 1 )
  if r.flat and rest:find( "/" ) then return nil end
  for _, e in ipairs( r.exts ) do
    if rest:sub( -#e ):lower() == e then return ( rest:sub( 1, -#e - 1 ):match( "([^/]+)$" ) ) end
  end
  return nil
end

-- The genome-relative file of a bare name: path( "soundfont", "meadow" ) == "music/meadow.sf2". found is the
-- generator's table for this game and kind (gen/ids.lua ids.paths[game][kind]), if there is one: files in
-- subfolders or with another extension (.midi) come from there. Old-style paths are returned unchanged.
function Layout.path( kind, name, found )
  if Layout.isPath( name ) then return name end
  if found and found[name] then return found[name] end
  local r = Layout.REFS[kind] or error( "GenomeLayout: no reference kind '" .. tostring( kind ) .. "'" )
  return r.dir .. name .. r.exts[1]
end

return Layout.BODY
end

-- Genome-relative directories of the older gene asset folders (Gene.getAssetDir).
local GENE_DIRS = { img = Layout.COLORMAP, models = Layout.BODYMODEL, model = Layout.BODYMODEL, music = Layout.MUSIC,
                    scene = Layout.SCENE, level = Layout.LEVEL }
function Layout.geneDir( geneType )
  return GENE_DIRS[geneType] or ( geneType.."/" )
end

-- Bare-name references: kind -> { dir, extensions (first = the one the engine asks for), what }. The ID generator
-- scans each dir (recursively) for these extensions; a name found twice is an error ("ambiguous").
Layout.REFS = {
  soundfont = { dir = Layout.MUSIC, exts = { ".sf2" }, what = "SoundFont" },
  song      = { dir = Layout.MUSIC, exts = { ".mid", ".midi" }, what = "song" },
  model     = { dir = Layout.BODYMODEL, exts = { ".bmesh", ".obj" }, what = "genome model" },  -- .obj -> .bmesh when packed
  level     = { dir = Layout.LEVEL, exts = { ".lua" }, what = "level" },
  scene     = { dir = Layout.SCENE, exts = { ".lua" }, what = "scene" },
  sprite    = { dir = Layout.BODY, exts = { ".lua" }, what = "sprite", flat = true },
  genome    = { dir = "", exts = { ".lua" }, what = "genome", flat = true },
}

-- True if ref is an old-style genome-relative path rather than a bare name ("music/meadow.sf2").
function Layout.isPath( ref )
  return type( ref ) == "string" and ( ref:find( "/", 1, true ) ~= nil or ref:find( "%.%a+$" ) ~= nil )
end

-- The bare name of a file for a kind: "music/sfx/coin.sf2" -> "coin" (nil if it isn't that kind's file).
function Layout.nameOf( kind, rel )
  local r = Layout.REFS[kind]
  if not r or rel:sub( 1, #r.dir ) ~= r.dir then return nil end
  local rest = rel:sub( #r.dir + 1 )
  if r.flat and rest:find( "/" ) then return nil end
  for _, e in ipairs( r.exts ) do
    if rest:sub( -#e ):lower() == e then return ( rest:sub( 1, -#e - 1 ):match( "([^/]+)$" ) ) end
  end
  return nil
end

-- The genome-relative file of a bare name: path( "soundfont", "meadow" ) == "music/meadow.sf2". found is the
-- generator's table for this game and kind (gen/ids.lua ids.paths[game][kind]), if there is one: files in
-- subfolders or with another extension (.midi) come from there. Old-style paths are returned unchanged.
function Layout.path( kind, name, found )
  if Layout.isPath( name ) then return name end
  if found and found[name] then return found[name] end
  local r = Layout.REFS[kind] or error( "GenomeLayout: no reference kind '" .. tostring( kind ) .. "'" )
  return r.dir .. name .. r.exts[1]
end

return Layout
