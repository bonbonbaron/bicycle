#include <dlfcn.h>
#include <string>
#include <string_view>
#include <iostream>

#include "Config.h"
#include "Constants.h"
#include "bicycle.h"

static void registerPortTypes() {
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
  
  registerPortTypes();
  registerActions();
  registerConditions();
  registerBlackboards();
}
