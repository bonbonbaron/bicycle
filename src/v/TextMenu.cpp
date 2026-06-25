#include "v/TextMenu.h"
#include "c/WindowManager.h"
#include <cassert>
#include <algorithm>

Menu::Menu( const std::string& menuName ) : Window(), _id( menuName ) {
  auto yamlFilename = MENU_DIR + menuName + SUFFIX.data();
  try {
    auto root = YAML::LoadFile( yamlFilename );

    // Create window
    auto w = root["w"].as<unsigned>();
    setWidth( w );
    auto h = root["h"].as<unsigned>();
    setHeight( h );
    if ( auto x = root["x"] ) {
      setX( x.as<int>() );
    }
    if ( auto y = root["y"] ) {
      setY( y.as<int>() );
    }
    create();

    // Menu Items
    auto entities = root["items"];
    if ( !entities.IsSequence() ) {
      bicycle::die( "Menu " + menuName + "'s items node needs to be a sequence." );
    }

    // For each entity, the key is the entity name, val is position.
    for ( const auto& e : entities ) {
      auto entityName = e.as<std::string>();
      YAML::Node entityNode;
      try {
        entityNode = YAML::LoadFile( ENTITY_DIR + entityName + SUFFIX.data() );
      }
      catch ( const YAML::BadFile& e ) {
        bicycle::die( "In menu " + menuName + ": for entity " + entityName + ": We couldn't find " + ENTITY_DIR + entityName + SUFFIX.data() + "." );
      }
      Entity entity;
      try {
        entity = entityNode.as<Entity>();
        auto entityPtr = std::make_shared<Entity>( entity );
        addItem( entityPtr );
      } 
      catch ( const std::runtime_error& e ) {
        bicycle::die( "Error for entity \'" + entityName + "\':\n" + e.what() );
      }
    }  // for each entity in menu's entites list
  }  // try-block
  catch ( const YAML::Exception& e ) {
    std::stringstream ss;
    ss << "Error reading from file " << yamlFilename << ":\n" << e.what();
    bicycle::die( ss.str() );
  }
}

void TextMenu::render() {
  const int LAST_DISP_IDX = std::min<int>( _firstDispIdx + getHeight() - WINDOW_PADDING, _items.size() );
  for ( int currRow = 1, dispIdx = _firstDispIdx; dispIdx < LAST_DISP_IDX; ++dispIdx ) {
    // CURSOR_WIDTH leaves room for cursor to the left of this menu item
    if ( dispIdx == _currMenuItemIdx ) {
      setAttr( A_STANDOUT );
    }
    mvprint( currRow++, CURSOR_WIDTH, _items.at( dispIdx )->body.getSymbol() );  
    unsetAttr( A_STANDOUT );
  }
  mvprint( _currMenuItemIdx - _firstDispIdx + WINDOW_PADDING / 2, WINDOW_PADDING / 2, CURSOR  );
}
