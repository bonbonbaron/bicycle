#include "data/Mem.h"
#include <iostream> // TODO: delete when you're done debugging
#include "bicycle.h"
#include "Constants.h"
#include "c/Trigger.h"
#include "c/CollisionDetector.h"
#include "v/common/Dialogue.h"
#include "v/common/TextMenu.h"
#include "v/common/Scene.h"
#include "c/MidiPlayer.h"
#include "c/Dice.h"
#include "c/Animator.h"
#include "v/common/Renderer.h"
#include "data/Pack.h"
#include "data/BImg.h"
#include <sstream>
#include <cctype>
#include <cstdio>
#include "c/Input.h"

namespace bicycle {

  void die( const std::string&& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    gfx::renderer().shutdown();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

  void die( const std::string& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    gfx::renderer().shutdown();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

  void init( const std::string& gameName ) {
    // Release builds mount the pack linked into the executable (BICYCLE_RELEASE) or <game>/assets.bpak
    // (BICYCLE_RELEASE_ASSETS); see include/data/Pack.h. Dev builds read loose files only. First, so the renderer
    // can load its fonts from it.
#ifdef BICYCLE_RELEASE
    if ( !assets::mount( gameName ) ) die( "bicycle: the embedded asset pack is corrupt" );
#elif defined( BICYCLE_RELEASE_ASSETS )
    assets::mount( gameName );
#endif
    gfx::renderer().init( gameName );
    auto& trig = Trigger::getInstance();
    trig.init( gameName );
  }

  // Pixel images: one pixel is 2x1 cells (see Renderer.h).
  static auto pixelImage( std::shared_ptr<const BImg> px ) -> Image {
    Image img;
    img.size.w = ( 2 * px->w ) << FIXEDPT_DEC_BITS;
    img.size.h = px->h << FIXEDPT_DEC_BITS;
    img.size.d = 1 << FIXEDPT_DEC_BITS;
    gfx::renderer().preloadImage( px );  // into the pixel modes' texture atlas, at load
    img.pixels = std::move( px );
    return img;
  }

  static auto loadBImg( const std::string& path ) -> std::optional<BImg> {
    mem::TempScope scratch;  // the file bytes are scratch; the parsed image lives in GRAPHIC
    auto bytes = assets::read( path, mem::TEMPORARY );
    if ( !bytes ) return std::nullopt;
    return BImg::parse( *bytes );
  }

  Entity push( std::shared_ptr<Window> shared ) {
    auto& wm = WindowManager::getInstance();
    return wm.push( shared );
  }

  extern "C" {
    // DIALOGUES

    Entity pushDialogue( const char* text, int x, int y, int w, int h ) {
      return push<Dialogue>( text, x, y, w, h ); 
    }

    // Whether the top window is a dialogue whose reader pressed E or Space on its last page.
    bool isDialogueDone() {
      auto dialogue = dynamic_pointer_cast<Dialogue>( WindowManager::getInstance().back() );
      return dialogue != nullptr && dialogue->isDone();
    }

    int getScreenWidth() {
      return gfx::cols();
    }

    int getScreenHeight() {
      return gfx::lines();
    }

    // MENUS

    Entity pushTextMenu( int x, int y, int w, int h ) {
      return push<TextMenu>( x, y, w, h ); 
    }

    void addItem( const char* text ) {
      auto& wm = WindowManager::getInstance();
      auto menu = dynamic_pointer_cast<Menu>( wm.back() );
      if ( menu != nullptr ) {
        menu->addItem( text );
      }
    }

    unsigned getSelection() {
      auto& wm = WindowManager::getInstance();
      auto menu = dynamic_pointer_cast<Menu>( wm.back() );
      if ( menu != nullptr ) {
        return menu->getSelection();
      }
      return 0;  // i'm tired, i'll make this -1 (int, not unsigned) later
    }

    // SCENES

    Entity pushScene() {
      return push<Scene>();
    }

    // TODO handle collision string and type later
    void addBgLayer( const char* bgStr ) {
      auto& wm = WindowManager::getInstance();
      auto scene = dynamic_pointer_cast<Scene>( wm.back() );
      auto& grid = scene->getGrid();
      Layer layer{ std::string( bgStr ), "", LayerType::FIXED };  // TODO
      grid.addLayer( layer );
    }

    // type is a LayerType; vx/vy are fixed-point cells per frame (AUTOLOOP); x/y offset the layer in cells.
    void addBgLayerEx( const char* bgStr, int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y ) {
      auto& wm = WindowManager::getInstance();
      auto scene = dynamic_pointer_cast<Scene>( wm.back() );
      auto& grid = scene->getGrid();
      Position vel{};
      vel.x = vx;
      vel.y = vy;
      Layer layer{ std::string( bgStr ), static_cast<LayerType>( type ), parallaxPct, parallaxPctY, vel, Position( x, y ) };
      grid.addLayer( layer );
    }

    Entity newEntity() {
      return newEntityId();
    }

    /* Really, we have access to all the world arrays and custom
     * data from the front end. So there's no need to send any
     * information other than entity ID. */
    void addFgEntity( Entity entity, const unsigned layerIdx ) {
      auto& wm = WindowManager::getInstance();
      auto scene = dynamic_pointer_cast<Scene>( wm.back() );
      auto& grid = scene->getGrid();
      grid.addEntity( entity );
    }

    void newImage( Entity entity, const char* imgStr ) {
      World::set<Image>( entity, imgStr );
      // Use image size to set box size while we're at it.
      const auto& img = World::get<Image>( entity );
      auto& box = World::get<Box>( entity );
      box.size = img.size;
    }

    // Which renderer this build has: 0 ASCII, 1 2D, 2 3D (Lua picks ASCII art or pixels by it).
    int renderMode() {
      return static_cast<int>( gfx::renderer().mode() );
    }

    // A pixel image from .bimg layers (newline-separated paths, bottom layer first) composited in order. Layers
    // are frame strips of the same size (scripts/aseprite/sprite.lua pixel export). Returns false if one's missing.
    bool newPixelImage( Entity entity, const char* paths ) {
      std::istringstream in( paths );
      std::string path;
      std::optional<BImg> out;
      int nLayers{};
      while ( std::getline( in, path ) ) {
        if ( path.empty() ) continue;
        auto layer = loadBImg( path );
        if ( !layer ) return false;
        if ( !out ) {
          out = std::move( *layer );
        }
        else {
          if ( nLayers == 1 && out->indexed() ) {  // second layer: switch the result to true color
            out->rgba.resize( static_cast<size_t>( out->w ) * out->h );
            for ( int y = 0; y < out->h; ++y )
              for ( int x = 0; x < out->w; ++x ) out->rgba[static_cast<size_t>( y ) * out->w + x] = out->pixel( x, y );
            out->palette.clear();
            out->indices.clear();
          }
          for ( int y = 0; y < std::min( out->h, layer->h ); ++y ) {
            for ( int x = 0; x < std::min( out->w, layer->w ); ++x ) {
              const uint32_t p = layer->pixel( x, y );
              if ( p >> 24 ) out->rgba[static_cast<size_t>( y ) * out->w + x] = p;
            }
          }
        }
        ++nLayers;
      }
      if ( !out ) return false;
      World::set<Image>( entity, pixelImage( std::make_shared<const BImg>( std::move( *out ) ) ) );
      World::get<Box>( entity ).size = World::get<Image>( entity ).size;
      return true;
    }

    // A tilemap background layer in pixels: tiles[] (row-major, nw x nh) index the tileset strip (tile i at
    // x = i * tileW). Other parameters as addBgLayerEx. Returns false if the tileset is missing.
    bool addBgLayerPixels( const char* tilesetPath, const int* tiles, int nw, int nh, int tileW, int tileH,
                           int type, int parallaxPct, int parallaxPctY, int vx, int vy, int x, int y ) {
      auto set = loadBImg( tilesetPath );
      if ( !set || tileW <= 0 || tileH <= 0 ) return false;
      BImg map;
      map.w = nw * tileW;
      map.h = nh * tileH;
      map.palette = set->palette;
      if ( set->indexed() ) map.indices.assign( static_cast<size_t>( map.w ) * map.h, 0 );
      else map.rgba.assign( static_cast<size_t>( map.w ) * map.h, 0 );
      const int nTiles = set->w / tileW;
      for ( int ty = 0; ty < nh; ++ty ) {
        for ( int tx = 0; tx < nw; ++tx ) {
          const int t = tiles[ty * nw + tx];
          if ( t <= 0 || t >= nTiles ) continue;  // tile 0 is Aseprite's empty tile
          for ( int py = 0; py < std::min( tileH, set->h ); ++py ) {
            for ( int px = 0; px < tileW; ++px ) {
              const size_t src = static_cast<size_t>( py ) * set->w + t * tileW + px;
              const size_t dst = static_cast<size_t>( ty * tileH + py ) * map.w + tx * tileW + px;
              if ( set->indexed() ) map.indices[dst] = set->indices[src];
              else map.rgba[dst] = set->rgba[src];
            }
          }
        }
      }
      auto scene = dynamic_pointer_cast<Scene>( WindowManager::getInstance().back() );
      if ( !scene ) return false;
      Position vel{};
      vel.x = vx;
      vel.y = vy;
      scene->getGrid().addLayer( Layer( pixelImage( std::make_shared<const BImg>( std::move( map ) ) ),
                                        static_cast<LayerType>( type ), parallaxPct, parallaxPctY, vel, Position( x, y ) ) );
      return true;
    }

    // 3D mode: draw the entity as this mesh (an asset path of a .bmesh). False in other modes or if it's missing.
    bool setModel( Entity entity, const char* bmeshPath ) {
      return gfx::renderer().setModel( entity, bmeshPath );
    }

    // 3D mode: the action a rigged model plays (Animation:set maps strip tags to actions).
    void setModelAction( Entity entity, const char* action ) {
      gfx::renderer().setModelAction( entity, action );
    }

    // 3D mode: static scenery (ground, props) exported from Blender with its world transform baked in.
    bool addStaticModel( const char* bmeshPath ) {
      return gfx::renderer().addStaticModel( bmeshPath );
    }

    void setPos( Entity entity, Position pos ) {
      auto& box = World::get<Box>( entity );
      box.pos = pos << FIXEDPT_DEC_BITS;
    }

    // ANIMATION
    // Strips are built once per entity: newAnimStrip(), then addAnimFrame() for each frame in play order.
    // Src rects are in character cells within the entity's image (all frames side by side).
    void newAnimStrip( Entity entity, const char* stripName, bool repeat ) {
      Animator::getInstance().newStrip( entity, stripName, repeat );
    }

    void addAnimFrame( Entity entity, const char* stripName, unsigned durationMs, int x, int y, int w, int h ) {
      Animator::getInstance().addFrame( entity, stripName, durationMs, Rect( x, y, w, h ) );
    }

    // Switches to and starts a strip; returns false if it's already playing or doesn't exist.
    bool setAnim( Entity entity, const char* stripName ) {
      return Animator::getInstance().set( entity, stripName );
    }

    void stopAnim( Entity entity ) {
      Animator::getInstance().stop( entity );
    }

    unsigned getAnimFrameIdx( Entity entity ) {
      return Animator::getInstance().getFrameIdx( entity );
    }

    // COLLISION
    // Registers an entity for collision checks against others on the same layer. x/y/w/h is its collision
    // box relative to its position, in cells; w or h of 0 means "use the entity's whole box".
    void addCollider( Entity entity, unsigned layer, unsigned type, int x, int y, int w, int h ) {
      World::set<CollBox>( entity, CollBox( Box( Position( x, y ), Size( w, h ) ), type ) );
      CollisionDetector::registerEntity( entity, layer );
    }

    // MOTION
    // Constant velocity in fixed-point cells per frame (256 = one cell per frame).
    void setVelocity( Entity entity, int vx, int vy ) {
      Position vel{};
      vel.x = vx;
      vel.y = vy;
      Kinematics::getInstance().setVelocity( entity, vel );
    }

    // Scripted motions (cells, seconds, degrees; see Kinematics.h and the motion gene in scripts/Genome.lua).
    void motionLinear( Entity entity, float speed, bool pingPong ) { Kinematics::getInstance().linear( entity, speed, pingPong ); }
    void motionWaypoint( Entity entity, float x, float y ) { Kinematics::getInstance().addWaypoint( entity, x, y ); }
    void motionOrbit( Entity entity, Entity center, float cx, float cy, float rx, float ry, float degPerSec, float phaseDeg ) {
      Kinematics::getInstance().orbit( entity, center, cx, cy, rx, ry, degPerSec, phaseDeg );
    }
    void motionCombo( Entity entity, float rx, float ry, float degPerSec, float phaseDeg, float speed, bool pingPong ) {
      Kinematics::getInstance().combo( entity, rx, ry, degPerSec, phaseDeg, speed, pingPong );
    }
    void motionFollow( Entity entity, Entity target, float offX, float offY, float rate, float bobRows, float bobHz,
                       bool mirror, float deadX, float deadY ) {
      Kinematics::getInstance().follow( entity, target, offX, offY, rate, bobRows, bobHz, mirror, deadX, deadY );
    }
    void motionStop( Entity entity ) { Kinematics::getInstance().stopMotion( entity ); }
    int motionType( Entity entity ) { return Kinematics::getInstance().motionType( entity ); }

    // HUD
    static auto currScene() -> std::shared_ptr<Scene> {
      return dynamic_pointer_cast<Scene>( WindowManager::getInstance().back() );
    }

    // A meter glued to the current scene at screen cell (x, y), starting full; returns its id (-1 if no scene).
    int addHudBar( const char* label, int max, int len, int x, int y ) {
      auto scene = currScene();
      return scene ? scene->addHudBar( label, max, len, x, y ) : -1;
    }

    void setHudBar( int id, int val ) {
      auto scene = currScene();
      if ( Bar* bar = scene ? scene->getHudBar( id ) : nullptr ) {
        bar->setValue( val );
      }
    }

    // Pixel modes: draw HUD bar id from images (asset paths of .bimg files; either may be null or "").
    // The frame is drawn as is over the fill, which is cropped to the bar's fraction. ASCII ignores this.
    bool setHudBarSprites( int id, const char* framePath, const char* fillPath ) {
      auto scene = currScene();
      Bar* bar = scene ? scene->getHudBar( id ) : nullptr;
      if ( !bar ) return false;
      auto load = []( const char* p ) -> std::shared_ptr<const BImg> {
        if ( !p || !*p ) return nullptr;
        auto img = loadBImg( p );
        return img ? std::make_shared<const BImg>( std::move( *img ) ) : nullptr;
      };
      auto frame = load( framePath ), fill = load( fillPath );
      bar->setSprites( frame, fill );
      return ( !framePath || !*framePath || frame ) && ( !fillPath || !*fillPath || fill );
    }

    void setHidden( Entity entity, bool hidden ) {
      auto scene = currScene();
      if ( scene ) {
        scene->setHidden( entity, hidden );
      }
    }

    Entity popWindow() {
      return pop();
    }
      
    // The current scene's camera entity (a kinematic body: Camera.track in scripts/bicycle.lua drives it).
    Entity cameraEntity() {
      auto scene = dynamic_pointer_cast<Scene>( WindowManager::getInstance().back() );
      return scene ? scene->cameraId() : 0;
    }

    void focusCamOn( Entity entity ) {
      auto& wm = WindowManager::getInstance();
      auto scene = dynamic_pointer_cast<Scene>( wm.back() );
      scene->setFocus( entity );
    }

    void sys(const Trigger::Action action, const Trigger::System system, Entity entity) {
      auto& trig = Trigger::getInstance();
      trig.sys( action, system, entity );
    }

    // ============
    // MIDI PLAYER 
    // ============
    void playSound( const int chan, const int key, const int vel ) {
      auto& player = MidiPlayer::getInstance();
      player.playSound( chan, key, vel );
    }
    void stopSound( const int chan ) {
      auto& player = MidiPlayer::getInstance();
      player.stopSound( chan );
    }
    void stopSoundFX() {
      auto& player = MidiPlayer::getInstance();
      player.stopSoundFX();
    }
    void playSoundFX( const int key, const int vel ) {
      auto& player = MidiPlayer::getInstance();
      player.playSoundFX(key, vel);
    }
    void playSong() {
      auto& player = MidiPlayer::getInstance();
      player.playSong();
    }
    void stopSong() {
      auto& player = MidiPlayer::getInstance();
      player.stopSong();
    }
    void pauseSong() {
      auto& player = MidiPlayer::getInstance();
      player.pauseSong();
    }
    void unpauseSong() {
      auto& player = MidiPlayer::getInstance();
      player.unpauseSong();
    }
    bool loadMidi( const char* midiFilepath ) {
      auto& player = MidiPlayer::getInstance();
      return player.loadMidi( midiFilepath );
    }
    int loadSoundfont( const char* sfFilepath ) {
      auto& player = MidiPlayer::getInstance();
      return player.loadSoundfont( sfFilepath );
    }
    bool selectProgram( const int chan, const int sfontId, const int bank, const int program ) {
      auto& player = MidiPlayer::getInstance();
      return player.selectProgram( chan, sfontId, bank, program );
    }
    int getSfxChannel() {
      return MidiPlayer::SFX_CHANNEL;
    }

    // Timer
    // auto Timer::create( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat, const TimeoutAddr addr) -> TimerId {
    TimerId createTimer( const unsigned timeMs, Entity entity, const unsigned timeoutType, const bool repeat, const TimeoutAddr addr ) {
      auto& timer = Timer::getInstance();
      return timer.create( timeMs, entity, timeoutType, repeat, addr );
    }

    // ASSETS (the game's asset pack, or loose files; see include/data/Pack.h)
    // Returns the asset's bytes (valid until the next call) and their length, or null.
    const char* assetRead( const char* path, size_t* len ) {
      static thread_local mem::Blob buf;
      auto data = assets::read( path, mem::TEXT );
      if ( !data ) {
        return nullptr;
      }
      buf = std::move( *data );
      *len = buf.size();
      return buf.data();
    }

    bool assetExists( const char* path ) {
      return assets::exists( path );
    }

    bool assetPacked() {
      return assets::mounted();
    }

    // Newline-separated paths of the pack's files in a folder ("" without a pack).
    const char* assetList( const char* dir ) {
      static thread_local std::string buf;
      buf.clear();
      for ( const auto& p : assets::list( dir ) ) {
        buf += p + "\n";
      }
      return buf.c_str();
    }

    int rollDice( const int min, const int max ) {
      auto d = Dice( min, max );
      return d.roll();
    }
  }  // extern "C"

  Entity pop() {
    auto& wm = WindowManager::getInstance();
    return wm.pop();
  }

  int run() {
    auto& wm = WindowManager::getInstance();
    auto& trigger = Trigger::getInstance();
    // BICYCLE_MAX_FRAMES=<n>: quit after n frames (headless tests, e.g. tests/release_io_test.sh).
    long maxFrames = 0, frames = 0;
    if ( const char* s = std::getenv( "BICYCLE_MAX_FRAMES" ) ) maxFrames = std::atol( s );

    // BICYCLE_SCRIPTED_INPUT="d@30-60/3,a@70-90/2": press D every 3rd frame from frame 30 to 60, then A every 2nd
    // from 70 to 90 (WASD/E/space), through the same path as real keys. For headless captures and tests, any mode.
    struct Press { LogicalKey key; long from, to, every; };
    std::vector<Press> script;
    if ( const char* s = std::getenv( "BICYCLE_SCRIPTED_INPUT" ) ) {
      std::stringstream ss( s );
      std::string item;
      while ( std::getline( ss, item, ',' ) ) {
        char k{};
        long from{}, to{}, every{ 1 };
        if ( std::sscanf( item.c_str(), "%c@%ld-%ld/%ld", &k, &from, &to, &every ) >= 3 && every > 0 ) {
          const char c = static_cast<char>( std::tolower( static_cast<unsigned char>( k ) ) );
          const LogicalKey key = c == ' ' ? LogicalKey::Space : c >= 'a' && c <= 'z' ? static_cast<LogicalKey>( c - 'a' + 1 ) : LogicalKey::NOTHING;
          if ( key != LogicalKey::NOTHING ) script.push_back( { key, from, to, every } );
        }
      }
    }

    while ( wm.size() > 0 ) {
      if ( maxFrames > 0 && frames >= maxFrames ) break;
      // Inputs
      Timer::run();
      gfx::renderer().pollInput();
      for ( const auto& p : script ) {
        if ( frames >= p.from && frames <= p.to && ( frames - p.from ) % p.every == 0 ) {
          Input in{};
          in.currKeysPressed.set( static_cast<unsigned>( p.key ) );
          in.lastPressed = p.key;
          Trigger::sendInput( in );
        }
      }
      ++frames;
      if ( gfx::renderer().quitRequested() ) {
        break;
      }
      Kinematics::getInstance().run();
      CollisionDetector::check();
      // Process inputs
      trigger.send();
      // Outputs
      wm.render();   // TODO make this internally only change dirty windows

      mem::memRst( mem::TEMPORARY );  // scratch never outlives a frame
      Timer::sleepFrame();  // burn off excess frame time
    }

    gfx::renderer().shutdown();
    return 0;
  }

}
