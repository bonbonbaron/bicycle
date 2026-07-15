#include <lua.hpp>
#include "Bridge.h"
#include "m/World.h"

// These functions provide the bridge between Lua scripts and Bicycle's back-end.

Bridge::Bridge() : luaState( luaL_newstate() ) {}
