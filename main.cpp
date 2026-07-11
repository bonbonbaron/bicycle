#include <lua.hpp>
#include <iostream>

// Example functions to expose
static int cpp_add(lua_State* L) {
    int a = luaL_checkinteger(L, 1);
    int b = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a + b);
    return 1;  // Number of return values
}

static int cpp_hello(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    std::cout << "Hello, " << name << " from C++!\n";
    return 0;
}

int main() {
    lua_State* L = luaL_newstate();

    lua_register(L, "add", cpp_add);
    lua_register(L, "hello", cpp_hello);

    if (luaL_dofile(L, "i.lua") != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }

    return 0;
}
