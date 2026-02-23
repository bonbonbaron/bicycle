#include "Grid.h"
#include <algorithm>

//************************
//   Camera
//************************

Camera::Camera( const int x, const int y, const int margin ) : _x(x), _y(y), _margin(margin) {}

void Camera::pan( const int dy, const int dx ) {
  _x += dx;
  _y += dy;
  _x = std::clamp<int>( _x, 0, _maxX );
  _y = std::clamp<int>( _y, 0, _maxY );
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Camera::canSee( const std::shared_ptr<Entity>& entity ) const -> bool {
  auto pos = entity->body.getPosition();
  return 
    pos->x >= _x && 
    pos->x <= _x + _w &&
    pos->y >= _y && 
    pos->y <= _y + _h;
}

void Camera::focusOn( const std::shared_ptr<Entity> entity ) {
  _focus = entity;
}

auto Camera::getFocus() const -> std::shared_ptr<Entity> {
  return _focus;
}

void Camera::setDims( const int h, const int w) {
  _h = h;
  _w = w;
}

void Camera::setLims( const int y, const int x) {
  _maxY = y;
  _maxX = x;
}

auto Camera::getX() const -> int {
  return _x;
}

auto Camera::getY() const -> int {
  return _y;
}

auto Camera::getLxMargin() const -> int {
  return _x + _margin;
}

auto Camera::getHxMargin() const -> int {
  return _x + _w - _margin + (WINDOW_PADDING/2);
}

auto Camera::getLyMargin() const -> int {
  return _y + _margin;
}

auto Camera::getHyMargin() const -> int {
  return _y + _h - _margin + (WINDOW_PADDING/2);
}



//************************
//   Grid
//************************

Grid::Grid( const std::string& gridName ) : Window() {
  _env = std::make_shared<Environment>();
  auto yamlFilename = GRID_DIR + gridName + SUFFIX.data();
  try {
    auto root = YAML::LoadFile( yamlFilename );

    /*****************
     * Background
     * ***************/
    auto bgFilename = root["bg"].as<std::string>();

    // Create window
    auto w = root["w"].as<int>();
    setWidth( w );
    auto h = root["h"].as<int>();
    setHeight( h );
    if ( auto x = root["x"] ) {
      setX( x.as<int>() );
    }
    if ( auto y = root["y"] ) {
      setY( y.as<int>() );
    }
    create();

    // Open background file
    auto BG_FP = BG_DIR + bgFilename;
    auto bgFile = std::ifstream( BG_FP, std::ios::ate );
    if ( !bgFile ) {
      throw std::runtime_error( "error processing Grid " + BG_FP );
    }
    // Get background size
    auto size = bgFile.tellg();

    if ( size == 0 ) {
      throw std::runtime_error( BG_FP + " is empty." );
    }

    // Get backround data/string
    std::string str(size, '\0'); // construct string to stream size
    bgFile.seekg(0);
    if(bgFile.read(&str[0], size)) {
      _env->bg = str;
    }

    // Get background dimensions before pruning newlines.
    // TODO does line.length() include new lines?
    bgFile.seekg(0);
    std::string line;
    while ( std::getline( bgFile, line ) ) {
      ++_env->bgDims.h;
      if ( line.length() > _env->bgDims.w ) {
        _env->bgDims.w = line.length();
      }
    }

    // Prune newlines out. They'd mess up drawing.
    auto idx = _env->bg.find_first_of( '\n' );
    while ( idx != std::string::npos ) {
      _env->bg.erase( idx, 1 );
      idx = _env->bg.find_first_of( '\n' );
    }

    /*****************
     * Foreground
     * ***************/
    auto entities = root["fg"];
    if ( !entities.IsMap() ) {
      bicycle::die( "Grid " + gridName + "'s entities node needs to be a map." );
    }

    // For each entity, the key is the entity name, val is position.
    for ( const auto& e : entities ) {
      auto entityName = e.first.as<std::string>();
      YAML::Node entityNode;
      try {
        entityNode = YAML::LoadFile( ENTITY_DIR + entityName + SUFFIX.data() );
      }
      catch ( const YAML::BadFile& e ) {
        bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": We couldn't find " + ENTITY_DIR + entityName + SUFFIX.data() + "." );
      }
      Entity entity;
      // Sequence of 2 integers = one instance's position
      if ( e.second[0].IsScalar() && e.second.size() == 2 ) {
        try {
          entity = entityNode.as<Entity>();
        } 
        catch ( const std::runtime_error& e ) {
          bicycle::die( "Error for entity \'" + entityName + "\':\n" + e.what() );
        }
        if ( ! e.second.IsSequence() ) {
          bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": pos node needs to be a sequence.\n" );
        }
        auto position = e.second.as<Position>();
        auto posPtr = std::make_shared<Position>( position );
        entity.body.setPosition( posPtr );
        // Copy the entity into a shared pointer object
        auto entityPtr = std::make_shared<Entity>( entity );
        (*entityPtr->bb)["pos"] = std::make_any<std::shared_ptr<Position>>( posPtr );  // share pos with bb
        (*entityPtr->bb)["env"] = _env;  // share environment to open the world up to each entity
        entityPtr->personality.distributeBlackboard( entityPtr->bb );
        addEntity( entityName, entityPtr );
      }
      // Sequence of at least 1 position
      else if ( e.second.IsSequence() ) {
        try {
          auto positions = e.second.as<std::vector<Position>>();
          for ( auto& pos : positions ) {
            try {
              entity = entityNode.as<Entity>();
            } 
            catch ( const std::runtime_error& e ) {
              bicycle::die( "Error for entity \'" + entityName + "\':\n" + e.what() );
            }
            if ( ! e.second.IsSequence() ) {
              bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": pos node needs to be a sequence.\n" );
            }
            auto posPtr = std::make_shared<Position>( pos );
            entity.body.setPosition( posPtr );
            auto entityPtr = std::make_shared<Entity>( entity );
            (*entityPtr->bb)["pos"] = std::make_any<std::shared_ptr<Position>>( posPtr );  // share pos with bb
            (*entityPtr->bb)["env"] = _env;  // share environment to open the world up to each entity
            entityPtr->personality.distributeBlackboard( entityPtr->bb );
            addEntity( entityName, entityPtr );
          }
        }
        catch ( const YAML::Exception& e ) {
          bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": pos error: " + e.what() );
        }
      }
    }  // for each entity in grid's entites list

    /*****************
     *   FOCUS
     *****************/

    // Camera
    _env->camera.setDims( getHeight() - WINDOW_PADDING, getWidth() - WINDOW_PADDING );
    _env->camera.setLims( _env->bgDims.h - getHeight() + WINDOW_PADDING , _env->bgDims.w - getWidth() + WINDOW_PADDING );

    std::string focus;
    try {
      focus = root[ "focus" ].as<std::string>();
      _env->camera.focusOn( _env->fg.at( focus ) );
    }
    catch ( const YAML::Exception& e ) {
      bicycle::die( GRID_DIR + gridName + ".yml is missing a 'focus' attribute." );
    }
    catch ( const std::out_of_range& e ) {
      bicycle::die( gridName + " tried to focus on an entity it doesn't have: " + focus );
    }

    // Kick off.
    for ( auto& [ name, entity ] : _env->fg ) {
      entity->personality.trigger( "onStart" );
    }
  }  // try-block
  catch ( const YAML::Exception& e ) {
    std::stringstream ss;
    ss << "Error reading from file " << yamlFilename << ":\n" << e.what();
    bicycle::die( ss.str() );
  }
}


