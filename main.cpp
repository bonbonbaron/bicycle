#include <lua.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <array>
#include <vector>

using namespace std;

typedef struct {
  int i{};
  double k;
  char awer;
} ABC;

int main() {
  char s[] = "i love big ol' beans, don't you Billeh?";
  ABC a {
    45,
    48.9,
    'p'
  };
  auto L = luaL_newstate();
  luaL_openlibs(L);
  if ( luaL_dofile( L, "./q.lua" ) != LUA_OK ) {
    auto errStr = std::string(lua_tostring( L, -1) );
    std::cerr << errStr << '\n';
    lua_pop(L, 1);
    return 1;
  }
	return 0;
}
