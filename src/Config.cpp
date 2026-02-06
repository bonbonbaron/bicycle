#include <dlfcn.h>
#include <string>
#include <string_view>
#include <iostream>

#include "Config.h"
#include "Constants.h"
#include "bicycle.h"
#include "Personality.h"

static void registerPortTypes( void* handle, const std::string& GAME_FP ) {
  PortTypeRegistry::value_type* ports = extGrab<PortTypeRegistry::value_type>( handle, "ports", " Couldn't find a ports array in " + GAME_FP + ".\nShould have a \"const PortTypeRegistry::value_type ports[]\" somewhere wrapped in \'extern \"C\" { ... }\'." );
  int* numPorts = extGrab<int>( handle, "numPorts", " Couldn't find numPorts in " + GAME_FP + ". Should have a \"const int numPorts{ sizeof( ports ) / sizeof( ports[0] ) };\" somewhere wrapped in \'extern \"C\" { ... }\'." );

  for (int i = 0; i < *numPorts; ++i ) {
    PortTypeRegistry::add( ports[i] );
  }
}

static void registerActions() {
}

static void registerConditions() {
}

static void registerBlackboards() {
}

void config( const std::string& gameName ) {
  // TODO make sure this'll work
  constexpr std::string_view GAME_DIR{ "/usr/local/games/" };
  const std::string GAME_FP = GAME_DIR.data() + gameName + ".so";
  void* handle = dlopen( GAME_FP.c_str(), RTLD_LAZY );
  if ( handle == nullptr ) {
    bicycle::die( "Couldn't find shared library " + GAME_FP + '\n' );
  }
  // TODO #2:  get array of actions
  // TODO #3:  get array of conditions
  // TODO #4:  get array of blackboards
  
  registerPortTypes( handle, GAME_FP );
  registerActions();
  registerConditions();
  registerBlackboards();
}

/* It'll be imPORTant to specify a type for each port.
 * But how will bicycle know about custom types?
 * Do those need to be registered from the SO? No, be-
 * cause they give type_indexes, not raw rypes. BOOM!
 */

/* So what does the port process look like?
 *  1. Store an array of std::pairs. Map 
 */