void Grid::addEntity( const std::string& name, const std::shared_ptr<Entity>& entity ) {
  int attempt{};
  std::string attemptedName = name;
  while ( _env->fg.contains( attemptedName ) ) {
    attemptedName = name + " " + std::to_string( ++attempt );
  }
  _env->fg[ attemptedName ] = entity;
}


Grid::Grid( const int x, const int y, const int w, const int h ) : Window(x, y, w, h) {}

void Grid::update() {
  // First, pan the camera if necessary. (TODO this should be outsourced to the controller side to make this a pure "view" layer of MVC.)
  auto fpos = _env->camera.getFocus()->body.getPosition();
  auto lx = _env->camera.getLxMargin();
  auto hx = _env->camera.getHxMargin();
  auto ly = _env->camera.getLyMargin();
  auto hy = _env->camera.getHyMargin();
  if ( fpos->x <= lx ) {
    _env->camera.pan( 0, fpos->x - lx );
  }
  else if ( fpos->x >= hx ) {
    _env->camera.pan( 0, fpos->x - hx );
  }
  if ( fpos->y <= ly ) {
    _env->camera.pan( fpos->y - ly, 0 );
  }
  else if ( fpos->y >= hy ) {
    _env->camera.pan( fpos->y - hy, 0 );
  }

  // Background
  for ( int row = 0; row < getHeight() - WINDOW_PADDING; ++row ) {
    int startPos =  ( _env->camera.getY() + row ) * _env->bgDims.w + _env->camera.getX();
    int stringLength =  getWidth() - WINDOW_PADDING;
    auto rowStr = std::string( _env->bg, startPos, stringLength );
    mvprint( row + 1, 1, rowStr );  // row + 1 to skip the stop border 
  }

  // Foreground
  for ( const auto& [ name, entity ] : _env->fg ) {
    if ( _env->camera.canSee( entity ) ) {
      auto pos = entity->body.getPosition();
      auto x = pos->x - _env->camera.getX();
      auto y = pos->y - _env->camera.getY();
      setAttr ( COLOR_PAIR(entity->body.getColor()) );
      mvprint( y, x, entity->body.getSymbol() );
      unsetAttr ( COLOR_PAIR(entity->body.getColor()) );
    }
  }
}

void Grid::react( const int input ) {
  auto focus = _env->camera.getFocus();
  if ( focus != nullptr && focus->bb != nullptr ) {
    focus->onInput( input );
  }
}


/*
 * update process:
 * ===============
 *
 *  (how to store/register all motion planners?)
 *  (do we need an active/inactive thing again like in jb?)
 *  (not all motion planners are collidable.)
 *  (okay, so i'm moving toward jb-style composition.)
 *  (after all, it would be good to have snow without processing every
 *    snowflake for collision when nobody cares about it.)
 *  (and if we want z-layers in addition to that, we need a collision check
 *    for each z-layer.)
 *  (all this seems to necessitate talking to a parent, which Entity current-
 *    ly has no concept of.)
 *  (because i need to tell the parent i'm planning to move, or that i'm curr-
 *    ently changing to layer 5, or that I'm collidable or no longer collid-
 *    able.)
 *  (all this while preserving an MVC architecture.)
 *  (what if i followed the same mailbox structure for these systems?)
 *    Systems i'd need:
 *      rendering
 *      animation
 *      motion
 *      collision
 *  
 *    ... just like jollybean.
 *  Once I do that, I can have a singleton ecosytem of systems like in jb.
 *  Writing to specific, or even all, mailboxes would be trivial at that point with mutex locks.
 *
 */
