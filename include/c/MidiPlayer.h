#pragma once
#include <string>
#include <fluidsynth.h>

// Music (a looping MIDI file) and sound effects (notes on a reserved channel), both synthesized by fluidsynth
// from SoundFonts. A General MIDI SoundFont is loaded first as a fallback for any program the game's own
// SoundFonts don't define.
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
    void playSong() const;
    void stopSong();
    void pauseSong();
    void unpauseSong();
    auto loadMidi( const std::string& midiFilepath ) -> bool;
    auto loadSoundfont( const std::string& sfFilepath ) -> int;  // returns SoundFont ID, or -1
    auto selectProgram( const int chan, const int sfontId, const int bank, const int program ) -> bool;
    ~MidiPlayer();
  private:
    MidiPlayer();
    MidiPlayer( const MidiPlayer& ) = delete;
    const auto& operator=( const MidiPlayer& ) = delete;
    int _tick{};
    fluid_settings_t* _settingsP{};
    fluid_synth_t* _synthP{};
    fluid_audio_driver_t* _aDriverP{};
    fluid_player_t* _playerP{};
    std::string _rawFile{};  // BICYCLE_AUDIO_FILE when fluidsynth can only write raw samples
};
