require("bicycle")

-- Music and sound effects for a scene. Configure in the scene table (paths relative to <game>/genome/):
--   scene.music = { soundfont = "music/meadow.sf2", song = "music/meadow.mid" }
--   scene.sfx   = { soundfont = "music/meadow.sf2", program = 120, bank = 0, sounds = { coin = 72, bump = 60 } }
-- A missing SoundFont falls back to the General MIDI one the engine always loads; a missing song is skipped.
-- See tools/audio/README.md for making SoundFonts and MIDI files from FL Studio, LMMS or midish.
local Sound = { sounds = {} }

local function genomePath( rel )
  return "./"..gameName.."/genome/"..rel
end

local function loadSoundfont( rel )
  if not rel then return -1 end
  local id = ffi.C.loadSoundfont( genomePath( rel ) )
  if id < 0 then io.stderr:write( "bicycle: couldn't load SoundFont "..rel.."; using General MIDI\n" ) end
  return id
end

function Sound.load( music, sfx )
  local loaded = {}  -- don't stack the same SoundFont twice
  if music then
    if music.soundfont then loaded[music.soundfont] = loadSoundfont( music.soundfont ) end
    if music.song and not os.getenv( "BICYCLE_MUTE_MUSIC" ) then  -- set BICYCLE_MUTE_MUSIC=1 to hear only SFX
      if ffi.C.loadMidi( genomePath( music.song ) ) then
        ffi.C.playSong()  -- loops forever
      else
        io.stderr:write( "bicycle: couldn't load song "..music.song.."\n" )
      end
    end
  end
  if sfx then
    local id = loaded[sfx.soundfont] or loadSoundfont( sfx.soundfont )
    if id >= 0 then
      ffi.C.selectProgram( ffi.C.getSfxChannel(), id, sfx.bank or 0, sfx.program or 0 )
    end
    for name, key in pairs( sfx.sounds or {} ) do
      Sound.sounds[name] = key
    end
  end
end

-- Plays a named sound effect (or a raw MIDI key) on the SFX channel.
function Sound.play( name, vel )
  local key = Sound.sounds[name] or ( type( name ) == "number" and name )
  if not key then error( "Unknown sound effect '"..tostring( name ).."'." ) end
  ffi.C.playSoundFX( key, vel or 110 )
end

function Sound.stopMusic()
  ffi.C.stopSong()
end

return Sound
