#include "Grid.h"
#include <algorithm>

Grid::Grid( const std::string& gridName ) {
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
      _bg = str;
    }

    // Get background dimensions before pruning newlines.
    // TODO does line.length() include new lines?
    bgFile.seekg(0);
    std::string line;
    while ( std::getline( bgFile, line ) ) {
      ++_bgDims.h;
      if ( line.length() > _bgDims.w ) {
        _bgDims.w = line.length();
      }
    }

    // Prune newlines out. They'd mess up drawing.
    auto idx = _bg.find_first_of( '\n' );
    while ( idx != std::string::npos ) {
      _bg.erase( idx, 1 );
      idx = _bg.find_first_of( '\n' );
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
      try {
        entity = entityNode.as<Entity>();
      } 
      catch ( const std::runtime_error& e ) {
        std::cerr << "Error for entity \'" << entityName << "\':\n";
        bicycle::die( e.what() );
      }
      if ( ! e.second.IsSequence() ) {
        bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": pos node needs to be a sequence.\n" );
      }
      // Sequence of 2 integers = one instance's position
      if ( e.second[0].IsScalar() && e.second.size() == 2 ) {
        auto position = e.second.as<Position>();
        entity.body.setPosition( position );
        // Copy the entity into a shared pointer object
        auto entityPtr = std::make_shared<Entity>( entity );
        addEntity( entityName, entityPtr );
      }
      // Sequence of at least 1 position
      else if ( e.second.IsSequence() ) {
        try {
          auto positions = e.second.as<std::vector<Position>>();
          for ( auto& pos : positions ) {
            entity.body.setPosition( pos );
            // Copy the entity into a shared pointer object
            auto entityPtr = std::make_shared<Entity>( entity );
            addEntity( entityName, entityPtr );
          }
        }
        catch ( const YAML::Exception& e ) {
          bicycle::die( "In grid " + gridName + ": for entity " + entityName + ": pos error: " + e.what() );
        }
      }
    }  // for each entity in grid's entites list
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
  while ( _fg.contains( attemptedName ) ) {
    attemptedName = name + " " + std::to_string( ++attempt );
  }
  _fg[ attemptedName ] = entity;
}


Grid::Grid( const int x, const int y, const int w, const int h ) : Window(x, y, w, h) {}

void Grid::update() {
  render();
}

void Grid::focusOn( const std::string& entityName ) {
  if ( _fg.contains( entityName ) ) {
    _focus = _fg.at( entityName );
  }
}

// TODO this assumes single-character symbols. Handle multi-char later.
auto Grid::isOnscreen( const std::shared_ptr<Entity>& entity ) -> bool {
  auto pos = entity->body.getPosition();
  return 
    pos.x >= _camera.x && 
    pos.x < _camera.x + getWidth() - WINDOW_PADDING &&
    pos.y >= _camera.y && 
    pos.y < _camera.y + getHeight() - WINDOW_PADDING;
}

void Grid::render() {
  // optimize this later if it's too slow
  // Background
  for ( size_t row = 0; row < getHeight() - WINDOW_PADDING; ++row ) {
    auto rowStr = std::string( 
        _bg, 
        ( ( _camera.y + row ) * _bgDims.w + _camera.x ), 
        static_cast<size_t>(getWidth() - WINDOW_PADDING ));
    mvprint( static_cast<int>(row) + 1, 1, rowStr );
  }

  // Foreground
  for ( const auto& [ name, entity ] : _fg ) {
    if ( isOnscreen( entity ) ) {
      auto pos = entity->body.getPosition();
      auto x = pos.x - _camera.x;
      auto y = pos.y - _camera.y;
      mvprint( y, x, entity->body.getSymbol() );
    }
  }
}

void Grid::pan( const int dx, const int dy ) {
  _camera.x += dx;
  _camera.y += dy;
  _camera.x = std::clamp<int>( _camera.x, 0, _bgDims.w - getWidth() + WINDOW_PADDING );
  _camera.y = std::clamp<int>( _camera.y, 0, _bgDims.h - getHeight() + WINDOW_PADDING );
}

void Grid::react( const int input ) {
  // TODO delete first if statement
  switch( input ) {
    case 'h':
      pan( -1, 0);
      break;
    case 'l':
      pan( 1, 0);
      break;
    case 'j':
      pan( 0, 1);
      break;
    case 'k':
      pan( 0, -1);
      break;
    default:
      break;
  }
  if ( _focus != nullptr ) {
    _focus->onInput( input );
  }
}


/* 
 * How do I make a Grid?
 * It needs:
 *    bg ( raw txt file )
 *    fg ( inserted from node's entities list )
 * How does Node know? Why does an entity's body have position in the first place,
 * 
 * And how do I easily develop fg? I don't want to have to query the position every t
 * time i want to place somebody/something down. It would be better to have "layers"
 * in a text file so i can control it.
 *
 * I can have a special prefix to indicate I'm placing a fg object down, like @.
 *
 * I can't make a YAML decoder for Grid since it default-constructs it. 
 *
 * Oh yes I can. 
 * In what context would I use the grid keyword?
 */
