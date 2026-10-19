// Lua FFI: music and sound effects (the MIDI player, see c/MidiPlayer.h).
#include "export.h"
#include "c/MidiPlayer.h"

namespace {
auto player() -> MidiPlayer& { return MidiPlayer::getInstance(); }
}  // namespace

extern "C" {

EXPORT_API bool loadMidi( const char* midiFilepath ) { return player().loadMidi( midiFilepath ); }
EXPORT_API int loadSoundfont( const char* sfFilepath ) { return player().loadSoundfont( sfFilepath ); }
EXPORT_API bool selectProgram( const int chan, const int sfontId, const int bank, const int program ) {
  return player().selectProgram( chan, sfontId, bank, program );
}

EXPORT_API void playSong() { player().playSong(); }
EXPORT_API void stopSong() { player().stopSong(); }
EXPORT_API void pauseSong() { player().pauseSong(); }
EXPORT_API void unpauseSong() { player().unpauseSong(); }
// Fades the music to level (0..1) over ms (scene changes).
EXPORT_API void musicFade( float level, unsigned ms ) { player().fadeTo( level, ms ); }

EXPORT_API void playSound( const int chan, const int key, const int vel ) { player().playSound( chan, key, vel ); }
EXPORT_API void stopSound( const int chan ) { player().stopSound( chan ); }
EXPORT_API void playSoundFX( const int key, const int vel ) { player().playSoundFX( key, vel ); }
EXPORT_API void stopSoundFX() { player().stopSoundFX(); }
EXPORT_API int getSfxChannel() { return MidiPlayer::SFX_CHANNEL; }

}  // extern "C"
