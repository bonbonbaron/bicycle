#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "c/MidiPlayer.h"
#include "data/Pack.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>

// SoundFonts inside the asset pack are inflated into memory and handed to fluidsynth through its SoundFont
// loader's file callbacks: names starting with MEM_PREFIX are memory buffers, anything else is a real file.
// fluidsynth re-opens SoundFonts for dynamic sample loading, so the buffers stay alive for the whole game.
namespace {
  constexpr const char* MEM_PREFIX = "bicycle-mem://";
  std::mutex memMtx;
  std::map<std::string, std::shared_ptr<const std::string>> memFiles;

  struct SfHandle {
    FILE* fp{};
    std::shared_ptr<const std::string> mem;
    long long pos{};
  };

  void* sfOpen( const char* filename ) {
    const std::string name( filename );
    if ( name.rfind( MEM_PREFIX, 0 ) == 0 ) {
      std::unique_lock<std::mutex> l( memMtx );
      auto it = memFiles.find( name );
      return it == memFiles.end() ? nullptr : new SfHandle{ nullptr, it->second, 0 };
    }
    FILE* fp = std::fopen( filename, "rb" );
    return fp ? new SfHandle{ fp, nullptr, 0 } : nullptr;
  }

  int sfRead( void* buf, fluid_long_long_t count, void* handle ) {
    auto* h = static_cast<SfHandle*>( handle );
    if ( h->fp ) {
      return std::fread( buf, static_cast<size_t>( count ), 1, h->fp ) == 1 ? FLUID_OK : FLUID_FAILED;
    }
    if ( count < 0 || h->pos + count > static_cast<long long>( h->mem->size() ) ) return FLUID_FAILED;
    std::memcpy( buf, h->mem->data() + h->pos, static_cast<size_t>( count ) );
    h->pos += count;
    return FLUID_OK;
  }

  int sfSeek( void* handle, fluid_long_long_t offset, int origin ) {
    auto* h = static_cast<SfHandle*>( handle );
    if ( h->fp ) {
#ifdef _WIN32
      return _fseeki64( h->fp, offset, origin ) == 0 ? FLUID_OK : FLUID_FAILED;
#else
      return fseeko( h->fp, offset, origin ) == 0 ? FLUID_OK : FLUID_FAILED;
#endif
    }
    long long base = origin == SEEK_SET ? 0 : origin == SEEK_CUR ? h->pos : static_cast<long long>( h->mem->size() );
    if ( base + offset < 0 || base + offset > static_cast<long long>( h->mem->size() ) ) return FLUID_FAILED;
    h->pos = base + offset;
    return FLUID_OK;
  }

  fluid_long_long_t sfTell( void* handle ) {
    auto* h = static_cast<SfHandle*>( handle );
    if ( h->fp ) {
#ifdef _WIN32
      return _ftelli64( h->fp );
#else
      return ftello( h->fp );
#endif
    }
    return h->pos;
  }

  int sfClose( void* handle ) {
    auto* h = static_cast<SfHandle*>( handle );
    if ( h->fp ) std::fclose( h->fp );
    delete h;
    return FLUID_OK;
  }
}

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

// Turns a file of raw 16-bit little-endian stereo samples into a WAV file.
static void addWavHeader( const std::string& path, const unsigned rate ) {
  std::ifstream in( path, std::ios::binary );
  std::string data( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
  in.close();
  auto le = []( std::string& s, unsigned v, int bytes ) {
    for ( int i = 0; i < bytes; ++i ) s += static_cast<char>( ( v >> ( 8 * i ) ) & 0xff );
  };
  std::string h = "RIFF";
  le( h, 36 + data.size(), 4 );
  h += "WAVEfmt ";
  le( h, 16, 4 ); le( h, 1, 2 ); le( h, 2, 2 ); le( h, rate, 4 ); le( h, rate * 4, 4 ); le( h, 4, 2 ); le( h, 16, 2 );
  h += "data";
  le( h, data.size(), 4 );
  std::ofstream out( path, std::ios::binary | std::ios::trunc );
  out << h << data;
}

MidiPlayer::MidiPlayer() {
  profile( "start" );
  // Settings
  _settingsP = new_fluid_settings();
  // fluidsynth supports the following audio drivers: alsa, file, jack, oss, pulseaudio, sdl2 
  if ( const char* file = std::getenv( "BICYCLE_AUDIO_FILE" ) ) {
    fluid_settings_setstr( _settingsP, "audio.driver", "file" );
    fluid_settings_setstr( _settingsP, "audio.file.name", file );
    // fluidsynth writes WAV only when built with libsndfile; otherwise it writes raw samples, and the destructor
    // puts a WAV header on them.
    if ( fluid_settings_str_equal( _settingsP, "audio.file.type", "raw" ) ||
         fluid_settings_setstr( _settingsP, "audio.file.type", "wav" ) != FLUID_OK ) {
      fluid_settings_setstr( _settingsP, "audio.file.format", "s16" );
      fluid_settings_setstr( _settingsP, "audio.file.endian", "little" );
      _rawFile = file;
    }
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
  if ( fluid_sfloader_t* loader = new_fluid_defsfloader( _settingsP ) ) {
    fluid_sfloader_set_callbacks( loader, sfOpen, sfRead, sfSeek, sfTell, sfClose );
    fluid_synth_add_sfloader( _synthP, loader );  // tried before the built-in one
  }
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
  double rate{ 44100 };
  if ( _settingsP ) fluid_settings_getnum( _settingsP, "synth.sample-rate", &rate );
  if ( _aDriverP ) delete_fluid_audio_driver( _aDriverP );
  if ( _playerP ) delete_fluid_player( _playerP );
  if ( !_rawFile.empty() ) {
    addWavHeader( _rawFile, static_cast<unsigned>( rate ) );
  }
  if ( _synthP ) delete_fluid_synth( _synthP );
  if ( _settingsP ) delete_fluid_settings( _settingsP );
}

// Replaces whatever song was queued.
// The song comes from the asset pack (inflated) or the file.
auto MidiPlayer::loadMidi( const std::string& midiFilepath ) -> bool {
  auto midi = assets::read( midiFilepath );
  if ( !midi ) {
    return false;
  }
  fluid_player_stop( _playerP );
  delete_fluid_player( _playerP );
  _playerP = new_fluid_player( _synthP );
  _tick = 0;
  const bool ok = fluid_player_add_mem( _playerP, midi->data(), midi->size() ) == FLUID_OK;
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
  std::string name = sfFilepath;
  if ( assets::mounted() ) {
    auto data = assets::read( sfFilepath );
    if ( !data ) {
      return -1;
    }
    name = MEM_PREFIX + sfFilepath;
    std::unique_lock<std::mutex> l( memMtx );
    memFiles[name] = std::make_shared<const std::string>( std::move( *data ) );
  }
  else if ( !fileExists( sfFilepath ) ) {
    return -1;
  }
  auto id = fluid_synth_sfload( _synthP, name.c_str(), 1 );
  profile( ( "SoundFont " + sfFilepath ).c_str() );
  return id == FLUID_FAILED ? -1 : id;
}

auto MidiPlayer::selectProgram( const int chan, const int sfontId, const int bank, const int program ) -> bool {
  return fluid_synth_program_select( _synthP, chan, sfontId, bank, program ) == FLUID_OK;
}
