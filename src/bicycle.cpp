#include "data/Mem.h"
#include <iostream> // TODO: delete when you're done debugging
#include "bicycle.h"
#include "Constants.h"
#include "c/Trigger.h"
#include "c/StartupStats.h"
#include "c/CollisionDetector.h"
#include "c/Physics3D.h"
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
#include <functional>
#include <unordered_map>
#include <cstring>
#include <chrono>
#include <array>
#include <cctype>
#include <cstdio>
#include "c/Input.h"
#include "c/InputState.h"

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
    // (BICYCLE_RELEASE_ASSETS); see include/data/Pack.h. Dev builds read loose files only (mount() refuses there and
    // notes a stale pack it ignores). First, so the renderer can load its fonts from it.
#ifdef BICYCLE_RELEASE
    if ( !assets::mount( gameName ) ) die( "bicycle: the embedded asset pack is corrupt" );
#else
    assets::mount( gameName );
#endif
    startup::mark( "asset pack" );
    gfx::renderer().init( gameName );
    startup::mark( "renderer init (window, GL, fonts)" );
    auto& trig = Trigger::getInstance();
    trig.init( gameName );
    startup::mark( "game main.lua (scene, models, audio)" );
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

  // Graybodies are loaded once and shared: every entity (and every palette variant) drawing the same pixels gets
  // the same image while any of them is alive. Weak entries, so a scene's images still go when it unloads.
  static std::unordered_map<std::string, std::weak_ptr<const BImg>> sharedImages;
  static auto sharedImage( const std::string& key, const std::function<std::optional<BImg>()>& make )
      -> std::shared_ptr<const BImg> {
    if ( auto it = sharedImages.find( key ); it != sharedImages.end() ) {
      if ( auto p = it->second.lock() ) return p;
    }
    auto img = make();
    if ( !img ) return nullptr;
    auto p = std::make_shared<const BImg>( std::move( *img ) );
    sharedImages[key] = p;
    return p;
  }
  static auto loadGraybody( const std::string& path ) -> std::shared_ptr<const BImg> {
    return sharedImage( path, [&] { return loadBImg( path ); } );
  }
  // "i:rgba,i:rgba,..." (decimal; RGBA with R in the low byte) -> palette entries to replace.
  static auto parsePalette( const char* spec ) -> std::vector<std::pair<int, uint32_t>> {
    std::vector<std::pair<int, uint32_t>> out;
    if ( !spec ) return out;
    std::istringstream in( spec );
    std::string item;
    while ( std::getline( in, item, ',' ) ) {
      const auto colon = item.find( ':' );
      if ( colon == std::string::npos ) continue;
      out.emplace_back( std::stoi( item.substr( 0, colon ) ), static_cast<uint32_t>( std::stoul( item.substr( colon + 1 ) ) ) );
    }
    return out;
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
    // A palette variant of a sprite (genome palette = "<name>_palette"): each layer's graybody with some palette
    // entries replaced ("i:rgba,..."). Variants share their graybody's pixels (BImg::gray) and are themselves
    // shared by every entity using the same sprite + palette.
    bool newPixelImage( Entity entity, const char* paths );
    bool newPixelImagePal( Entity entity, const char* paths, const char* palette ) {
      if ( !palette || !*palette ) return newPixelImage( entity, paths );
      std::vector<std::string> layers;
      {
        std::istringstream in( paths );
        std::string path;
        while ( std::getline( in, path ) ) if ( !path.empty() ) layers.push_back( path );
      }
      if ( layers.size() != 1 ) {  // composites are true color: recolor each layer, then stack them
        for ( auto& l : layers ) {
          auto gray = loadGraybody( l );
          if ( !gray || !gray->indexed() ) return false;
        }
        auto img = sharedImage( std::string( paths ) + "|" + palette, [&]() -> std::optional<BImg> {
          std::optional<BImg> out;
          const auto pal = parsePalette( palette );
          for ( auto& l : layers ) {
            auto gray = loadGraybody( l );
            if ( !out ) {
              out.emplace();
              out->w = gray->w;
              out->h = gray->h;
              out->rgba.assign( static_cast<size_t>( gray->w ) * gray->h, 0u );
            }
            auto p = gray->palette;
            for ( auto [i, c] : pal ) if ( i >= 0 && static_cast<size_t>( i ) < p.size() ) p[i] = c;
            for ( int y = 0; y < std::min( out->h, gray->h ); ++y )
              for ( int x = 0; x < std::min( out->w, gray->w ); ++x ) {
                const uint8_t k = gray->indices[static_cast<size_t>( y ) * gray->w + x];
                if ( k && ( p[k] >> 24 ) ) out->rgba[static_cast<size_t>( y ) * out->w + x] = p[k];
              }
          }
          return out;
        } );
        if ( !img ) return false;
        World::set<Image>( entity, pixelImage( img ) );
        World::get<Box>( entity ).size = World::get<Image>( entity ).size;
        return true;
      }
      auto gray = loadGraybody( layers[0] );
      if ( !gray || !gray->indexed() ) return false;
      auto img = sharedImage( layers[0] + "|" + palette, [&]() -> std::optional<BImg> {
        BImg v;
        v.w = gray->w;
        v.h = gray->h;
        v.palette = gray->palette;
        for ( auto [i, c] : parsePalette( palette ) ) {
          if ( i >= 0 && static_cast<size_t>( i ) < v.palette.size() ) v.palette[i] = c;
        }
        v.gray = gray;  // the indices stay the graybody's
        return v;
      } );
      World::set<Image>( entity, pixelImage( img ) );
      World::get<Box>( entity ).size = World::get<Image>( entity ).size;
      return true;
    }

    bool newPixelImage( Entity entity, const char* paths ) {
      // One layer: the shared graybody itself (many coins, one image).
      std::string path( paths );
      while ( !path.empty() && path.back() == '\n' ) path.pop_back();
      if ( !path.empty() && path.find( '\n' ) == std::string::npos ) {
        auto img = loadGraybody( path );
        if ( !img ) return false;
        World::set<Image>( entity, pixelImage( img ) );
        World::get<Box>( entity ).size = World::get<Image>( entity ).size;
        return true;
      }
      std::istringstream in( paths );
      path.clear();
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
    // Strips are keyed by animation tag id (ids.anim in gen/ids.lua; Lua resolves names once, scripts/Ids.lua).
    void newAnimStrip( Entity entity, uint8_t stripId, bool repeat ) {
      Animator::getInstance().newStrip( entity, stripId, repeat );
    }

    void addAnimFrame( Entity entity, uint8_t stripId, unsigned durationMs, int x, int y, int w, int h ) {
      Animator::getInstance().addFrame( entity, stripId, durationMs, Rect( x, y, w, h ) );
    }

    // Switches to and starts a strip; returns false if it's already playing or the entity doesn't have it.
    bool setAnim( Entity entity, uint8_t stripId ) {
      return Animator::getInstance().set( entity, stripId );
    }

    // Release builds treat the generated ids as complete (scripts/Ids.lua).
    bool isReleaseBuild() {
#ifdef BICYCLE_RELEASE_ASSETS
      return true;
#else
      return false;
#endif
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

    // ============
    // SCENE CHANGES (scripts/Scene.lua: Scene.goto)
    // ============
    // Everything the engine keeps for an entity, dropped: timers, animation, motion (and followers of it),
    // collisions, its 3D model, its components; then its id is free again.
    void despawnEntity( Entity entity ) {
      Trigger::forgetEntity( entity );  // queued collisions/timeouts to or from it: never delivered
      Timer::getInstance().stopEntity( entity );
      Animator::getInstance().forget( entity );
      Kinematics::getInstance().forget( entity );
      CollisionDetector::getInstance().removeEntity( entity );
      phys3d::World3D::get().removeEntity( entity );  // true 3D: transform, body, overlaps
      gfx::renderer().forgetEntity( entity );
      World::set<Image>( entity, Image{} );
      World::set<Box>( entity, Box{} );
      World::set<CollBox>( entity, CollBox{} );
      World::set<Rect>( entity, Rect{} );
      delEntityId( entity );
    }

    // While on, images, meshes and songs being loaded go to the SCENE pool (mem::memSceneScope).
    void sceneScope( bool on ) {
      mem::memSceneScope( on );
    }

    // After the old scene's entities and window are gone: the renderer drops what only it used, then SCENE is reset.
    // Returns the SCENE bytes still alive (0 = unloaded cleanly; anything else is a leak and keeps the pool).
    long long sceneUnloaded() {
      gfx::renderer().onSceneUnloaded();
      return static_cast<long long>( mem::memSceneReset() );
    }

    // BICYCLE_SCENE_STATS=1: one line per scene change on stderr (tests/scene_lifecycle_test.sh reads it).
    void sceneStats( const char* event, const char* sceneName, long long leaked ) {
      static const bool on = [] { const char* e = std::getenv( "BICYCLE_SCENE_STATS" ); return e && *e && *e != '0'; }();
      if ( !on ) return;
      const auto s = mem::memStats( mem::SCENE ), g = mem::memStats( mem::GRAPHIC ), a = mem::memStats( mem::AUDIO ),
                 gen = mem::memStats( mem::GENERAL );
      std::fprintf( stderr, "bicycle scene: %s %s scene_hw=%zu scene_live=%zu leaked=%lld graphic_hw=%zu graphic_live=%zu "
                    "audio_live=%zu general_live=%zu\n", event, sceneName, s.highWater, s.inUse, leaked, g.highWater,
                    g.inUse, a.inUse, gen.inUse );
    }

    void setTransition( int kind, float t ) {
      gfx::renderer().setTransition( kind, t );
    }

    void musicFade( float level, unsigned ms ) {
      MidiPlayer::getInstance().fadeTo( level, ms );
    }

    void motionSnap( Entity entity ) {
      Kinematics::getInstance().snap( entity );
    }

    // A persistent entity joins the current scene's layer layerIdx (0-based; clamped to the last layer).
    void attachEntity( Entity entity, unsigned layerIdx ) {
      auto scene = dynamic_pointer_cast<Scene>( WindowManager::getInstance().back() );
      if ( scene && !scene->getGrid().getLayers().empty() ) scene->getGrid().addEntityTo( entity, layerIdx );
    }

    // The window on top: whether it's a scene, and its id (Scene.goto closes dialogues and menus over the scene).
    bool topIsScene() {
      return dynamic_pointer_cast<Scene>( WindowManager::getInstance().back() ) != nullptr;
    }

    Entity topWindowId() {
      auto w = WindowManager::getInstance().back();
      return w ? w->getId() : 0;
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

    // Stops a timer (createTimer's id) before it times out again.
    void stopTimer( const TimerId timerId ) {
      Timer::getInstance().stop( timerId );
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
    // "/1" (or no step) holds the key: pressed on the first frame, released after the last. A step taps: each
    // occurrence presses and the next frame releases.
    struct Press { LogicalKey key; long from, to, every; bool down; };
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
          if ( key != LogicalKey::NOTHING ) script.push_back( { key, from, to, every, false } );
        }
      }
    }

    // BICYCLE_SCRIPTED_MOUSE="12:0@30-60;L@80-80;R@90-95": mouse motion (dx:dy pixels per frame) or a held button
    // (L/R/M) over a frame range, into the polled input state (include/c/InputState.h), like a real mouse.
    // Scripted key presses above also count as held keys on their frames.
    struct MouseScript { float dx, dy; int button; long from, to; };
    std::vector<MouseScript> mouseScript;
    if ( const char* s = std::getenv( "BICYCLE_SCRIPTED_MOUSE" ) ) {
      std::stringstream ss( s );
      std::string item;
      while ( std::getline( ss, item, ';' ) ) {
        MouseScript m{};
        char b{};
        if ( std::sscanf( item.c_str(), "%f:%f@%ld-%ld", &m.dx, &m.dy, &m.from, &m.to ) == 4 ) { m.button = -1; mouseScript.push_back( m ); }
        else if ( std::sscanf( item.c_str(), "%c@%ld-%ld", &b, &m.from, &m.to ) == 3 ) {
          m.button = b == 'L' ? 0 : b == 'R' ? 1 : 2;
          mouseScript.push_back( m );
        }
      }
    }
    unsigned scriptedButtons{};
    const bool frameStats = std::getenv( "BICYCLE_FRAME_STATS" ) && *std::getenv( "BICYCLE_FRAME_STATS" ) == '1';
    std::array<double, 3> acc{};

    while ( wm.size() > 0 ) {
      if ( maxFrames > 0 && frames >= maxFrames ) break;
      // Inputs
      Timer::run();
      auto& held = inputState();
      if ( gfx::renderer().mode() == gfx::Mode::Ascii ) {  // the terminal listener re-marks the keys it holds down
        held.beginFrame();
        held.held.reset();
      }
      gfx::renderer().pollInput();
      for ( auto& p : script ) {
        const bool down = frames >= p.from && frames <= p.to && ( frames - p.from ) % p.every == 0;
        if ( down != p.down ) {
          Input in{};
          if ( down ) in.currKeysPressed.set( static_cast<unsigned>( p.key ) );
          in.lastPressed = p.key;
          Trigger::sendInput( in, down );
          if ( down ) held.pressed.set( static_cast<unsigned>( p.key ) );
          p.down = down;
        }
        if ( down ) held.held.set( static_cast<unsigned>( p.key ) );
      }
      unsigned buttons{};
      float lookDx{}, lookDy{};
      for ( const auto& m : mouseScript ) {
        if ( frames < m.from || frames > m.to ) continue;
        if ( m.button < 0 ) { held.mouseDx += m.dx; held.mouseDy += m.dy; lookDx += m.dx; lookDy += m.dy; }
        else buttons |= 1u << m.button;
      }
      if ( lookDx != 0 || lookDy != 0 ) Trigger::sendLook( lookDx, lookDy );
      for ( unsigned b = 0; b < 3; ++b ) {  // scripted buttons are events too (MouseLeft/Right/Middle)
        const bool now = buttons & ( 1u << b ), before = scriptedButtons & ( 1u << b );
        if ( now == before ) continue;
        Input in{};
        in.lastPressed = static_cast<LogicalKey>( static_cast<int>( LogicalKey::MouseLeft ) + static_cast<int>( b ) );
        Trigger::sendInput( in, now );
      }
      held.mouseButtons = ( held.mouseButtons & ~scriptedButtons ) | buttons;
      held.mousePressed |= buttons & ~scriptedButtons;
      scriptedButtons = buttons;
      ++frames;
      if ( gfx::renderer().quitRequested() ) {
        break;
      }
      using clk = std::chrono::steady_clock;
      const auto t0 = clk::now();
      Kinematics::getInstance().run();  // 2D motions, then the 3D layer (physics, camera rig)
      MidiPlayer::getInstance().tickFade();
      CollisionDetector::check();
      const auto t1 = clk::now();
      // Process inputs
      trigger.send();
      const auto t2 = clk::now();
      // Outputs
      wm.render();   // TODO make this internally only change dirty windows
      if ( frames == 1 ) startup::mark( "first frame" );
      const auto t3 = clk::now();
      // BICYCLE_FRAME_STATS=1: where a frame's time goes (ms, averaged over 90 frames): motion + physics, Lua
      // (events + onFrame), rendering (including the GL work and the swap).
      if ( frameStats ) {
        auto ms = []( clk::duration d ) { return std::chrono::duration<double, std::milli>( d ).count(); };
        acc[0] += ms( t1 - t0 ); acc[1] += ms( t2 - t1 ); acc[2] += ms( t3 - t2 );
        if ( frames % 90 == 0 ) {
          std::fprintf( stderr, "frame: physics %.2f ms, lua %.2f ms, render %.2f ms (avg of 90)\n", acc[0] / 90, acc[1] / 90, acc[2] / 90 );
          acc = {};
        }
      }

      mem::memRst( mem::TEMPORARY );  // scratch never outlives a frame
      Timer::sleepFrame();  // burn off excess frame time
    }

    gfx::renderer().shutdown();
    return 0;
  }

}
