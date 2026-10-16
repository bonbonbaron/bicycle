#include <iostream> // TODO: delete when you're done debugging
#include "bicycle.h"
#include "Constants.h"
#include "c/Trigger.h"
#include "c/SshInputListener.h"
#include "c/CollisionDetector.h"
#include "v/Dialogue.h"
#include "v/TextMenu.h"
#include "v/Scene.h"
#include "c/MidiPlayer.h"
#include "c/Dice.h"
#include "c/Animator.h"

namespace bicycle {

  void die( const std::string&& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    endwin();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

  void die( const std::string& s ) {
    std::unique_lock<std::mutex> l( bike_mtx );
    endwin();
    std::cerr << "\e[91m" << s << "\e[0m\n";
    exit(1);
  }

  void init( const std::string& gameName ) {
    // Init ncurses
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    cbreak();  // reads each character as they're typed instead of waiting for newline
    noecho();  // doesn't echo user's input back to them
    curs_set(0);
    ColorPalette::init();
    clear();
    refresh();
    auto& trig = Trigger::getInstance();
    trig.init( gameName );
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

    // Text box glued to the bottom of the current scene: one row per '\n'-separated line; "" hides it.
    void setTextBox( const char* text ) {
      auto scene = currScene();
      if ( ! scene ) {
        return;
      }
      std::vector<std::string> lines{};
      std::string s( text ? text : "" );
      for ( size_t start = 0; ! s.empty() && start <= s.size(); ) {
        size_t end = s.find( '\n', start );
        if ( end == std::string::npos ) {
          end = s.size();
        }
        lines.push_back( s.substr( start, end - start ) );
        start = end + 1;
      }
      scene->setTextBox( lines );
    }

    void setHidden( Entity entity, bool hidden ) {
      auto scene = currScene();
      if ( scene ) {
        scene->setHidden( entity, hidden );
      }
    }

    int getTextBoxWidth() {
      auto scene = currScene();
      return scene ? scene->getTextBoxWidth() : 40;
    }

    Entity popWindow() {
      return pop();
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

    while ( wm.size() > 0 ) {
      // Inputs
      Timer::run();
      SshInput::listen();
      Kinematics::getInstance().run();
      CollisionDetector::check();
      // Process inputs
      trigger.send();
      // Outputs
      wm.render();   // TODO make this internally only change dirty windows

      // This is dummy-code for testing colors.
      // std::array<chtype, 100> chs{};
      // chs.fill( 'H' | COLOR_PAIR(RED) );
      // chs.at(chs.size() - 1 ) = '\0';
      // move(10,10);
      // addchstr(chs.data());
      // refresh();

      Timer::sleepFrame();  // burn off excess frame time
    }

    return endwin();
  }

}
