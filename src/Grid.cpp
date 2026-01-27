#include "Grid.h"

constexpr std::string_view GRID_DIR{ "./config/grid/" };
constexpr std::string_view GRID_EXT{ ".yml" };
constexpr std::string_view BG_DIR{ "./config/grid/bg/" };

Grid::Grid( const std::string& gridName ) {
  auto yamlFilename = GRID_DIR.data() + gridName + GRID_EXT.data();
  try {
    auto root = YAML::LoadFile( yamlFilename );
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
    auto BG_FP = BG_DIR.data() + bgFilename;
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

    // Get background dimensions
    bgFile.seekg(0);
    std::string line;
    while ( std::getline( bgFile, line ) ) {
      ++_bgDims.h;
      if ( line.length() > _bgDims.w ) {
        _bgDims.w = line.length();
      }
    }
  }
  catch ( const YAML::Exception& e ) {
    std::stringstream ss;
    ss << "Error reading from file " << yamlFilename << ":\n" << e.what();
    bicycle::die( ss.str() );
  }
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

void Grid::render() {
  // optimize this later if it's too slow
  for ( size_t row = 0; row < getHeight(); ++row ) {
    move( row + 1, 1 );
    auto rowStr = std::string( 
        _bg, 
        ( ( _camera.y + row ) * _bgDims.w + _camera.x ), 
        static_cast<size_t>(getWidth() - WINDOW_PADDING ));
    print( rowStr );
  }
}

void Grid::react( const int input ) {
  // TODO delete first if statement
  switch( input ) {
    case 'h':
      --_camera.x;
      break;
    case 'l':
      ++_camera.x;
      break;
    case 'j':
      ++_camera.y;
      break;
    case 'k':
      --_camera.y;
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
