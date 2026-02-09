#include <dlfcn.h>
#include <string>
#include <string_view>
#include <iostream>

#include "Config.h"
#include "Constants.h"
#include "bicycle.h"
#include "Personality.h"

static void registerPortTypes( void* handle, const std::string& GAME_FP ) {
  PortTypeRegistry::value_type* ports = extGrab<PortTypeRegistry::value_type>( handle, "ports" );
  int* numPorts = extGrab<int>( handle, "numPorts" );

  for (int i = 0; i < *numPorts; ++i ) {
    PortTypeRegistry::add( ports[i] );
  }
}

static void registerActions( void* handle, const std::string& GAME_FP ) {
  ActPkg* actions = extGrab<ActPkg>( handle, "actions" );
  const int* numActions = extGrab<int>( handle, "numActions" );

  // Allow devs some room for ignorance of macros unfortunate inclusion of undesirable characters.
  const std::string ILLEGAL_CHARS{ "\", " };

  for ( int i = 0 ; i < *numActions; ++i ) { 
    std::vector<BbKey> dstPorts;
    std::string& srcPorts = actions[i].ports;
    // Kind of like null-terminated strings, but it's an empty-string-terminated array.
    size_t start = srcPorts.find_first_not_of( ILLEGAL_CHARS );
    if ( start != std::string::npos ) {
      size_t end   = srcPorts.find_first_of( ILLEGAL_CHARS, start );
      for ( ;; ) {
        if ( end == std::string::npos ) {
          dstPorts.emplace_back( srcPorts, start );
          break;
        }
        assert( end > start );
        dstPorts.emplace_back( srcPorts, start, end - start );
        if ( start == std::string::npos ) {
          break;
        }
        start = srcPorts.find_first_not_of( ILLEGAL_CHARS, end );
        if ( start == std::string::npos ) {
          break;
        }
        end   = srcPorts.find_first_of( ILLEGAL_CHARS, start );
      } 
    }
    auto action = std::make_shared<Action>( actions[i].func, dstPorts );
    std::pair< std::string, ActionPtr > pair{ actions[i].name, action };
    ActionRegistry::add( pair );
  }
}

static void registerBlackboards() {
}

void config( const std::string& gameName ) {
  // TODO make sure this'll work
  constexpr std::string_view GAME_DIR{ "/usr/local/games/" };
  const std::string GAME_FP = GAME_DIR.data() + gameName + ".so";
  void* handle = dlopen( GAME_FP.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE );
  if ( handle == nullptr ) {
    bicycle::die( "Couldn't find shared library " + GAME_FP + '\n' );
  }

  registerPortTypes( handle, GAME_FP );
  registerActions( handle, GAME_FP );
  registerBlackboards(); // TODO #4:  get array of blackboards

  dlclose( handle );
}

