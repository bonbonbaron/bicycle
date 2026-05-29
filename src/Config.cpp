#include <dlfcn.h>
#include <string>
#include <string_view>
#include <iostream>

#include "Config.h"
#include "Constants.h"
#include "bicycle.h"
#include "m/Personality.h"

/* These static functions act like the video game console.
 * The "game cartridge," or shared object file, gets dynamically
 * linked in at runtime. That linkage entails the following:
 *
 *  1) blackboards
 *  2) actions (i had impl'd that before and then deleted it; need to figure 
 *              out why and where I was taking that idea instead)
 */

static void registerBlackboards( void* handle ) {
  auto bbs = extGrab<std::shared_ptr<std::map<std::string, Blackboard>>>( handle, "blackboards" );
  if ( !(*bbs) || (*bbs)->size() == 0 ) {
    bicycle::die( "Game has no blackboards." );
  }
  for ( const auto& pair : **bbs ) {
    BlackboardRegistry::add( pair );
  }
}

// We have to jump through some hoops to smuggle goods across the dynamically loaded library boundary.
void config( const std::string& gameName ) {
  constexpr std::string_view GAME_DIR{ "/usr/local/games/" };
  const std::string GAME_FP = GAME_DIR.data() + gameName + ".so";
  void* handle = dlopen( GAME_FP.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE );
  if ( handle == nullptr ) {
    bicycle::die( "Couldn't find shared library " + GAME_FP + '\n' );
  }

  // TODO: registerActions( handle );
  registerBlackboards( handle ); 

  dlclose( handle );
}

