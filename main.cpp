#include <lua.hpp>
#include <iostream>

// TODO prove we can grab an arbitrary number of arbitrarily typed args from the lua stack.
// Reason is to see if we can start systems with diff nums/types of args.

int main() {
    lua_State* L = luaL_newstate();

    if (luaL_dofile(L, "i.lua") != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }

    // ohhh... maybe you have to call it directly.
    lua_getglobal( L, "a" );
    lua_pushinteger( L, 1 );
    lua_pushinteger( L, 2 );
    lua_pushinteger( L, 5 );
    lua_call( L, 3, 5 );
    // pull off stack
    auto i = lua_tointeger( L, -1 );
    auto j = lua_tointeger( L, -2 );
    auto k = lua_tointeger( L, -3 );

    std::cout << "i: " << i << "; ";
    std::cout << "j: " << j << "; ";
    std::cout << "k: " << k << '\n';

    return 0;
}
