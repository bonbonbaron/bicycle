#pragma once
#include <string>
#include <utility>
#include <vector>
#include <fluidsynth.h>

// Music (a looping MIDI file) and sound effects (notes on a reserved channel), both synthesized by fluidsynth
// from SoundFonts. A General MIDI SoundFont is the fallback for any program the game's own SoundFonts don't define;
// it's loaded only when one is needed (a song or the SFX channel uses a bank/program none of the game's SoundFonts
// has), and then goes under them in the stack. Most games never touch it, so start-up skips reading ~30 MB.
// BICYCLE_EAGER_GM=1 loads it at start-up as before.
//
// Environment:
//   BICYCLE_AUDIO_DRIVER  fluidsynth audio driver (pulseaudio, alsa, jack, sdl2, ...); default: fluidsynth's choice
//   BICYCLE_AUDIO_FILE    render all audio to this WAV file instead of a sound card (driver "file")
//   BICYCLE_GM_SF2        fallback General MIDI SoundFont; default: first of ./sf2/default-GM.sf2, /usr/share/sounds/sf2/{default-GM,TimGM6mb,FluidR3_GM}.sf2
//   BICYCLE_PROFILE       append audio start-up timings (ms per step) to this file
// Samples load lazily (synth.dynamic-sample-loading): only the presets channels select are read from disk.
class MidiPlayer {
  public:
    // Not thread-safe without mutex, but in my case, why would it need one?
    static auto getInstance() -> MidiPlayer& {
      static MidiPlayer midiPlayer;
      return midiPlayer;
    }
    static constexpr int SFX_CHANNEL{ 15 };  // keep songs off this channel
    void playSound( const int chan, const int key, const int vel ) const;
    void stopSound( const int chan );
    void stopSoundFX();
    void playSoundFX( const int key, const int vel );
    // Positional SFX: pan -1 (left) .. 1 (right) as MIDI CC10 on the SFX channel, gain 0..1 scales the velocity.
    // One channel: overlapping effects share the latest pan.
    void playSoundFXPanned( const int key, const int vel, const float pan, const float gain );
    void playSong() const;
    void stopSong();
    void pauseSong();
    void unpauseSong();
    auto loadMidi( const std::string& midiFilepath ) -> bool;
    auto loadSoundfont( const std::string& sfFilepath ) -> int;  // returns SoundFont ID, or -1
    auto selectProgram( const int chan, const int sfontId, const int bank, const int program ) -> bool;
    // Volume ramps (scene music crossfades): fadeTo( level, ms ) moves the synth's gain to level (0..1 of normal)
    // over ms; tickFade() advances it once a frame (the main loop calls it).
    void fadeTo( float level, unsigned ms );
    void tickFade();
    auto fadeLevel() const -> float { return _level; }
    ~MidiPlayer();
  private:
    MidiPlayer();
    MidiPlayer( const MidiPlayer& ) = delete;
    const auto& operator=( const MidiPlayer& ) = delete;
    int _tick{};
    float _baseGain{ -1.0f }, _level{ 1.0f }, _target{ 1.0f }, _step{};
    fluid_settings_t* _settingsP{};
    fluid_synth_t* _synthP{};
    fluid_audio_driver_t* _aDriverP{};
    fluid_player_t* _playerP{};
    std::string _rawFile{};  // BICYCLE_AUDIO_FILE when fluidsynth can only write raw samples
    // Lazy General MIDI fallback (see above).
    struct GameFont { std::string name; int id; };  // a game SoundFont, by its in-memory name and current id
    std::vector<GameFont> _gameFonts{};             // load order; ids change when GM is slid under them
    std::vector<std::pair<int, int>> _idMap{};      // first id handed to Lua -> current id
    struct ChanSel { int sfont{ -1 }, bank{}, program{}; };
    ChanSel _sel[16]{};                             // explicit selectProgram per channel (re-applied after a restack)
    bool _gmTried{};
    bool _sfxSelected{};
    auto hasPreset( int bank, int program ) const -> bool;  // in one of the game's SoundFonts
    void ensureGm( const char* why );                       // load GM under the game's SoundFonts, once
    auto currentId( int luaId ) const -> int;
    std::string _unused{};  // BICYCLE_AUDIO_FILE when fluidsynth can only write raw samples
};
