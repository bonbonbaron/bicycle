#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "c/MidiPlayer.h"

// BICYCLE_PROFILE=<file> appends how long each audio start-up step took (ms), to find what slows start-up.
static void profile( const char* what ) {
  static FILE* f = std::getenv( "BICYCLE_PROFILE" ) ? std::fopen( std::getenv( "BICYCLE_PROFILE" ), "a" ) : nullptr;
  if ( !f ) {
    return;
  }
  using Clock = std::chrono::steady_clock;
  static Clock::time_point last = Clock::now();
  const auto now = Clock::now();
  std::fprintf( f, "audio %-40s %8.1f ms\n", what, std::chrono::duration<double, std::milli>( now - last ).count() );
  std::fflush( f );
  last = now;
}

static auto fileExists( const std::string& fp ) -> bool {
  return std::ifstream( fp ).good();
}

MidiPlayer::MidiPlayer() {
  profile( "start" );
  // Settings
  _settingsP = new_fluid_settings();
  // fluidsynth supports the following audio drivers: alsa, file, jack, oss, pulseaudio, sdl2 
  if ( const char* file = std::getenv( "BICYCLE_AUDIO_FILE" ) ) {
    fluid_settings_setstr( _settingsP, "audio.driver", "file" );
    fluid_settings_setstr( _settingsP, "audio.file.name", file );
  }
  else if ( const char* driver = std::getenv( "BICYCLE_AUDIO_DRIVER" ) ) {
    fluid_settings_setstr( _settingsP, "audio.driver", driver );
  }
  fluid_settings_setint( _settingsP, "player.reset-synth", 0 );
  fluid_settings_setint( _settingsP, "synth.verbose", 0 );
  // Read a SoundFont's samples from disk only for the presets channels actually select, instead of all of them
  // up front. General MIDI SoundFonts are big (FluidR3_GM is 148 MB), and the game uses a handful of programs.
  fluid_settings_setint( _settingsP, "synth.dynamic-sample-loading", 1 );
  fluid_set_log_function( FLUID_WARN, nullptr, nullptr );  // keep fluidsynth from scribbling on the ncurses screen
  fluid_set_log_function( FLUID_INFO, nullptr, nullptr );

  // Synthesizer
  _synthP = new_fluid_synth( _settingsP );
  fluid_synth_set_gain( _synthP, 1.0 );
  profile( "settings + synth" );

  // Audio driver output. Without one (e.g. no sound card) the game still runs, silently.
  _aDriverP = new_fluid_audio_driver( _settingsP, _synthP );
  _playerP = new_fluid_player( _synthP );
  profile( _aDriverP ? "audio driver + player" : "audio driver (none) + player" );

  // General MIDI fallback goes on the bottom of the SoundFont stack.
  const char* gm = std::getenv( "BICYCLE_GM_SF2" );
  // ./sf2/default-GM.sf2 is where to put one on Windows (or anywhere without a system-wide SoundFont).
  for ( const std::string& fp : { std::string( gm ? gm : "" ), std::string( "./sf2/default-GM.sf2" ),
        std::string( "/usr/share/sounds/sf2/default-GM.sf2" ),
        std::string( "/usr/share/sounds/sf2/TimGM6mb.sf2" ), std::string( "/usr/share/sounds/sf2/FluidR3_GM.sf2" ) } ) {
    if ( !fp.empty() && fileExists( fp ) && fluid_synth_sfload( _synthP, fp.c_str(), 1 ) != FLUID_FAILED ) {
      profile( ( "GM fallback " + fp ).c_str() );
      break;
    }
  }
}

MidiPlayer::~MidiPlayer() {
  if ( _aDriverP ) delete_fluid_audio_driver( _aDriverP );
  if ( _playerP ) delete_fluid_player( _playerP );
  if ( _synthP ) delete_fluid_synth( _synthP );
  if ( _settingsP ) delete_fluid_settings( _settingsP );
}

// Replaces whatever song was queued.
auto MidiPlayer::loadMidi( const std::string& midiFilepath ) -> bool {
  if ( !fileExists( midiFilepath ) ) {
    return false;
  }
  fluid_player_stop( _playerP );
  delete_fluid_player( _playerP );
  _playerP = new_fluid_player( _synthP );
  _tick = 0;
  const bool ok = fluid_player_add( _playerP, midiFilepath.c_str() ) == FLUID_OK;
  profile( ( "MIDI " + midiFilepath ).c_str() );
  return ok;
}

void MidiPlayer::stopSound( const int chan ) {
  fluid_synth_all_notes_off( _synthP, chan );
}

void MidiPlayer::stopSoundFX() {
  fluid_synth_all_notes_off( _synthP, SFX_CHANNEL );
}

// Sound effects are one-shot samples, so the note doesn't need to be released.
void MidiPlayer::playSoundFX( const int key, const int vel ) {
  fluid_synth_noteoff( _synthP, SFX_CHANNEL, key );  // retrigger cleanly
  fluid_synth_noteon( _synthP, SFX_CHANNEL, key, vel );
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
  for ( int chan = 0; chan < SFX_CHANNEL; ++chan ) {
    fluid_synth_all_notes_off( _synthP, chan );
  }
}

void MidiPlayer::pauseSong() {
  _tick = fluid_player_get_current_tick( _playerP );
  fluid_player_stop( _playerP );
}

void MidiPlayer::unpauseSong() {
  fluid_player_seek( _playerP, _tick );
  fluid_player_play( _playerP );
}

// SoundFonts stack: programs are looked up in the most recently loaded one first.
auto MidiPlayer::loadSoundfont( const std::string& sfFilepath ) -> int {
  if ( !fileExists( sfFilepath ) ) {
    return -1;
  }
  auto id = fluid_synth_sfload( _synthP, sfFilepath.c_str(), 1 );
  profile( ( "SoundFont " + sfFilepath ).c_str() );
  return id == FLUID_FAILED ? -1 : id;
}

auto MidiPlayer::selectProgram( const int chan, const int sfontId, const int bank, const int program ) -> bool {
  return fluid_synth_program_select( _synthP, chan, sfontId, bank, program ) == FLUID_OK;
}
