// The engine's top level: start a game, then run its frames until the last window closes.
// What Lua can call is in src/lua/ (one file per area); the frame's parts are the systems in src/c/.
#include "bicycle.h"
#include "c/Animator.h"
#include "c/CollisionDetector.h"
#include "c/InputState.h"
#include "c/Kinematics.h"
#include "c/MidiPlayer.h"
#include "c/ScriptedInput.h"
#include "c/Timer.h"
#include "c/Trigger.h"
#include "data/Mem.h"
#include "data/Pack.h"
#include "v/common/Renderer.h"
#include <cstdlib>
#include <iostream>

namespace bicycle {

  void die( const std::string& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    gfx::renderer().shutdown();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit( 1 );
  }

  void init( const std::string& gameName ) {
    // Release builds mount the pack linked into the executable (BICYCLE_RELEASE) or <game>/assets.bpak
    // (BICYCLE_RELEASE_ASSETS); see include/data/Pack.h. Dev builds read loose files only (mount() refuses there and
    // notes a stale pack it ignores). First, so the renderer can load its fonts from it.
#ifdef BICYCLE_RELEASE
    if ( !assets::mount( gameName ) ) die( "bicycle: the embedded asset pack is corrupt" );
#else
    assets::mount( gameName );
#endif
    gfx::renderer().init( gameName );
    Trigger::getInstance().init( gameName );
  }

  Entity push( std::shared_ptr<Window> shared ) {
    return WindowManager::getInstance().push( shared );
  }

  Entity pop() {
    return WindowManager::getInstance().pop();
  }

  int run() {
    auto& wm = WindowManager::getInstance();
    auto scripted = ScriptedInput::fromEnv();
    // BICYCLE_MAX_FRAMES=<n>: quit after n frames (headless tests, e.g. tests/release_io_test.sh).
    const char* max = std::getenv( "BICYCLE_MAX_FRAMES" );
    const long maxFrames = max ? std::atol( max ) : 0;

    for ( long frame = 0; wm.size() > 0 && ( maxFrames <= 0 || frame < maxFrames ); ) {
      // Input
      Timer::run();
      if ( gfx::renderer().mode() == gfx::Mode::Ascii ) {  // the terminal listener re-marks the keys it holds down
        inputState().beginFrame();
        inputState().held.reset();
      }
      gfx::renderer().pollInput();
      scripted.play( frame );
      ++frame;
      if ( gfx::renderer().quitRequested() ) break;

      // The world moves
      Kinematics::getInstance().run();  // 2D motions, then the 3D layer (physics, camera rig)
      MidiPlayer::getInstance().tickFade();
      CollisionDetector::check();

      // The game reacts (Lua: input, collisions, timeouts)
      Trigger::getInstance().send();

      // Output
      wm.render();

      mem::memRst( mem::TEMPORARY );  // scratch never outlives a frame
      Timer::sleepFrame();            // burn off the rest of the frame's time
    }

    gfx::renderer().shutdown();
    return 0;
  }

}  // namespace bicycle
