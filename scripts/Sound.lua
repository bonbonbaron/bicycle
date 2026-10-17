require("bicycle")

-- Music belongs to scenes; sound effects belong to genomes.
--
--   scene.music = { soundfont = "meadow", song = "meadow" }        -- bare names: genome/music/meadow.sf2 / .mid
--   genome.sfx  = { soundfont = "sfx", program = 120, bank = 0,    -- the genome's own sounds, by names local to it
--                   sounds = { collect = 72,                       -- a MIDI key of that program
--                              boing = { key = 60, program = 12, soundfont = "kit", vel = 90 } } }  -- per sound
--   function coin:onCollision( other, t ) self:sound( "collect" ) end  -- plays the coin's own "collect"
--
-- A coin's "collect" and a keepsake's "collect" are different sounds, and nothing is renamed per scene: each
-- genome's sounds resolve from its own sfx gene, wherever it's dropped. SoundFonts are loaded once per run and
-- shared (cached by name: every genome using "sfx" uses one copy). A missing SoundFont falls back to General MIDI.
-- One-shot samples: make a SoundFont of them (tools/audio/make_sf2.py) and name their keys here.
-- See tools/audio/README.md for making SoundFonts and MIDI files from FL Studio, LMMS or midish.
local Ids = require( "Ids" )
local Sound = { sounds = {} }

-- SoundFonts load once per run (they outlive scenes: loaded outside the scene's memory, see Scene.lua), and a scene
-- whose song is already playing keeps it going.
local soundfonts = {}  -- file -> SoundFont id
Sound.soundfonts = soundfonts
local function loadSoundfont( ref )
  if not ref then return -1 end
  local file = Ids.asset( "soundfont", ref )
  if soundfonts[file] then return soundfonts[file] end
  local scoped = memSceneScoped and memSceneScoped()
  if scoped then ffi.C.sceneScope( false ) end
  local id = ffi.C.loadSoundfont( file )
  if scoped then ffi.C.sceneScope( true ) end
  if id < 0 then io.stderr:write( "bicycle: couldn't load SoundFont "..ref.."; using General MIDI\n" ) end
  soundfonts[file] = id
  return id
end
Sound.loadSoundfont = loadSoundfont

Sound.song = nil  -- the song playing (its file), or nil

-- The SFX channel's program, switched only when a sound needs another one (notes already sounding keep theirs).
local selected = { sf = false, bank = false, program = false }
local function select( sf, bank, program )
  if sf < 0 or ( selected.sf == sf and selected.bank == bank and selected.program == program ) then return end
  ffi.C.selectProgram( ffi.C.getSfxChannel(), sf, bank, program )
  selected.sf, selected.bank, selected.program = sf, bank, program
end

-- keepSong: leave the current song playing whatever this scene's music says (Scene.go's music = "keep").
-- sfx: an old scene-level sfx table (still played by Sound.play( name ) without an owner; prefer genome sfx genes).
function Sound.load( music, sfx, keepSong )
  if keepSong then
    if music and music.soundfont then loadSoundfont( music.soundfont ) end
  elseif music then
    if music.soundfont then loadSoundfont( music.soundfont ) end
    local song = music.song and Ids.asset( "song", music.song )
    if song ~= Sound.song then
      ffi.C.stopSong()
      Sound.song = nil
    end
    if song and song ~= Sound.song and not os.getenv( "BICYCLE_MUTE_MUSIC" ) then  -- BICYCLE_MUTE_MUSIC=1: SFX only
      if ffi.C.loadMidi( song ) then
        ffi.C.playSong()  -- loops forever
        Sound.song = song
      else
        io.stderr:write( "bicycle: couldn't load song "..music.song.."\n" )
      end
    end
  else
    ffi.C.stopSong()
    Sound.song = nil
  end
  if sfx then
    local spec = Sound.prepare( sfx )
    for id, s in pairs( spec ) do Sound.sounds[id] = s end
  end
end

-- The song file a scene's music plays (for comparing scenes' music), or nil.
function Sound.songOf( music )
  return music and music.song and Ids.asset( "song", music.song ) or nil
end

-- A genome's sfx gene resolved once: sound id -> { sf, bank, program, key, vel }. Cached per sfx table, so every
-- instance of a genome (and every palette swap sharing the table) resolves it once.
local prepared = setmetatable( {}, { __mode = "k" } )
function Sound.prepare( sfx )
  if type( sfx ) ~= "table" then return {} end
  if prepared[sfx] then return prepared[sfx] end
  local sf = loadSoundfont( sfx.soundfont )
  local out = {}
  for name, v in pairs( sfx.sounds or {} ) do
    local s = type( v ) == "table" and v or { key = v }
    out[Ids.define( "sound", name )] = {
      sf = s.soundfont and loadSoundfont( s.soundfont ) or sf, bank = s.bank or sfx.bank or 0,
      program = s.program or sfx.program or 0, key = s.key or s[1] or 60, vel = s.vel or sfx.vel, name = name }
  end
  prepared[sfx] = out
  return out
end

-- The resolved sound named name of owner (a genome instance's table: its sfx gene), else of the old scene-level
-- table; errors list the owner's sounds.
local function lookup( name, owner, level )
  local id = Ids.find( "sound", name )
  local own = owner and owner.sfx and Sound.prepare( owner.sfx )
  local s = id and ( ( own and own[id] ) or ( not owner and Sound.sounds[id] ) )
  if s then return s end
  local names = {}
  for _, v in pairs( own or Sound.sounds ) do names[#names + 1] = v.name end
  table.sort( names )
  error( "no sound effect '"..tostring( name ).."' in "..( owner and ( "genome '"..tostring( owner.genomeName or owner.name or "?" ).."'s sfx.sounds" ) or "the scene's sfx" )..
         " (has: "..( #names > 0 and table.concat( names, ", " ) or "none" )..")", level or 3 )
end

-- Plays one of owner's sounds (owner: a genome instance's table) or a raw MIDI key on the SFX channel; x, y, z: a
-- 3D position (panned and attenuated for the camera).
function Sound.playFor( owner, name, vel, x, y, z )
  if type( name ) == "number" then ffi.C.playSoundFX( name, vel or 110 ) return end
  local s = lookup( name, owner, 3 )
  select( s.sf, s.bank, s.program )
  if x then
    require( "World3D" ).sound( s.key, vel or s.vel or 110, x, y, z )
  else
    ffi.C.playSoundFX( s.key, vel or s.vel or 110 )
  end
end

-- Old API: a sound of the scene-level sfx table (or a raw MIDI key). Genomes use self:sound( name ).
function Sound.play( name, vel, owner )
  Sound.playFor( owner, name, vel )
end

-- The MIDI key of a named sound effect (owner's, else the scene-level table's), or nil.
function Sound.key( name, owner )
  local id = Ids.find( "sound", name )
  if not id then return nil end
  local own = owner and owner.sfx and Sound.prepare( owner.sfx )
  local s = ( own and own[id] ) or Sound.sounds[id]
  return s and s.key
end

function Sound.stopMusic()
  ffi.C.stopSong()
end

return Sound
