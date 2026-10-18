# Music and sound effects for bicycle

bicycle plays music and sound effects with fluidsynth. A song is a **MIDI file** (`.mid`). The sounds come from a
**SoundFont** (`.sf2`), which maps MIDI programs to instruments. A General MIDI SoundFont is always loaded
underneath yours. Any program your SoundFont doesn't define falls back to it (`BICYCLE_GM_SF2` overrides which
file; Debian: `apt install fluid-soundfont-gm`).

## Wire it into a scene and its genomes

Put the files under `<game>/genome/music/` and name them by their bare file names (no folder, no extension: the
field says it's a SoundFont or a song). Music belongs to the scene (`test/genome/scene/intro.lua`):

```lua
intro.music = { soundfont = "meadow", song = "meadow" }   -- music/meadow.sf2 + music/meadow.mid, loops forever
```

Sound effects belong to the genome that makes them (`test/genome/coin.lua`), with names of its own:

```lua
local coin = { sprite = "goldcoin", coll = { type = "coin" },
               sfx = { soundfont = "meadow", program = 120, sounds = { collect = 72 } } }   -- name = MIDI key
function coin:onCollision( other ) self:sound( "collect" ) end
```

A sound can also be `{ key = 64, program = 121, soundfont = "other", vel = 90 }`. A SoundFont several genomes use is
loaded once and shared. Scenes don't list sound effects any more (an old `scene.sfx` still works, with a warning).
Sound effects play on MIDI channel 15 (channel 16 in 1-based
tools). Songs must not use it; `midi_prep.py` moves it if they do.

Environment: `BICYCLE_AUDIO_DRIVER=pulseaudio|alsa|jack|sdl2` picks the output. `BICYCLE_AUDIO_FILE=out.wav`
records everything to a WAV instead (good for testing on machines without speakers). `BICYCLE_MUTE_MUSIC=1`
leaves only the sound effects.

## 1. The song (MIDI)

| Tool | Export |
|---|---|
| FL Studio | File > Export > MIDI file. Each channel becomes a track named after it. |
| LMMS | File > Export MIDI. |
| midish | `export "song.mid"` (or `import`/`export` in a script; `midish -b < script`). |

Then check it against your SoundFont and keep it off the SFX channel:

```sh
tools/audio/midi_prep.py song.mid <game>/genome/music/song.mid --sf2 <game>/genome/music/game.sf2 \
    [--map-json kit/programs.json] [--map "Lead=0" --map "Bass=33"]
```

`--map` sets each track's program by track name. FL Studio and LMMS name tracks after channels, so the
programs match the kit `flp_to_kit.py` builds.

## 2. The instruments (SoundFont)

`make_sf2.py` builds a `.sf2` from a **kit**: one folder of WAVs per instrument.

```
kit/
  000_lead/   C3.wav C4.wav C5.wav   multi-sampled: each key plays its nearest sample, repitched
  033_bass/   C2.wav + instrument.json {"loop": true, "loop_start": 0.2, "loop_end": 0.8, "release": 0.15}
  120_sfx/    60.wav 64.wav 72.wav + instrument.json {"kit": true}   one unpitched sound per key
```

The folder number is the MIDI program. File names are root keys (`60.wav` or `C4.wav`; C4 = 60). WAVs can
be any rate, mono or stereo, and 8/16/24/32-bit. Samples are normalized per instrument unless
`"normalize": false`. Build it:

```sh
tools/audio/make_sf2.py kit <game>/genome/music/game.sf2 --name game
```

### From FL Studio

FL Studio runs on Windows and macOS only, and its synths (3x Osc, Sytrus, VSTs) only make sound inside it.
So the samples have to come out of FL. The rest is scripted:

1. Copy the `.flp` (and its samples, if you have them) to this machine and run
   `tools/audio/flp_to_kit.sh song.flp kit --search path/to/samples`. This uses PyFLP via uv with Python 3.10;
   run `pip install uv` once.
   * **Sampler channels**: their WAVs are copied in with FL's root note. Automation and layer channels are
     skipped.
   * **Plugin channels**: you get `kit/NNN_name/RENDER_ME.txt` and a `note_sweep.mid`. In FL, drag the sweep
     onto that channel, solo it, and File > Export > Wave (16-bit). Then run
     `tools/audio/slice_sweep.py stem.wav kit/NNN_name`.
   * `kit/programs.json` maps channel names to programs for `midi_prep.py --map-json`.
2. Run `make_sf2.py kit game.sf2`, export the MIDI (above), then run `midi_prep.py --map-json kit/programs.json`.

What can't be automated: rendering FL Studio's or VST synths' audio (that needs FL itself); FL effects, mixer
and automation (a SoundFont holds raw samples plus a simple envelope); non-WAV samples (convert them first,
e.g. `sox in.mp3 out.wav`).

### From LMMS

Same sweep trick. Put `note_sweep.mid` on an instrument track, then File > Export (or headless:
`lmms render project.mmp -o stem.wav`), and run `slice_sweep.py`. For AudioFileProcessor tracks, just copy the
sample into a kit folder.

### From any sampler or a folder of WAVs

Drop the WAVs into `kit/NNN_name/` and run `make_sf2.py`.

## Check it

```sh
tools/audio/test_pipeline.sh       # synth samples -> midish -> midi_prep -> sweep/slice -> make_sf2 -> render
fluidsynth -ni -F out.wav game.sf2 song.mid   # listen to (or measure) a render without the game
```
