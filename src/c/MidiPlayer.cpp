#include <string>
#include <algorithm>
#include "data/Mem.h"
#include <algorithm>
#include <chrono>
#include "Constants.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#ifndef BICYCLE_RELEASE
#include <fstream>
#endif
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
  std::map<std::string, std::shared_ptr<const mem::Blob>> memFiles;

  struct SfHandle {
    FILE* fp{};
    std::shared_ptr<const mem::Blob> mem;
    long long pos{};
  };

  void* sfOpen( const char* filename ) {
    const std::string name( filename );
    if ( name.rfind( MEM_PREFIX, 0 ) == 0 ) {
      std::unique_lock<std::mutex> l( memMtx );
      auto it = memFiles.find( name );
      return it == memFiles.end() ? nullptr : new SfHandle{ nullptr, it->second, 0 };
    }
#ifdef BICYCLE_RELEASE
    return nullptr;  // release: SoundFonts only come from memory (the embedded pack)
#else
    FILE* fp = std::fopen( filename, "rb" );
    return fp ? new SfHandle{ fp, nullptr, 0 } : nullptr;
#endif
  }

  int sfRead( void* buf, fluid_long_long_t count, void* handle ) {
    auto* h = static_cast<SfHandle*>( handle );
    if ( h->fp ) {
      return std::fread( buf, static_cast<std::size_t>( count ), 1, h->fp ) == 1 ? FLUID_OK : FLUID_FAILED;
    }
    if ( count < 0 || h->pos + count > static_cast<long long>( h->mem->size() ) ) return FLUID_FAILED;
    std::memcpy( buf, h->mem->data() + h->pos, static_cast<std::size_t>( count ) );
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

#ifndef BICYCLE_RELEASE
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
#endif

MidiPlayer::MidiPlayer() {
  // Settings
  _settingsP = new_fluid_settings();
  // fluidsynth supports the following audio drivers: alsa, file, jack, oss, pulseaudio, sdl2 
#ifndef BICYCLE_RELEASE  // writing the audio to a file is a dev tool
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
  else
#endif
  if ( const char* driver = std::getenv( "BICYCLE_AUDIO_DRIVER" ) ) {
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

  // Audio driver output. Without one (e.g. no sound card) the game still runs, silently.
  _aDriverP = new_fluid_audio_driver( _settingsP, _synthP );
  _playerP = new_fluid_player( _synthP );

  // General MIDI fallback: on demand (ensureGm), unless BICYCLE_EAGER_GM=1.
  if ( const char* e = std::getenv( "BICYCLE_EAGER_GM" ) ; e && *e == '1' ) ensureGm( "BICYCLE_EAGER_GM" );
}

// Loads the GM SoundFont once, then reloads the game's SoundFonts (from memory: milliseconds) so they stay on top.
void MidiPlayer::ensureGm( const char* why ) {
  if ( _gmTried ) return;
  _gmTried = true;
  int gmId = FLUID_FAILED;
#ifdef BICYCLE_RELEASE
  // Release: only if it was embedded (-DBICYCLE_EMBED_GM_SF2=ON); never from disk.
  if ( assets::exists( "sf2/default-GM.sf2" ) ) {
    if ( auto data = assets::read( "sf2/default-GM.sf2", mem::AUDIO ) ) {
      const std::string name = MEM_PREFIX + std::string( "sf2/default-GM.sf2" );
      {
        std::unique_lock<std::mutex> l( memMtx );
        memFiles[name] = std::make_shared<const mem::Blob>( std::move( *data ) );
      }
      gmId = fluid_synth_sfload( _synthP, name.c_str(), 1 );
    }
  }
  std::string used = "embedded";
#else
  const char* gm = std::getenv( "BICYCLE_GM_SF2" );
  std::string used;
  // ./sf2/default-GM.sf2 is where to put one on Windows (or anywhere without a system-wide SoundFont).
  for ( const std::string& fp : { std::string( gm ? gm : "" ), std::string( "./sf2/default-GM.sf2" ),
        std::string( "/usr/share/sounds/sf2/default-GM.sf2" ),
        std::string( "/usr/share/sounds/sf2/TimGM6mb.sf2" ), std::string( "/usr/share/sounds/sf2/FluidR3_GM.sf2" ) } ) {
    if ( !fp.empty() && fileExists( fp ) && ( gmId = fluid_synth_sfload( _synthP, fp.c_str(), 1 ) ) != FLUID_FAILED ) {
      used = fp;
      break;
    }
  }
#endif
  if ( gmId == FLUID_FAILED ) return;
  // Restack: each game SoundFont again on top, in its original order.
  for ( auto& f : _gameFonts ) {
    const int fresh = fluid_synth_sfload( _synthP, f.name.c_str(), 0 );
    if ( fresh == FLUID_FAILED ) continue;
    fluid_synth_sfunload( _synthP, f.id, 0 );
    for ( auto& m : _idMap ) if ( m.second == f.id ) m.second = fresh;
    f.id = fresh;
  }
  fluid_synth_program_reset( _synthP );
  for ( int chan = 0; chan < 16; ++chan ) {
    if ( _sel[chan].sfont >= 0 ) fluid_synth_program_select( _synthP, chan, currentId( _sel[chan].sfont ), _sel[chan].bank, _sel[chan].program );
  }
}

auto MidiPlayer::currentId( int luaId ) const -> int {
  for ( const auto& m : _idMap ) if ( m.first == luaId ) return m.second;
  return luaId;
}

auto MidiPlayer::hasPreset( int bank, int program ) const -> bool {
  for ( const auto& f : _gameFonts ) {
    fluid_sfont_t* sf = fluid_synth_get_sfont_by_id( _synthP, f.id );
    if ( sf && fluid_sfont_get_preset( sf, bank, program ) ) return true;
  }
  return false;
}

// The (bank, program) pairs a Standard MIDI File plays notes with: program changes, bank selects (CC0), channel
// 10 as drums (bank 128), and channels that play without a program change (program 0).
static void midiPresets( const uint8_t* d, std::size_t n, std::vector<std::pair<int, int>>& out ) {
  auto be = [&]( std::size_t i, int k ) { uint32_t v = 0; for ( int j = 0; j < k; ++j ) v = v << 8 | d[i + j]; return v; };
  std::size_t i = 0;
  if ( n < 14 || std::memcmp( d, "MThd", 4 ) != 0 ) return;
  i = 8 + be( 4, 4 );
  int bank[16]{}, prog[16]{};
  bool used[16]{};
  auto add = [&]( int c ) {
    const std::pair<int, int> bp{ c == 9 ? 128 : bank[c], prog[c] };
    for ( const auto& o : out ) if ( o == bp ) return;
    out.push_back( bp );
  };
  while ( i + 8 <= n ) {
    const std::size_t len = be( i + 4, 4 ), start = i + 8, end = std::min( n, start + len );
    const bool track = std::memcmp( d + i, "MTrk", 4 ) == 0;
    i = start;
    uint8_t status = 0;
    while ( track && i < end ) {
      while ( i < end && d[i] & 0x80 ) ++i;  // delta time
      ++i;
      if ( i >= end ) break;
      uint8_t b = d[i];
      if ( b == 0xFF || b == 0xF0 || b == 0xF7 ) {
        i += b == 0xFF ? 2 : 1;
        uint32_t l = 0;
        while ( i < end && d[i] & 0x80 ) l = l << 7 | ( d[i++] & 0x7f );
        if ( i < end ) l = l << 7 | d[i++];
        i += l;
        continue;
      }
      if ( b & 0x80 ) { status = b; ++i; }
      if ( !status ) break;
      const int c = status & 0x0f, kind = status & 0xf0;
      const int nData = ( kind == 0xC0 || kind == 0xD0 ) ? 1 : 2;
      if ( i + nData > end ) break;
      if ( kind == 0xC0 ) { prog[c] = d[i]; if ( used[c] ) add( c ); }
      else if ( kind == 0xB0 && d[i] == 0 ) bank[c] = d[i + 1];
      else if ( kind == 0x90 && d[i + 1] > 0 && !used[c] ) { used[c] = true; add( c ); }
      i += nData;
    }
    i = end;
  }
}

MidiPlayer::~MidiPlayer() {
  double rate{ 44100 };
  if ( _settingsP ) fluid_settings_getnum( _settingsP, "synth.sample-rate", &rate );
  if ( _aDriverP ) delete_fluid_audio_driver( _aDriverP );
  if ( _playerP ) delete_fluid_player( _playerP );
#ifndef BICYCLE_RELEASE
  if ( !_rawFile.empty() ) {
    addWavHeader( _rawFile, static_cast<unsigned>( rate ) );
  }
#endif
  if ( _synthP ) delete_fluid_synth( _synthP );
  if ( _settingsP ) delete_fluid_settings( _settingsP );
}

// Replaces whatever song was queued.
// The song comes from the asset pack (inflated) or the file.
auto MidiPlayer::loadMidi( const std::string& midiFilepath ) -> bool {
  auto midi = assets::read( midiFilepath, mem::AUDIO );
  if ( !midi ) {
    return false;
  }
  fluid_player_stop( _playerP );
  delete_fluid_player( _playerP );
  _playerP = new_fluid_player( _synthP );
  _tick = 0;
  const bool ok = fluid_player_add_mem( _playerP, midi->data(), midi->size() ) == FLUID_OK;
  if ( !_gmTried ) {
    std::vector<std::pair<int, int>> presets;
    midiPresets( reinterpret_cast<const uint8_t*>( midi->data() ), midi->size(), presets );
    for ( const auto& [bank, program] : presets ) {
      if ( !hasPreset( bank, program ) ) { ensureGm( ( "song " + midiFilepath ).c_str() ); break; }
    }
  }
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
  if ( !_sfxSelected && !_gmTried && !hasPreset( 0, 0 ) ) ensureGm( "sound effect without a SoundFont" );
  fluid_synth_noteoff( _synthP, SFX_CHANNEL, key );  // retrigger cleanly
  fluid_synth_noteon( _synthP, SFX_CHANNEL, key, vel );
}

void MidiPlayer::playSoundFXPanned( const int key, const int vel, const float pan, const float gain ) {
  const int v = static_cast<int>( vel * std::clamp( gain, 0.f, 1.f ) + 0.5f );
  if ( v <= 0 ) return;  // out of earshot
  const int cc = std::clamp( static_cast<int>( 64 + pan * 63 + 0.5f ), 0, 127 );
  fluid_synth_cc( _synthP, SFX_CHANNEL, 10, cc );
  playSoundFX( key, std::min( 127, v ) );
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
  // The game's SoundFont is read (from the pack or the loose file) into the AUDIO pool and handed to fluidsynth
  // through the in-memory sfloader above. (The GM fallback is streamed from disk: it's ~30 MB.)
  auto data = assets::read( sfFilepath, mem::AUDIO );
  if ( !data ) {
    return -1;
  }
  const std::string name = MEM_PREFIX + sfFilepath;
  {
    std::unique_lock<std::mutex> l( memMtx );
    memFiles[name] = std::make_shared<const mem::Blob>( std::move( *data ) );
  }
  auto id = fluid_synth_sfload( _synthP, name.c_str(), 1 );
  if ( id == FLUID_FAILED ) return -1;
  _gameFonts.push_back( { name, id } );
  _idMap.push_back( { id, id } );
  return id;
}

auto MidiPlayer::selectProgram( const int chan, const int sfontId, const int bank, const int program ) -> bool {
  if ( chan < 0 || chan >= 16 ) return false;
  _sel[chan] = { sfontId, bank, program };
  if ( chan == SFX_CHANNEL ) _sfxSelected = true;
  return fluid_synth_program_select( _synthP, chan, currentId( sfontId ), bank, program ) == FLUID_OK;
}

void MidiPlayer::fadeTo( const float level, const unsigned ms ) {
  if ( !_synthP ) return;
  if ( _baseGain < 0 ) _baseGain = fluid_synth_get_gain( _synthP );
  _target = std::clamp( level, 0.0f, 1.0f );
  const float frames = std::max( 1.0f, ms / static_cast<float>( Constants::MILLISECONDS_PER_FRAME ) );
  _step = ( _target - _level ) / frames;
  if ( ms == 0 ) {
    _level = _target;
    fluid_synth_set_gain( _synthP, _baseGain * _level );
  }
}

void MidiPlayer::tickFade() {
  if ( !_synthP || _baseGain < 0 || _level == _target ) return;
  _level += _step;
  if ( ( _step > 0 && _level >= _target ) || ( _step < 0 && _level <= _target ) || _step == 0 ) _level = _target;
  fluid_synth_set_gain( _synthP, _baseGain * _level );
}
