#include <iostream>
#include "MidiPlayer.h"

MidiPlayer::MidiPlayer() {

  // Settings
  _settingsP = new_fluid_settings();

  // fluidsynth supports the following audio drivers: alsa, file, jack, oss, pulseaudio, sdl2 
  fluid_settings_setstr(_settingsP, "audio.driver", "alsa");
  fluid_settings_setstr(_settingsP, "audio.alsa.device", "hw:2");
  fluid_settings_setint(_settingsP, "player.reset-synth", 0);

  // Synthesizer
  _synthP = new_fluid_synth( _settingsP );

  // Audio driver output
  _aDriverP = new_fluid_audio_driver ( _settingsP, _synthP );
  _playerP = new_fluid_player( _synthP );

  fluid_synth_sfload( _synthP, "/usr/share/sounds/sf2/TimGM6mb.sf2", 0 );

  const unsigned 
    sfontId = 1,
            bank = 0,
            preset = 1;

  for (int channel = 0; channel < NUM_SUPPORTED_CHANNELS; ++channel ) {
    fluid_synth_program_select( _synthP, channel, sfontId, bank, preset);
  }

  fluid_synth_set_gain( _synthP, 3.0 );
}


void MidiPlayer::loadMidi( const std::string& midiFilepath ) {
  fluid_player_add( _playerP, midiFilepath.c_str() );
}

void MidiPlayer::loadMidi( const std::string&& midiFilepath ) {
  fluid_player_add( _playerP, midiFilepath.c_str() );
}

void MidiPlayer::stopSound( const int chan ) {
  fluid_synth_all_notes_off( _synthP, chan );
}

void MidiPlayer::stopSoundFX() {
  fluid_synth_all_notes_off( _synthP, NUM_SUPPORTED_CHANNELS - 1 );
}

void MidiPlayer::playSoundFX( const int key, const int vel ) {
  fluid_synth_noteon( _synthP, NUM_SUPPORTED_CHANNELS - 1, key, vel );
}

void MidiPlayer::playSound( const int chan, const int key, const int vel ) const {
  fluid_synth_noteon( _synthP, chan, key, vel);
}

void MidiPlayer::playSong() const {
  fluid_player_set_loop( _playerP, -1 );
  fluid_player_play( _playerP );
}

void MidiPlayer::stopSong() {
  _tick = 0;
  fluid_player_stop( _playerP );
}

void MidiPlayer::pauseSong() {
  _tick = fluid_player_get_current_tick( _playerP );
  fluid_player_stop( _playerP );
}

void MidiPlayer::unpauseSong() {
  fluid_player_seek( _playerP, _tick );
  fluid_player_play( _playerP );
}
