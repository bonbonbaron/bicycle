#pragma once
#include <fluidsynth.h>

class MidiPlayer {
  public:
    // Not thread-safe without mutex, but in my case, why would it need one?
    static auto getInstance() -> MidiPlayer& {
      static MidiPlayer midiPlayer;
      return midiPlayer;
    }
    void playSound( const int chan, const int key, const int vel ) const;
    void stopSound( const int chan );
    void stopSoundFX();
    void playSoundFX( const int key, const int vel );
    void playSong() const;
    void stopSong();
    void pauseSong();
    void unpauseSong();
    void loadMidi( const std::string& midiFilepath );
    void loadMidi( const std::string&& midiFilepath );
    void loadSoundfont( const std::string& sfFilepath );
    void loadSoundfont( const std::string&& sfFilepath );
  private:
    MidiPlayer();
    MidiPlayer( const MidiPlayer& ) = delete;
    const auto& operator=( const MidiPlayer& ) = delete;
    int _tick{};
    fluid_settings_t* _settingsP{};
    fluid_synth_t* _synthP{};
    fluid_audio_driver_t* _aDriverP{};
    fluid_player_t* _playerP{};
    static constexpr unsigned NUM_SUPPORTED_CHANNELS{ 5 };
};
