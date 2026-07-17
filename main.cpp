#include <lua.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <array>
#include <vector>

using namespace std;

typedef struct {
  int i;
  double k;
  char awer;
} ABC;
std::array<ABC,10> abcs_internal{};
template<typename T>
struct Arr {
  T* arr;
  unsigned nElems;
};

extern "C" {
  char s[] = "i love big ol' beans, don't you Billeh?";
  __attribute__((visibility("default"))) 
  ABC a {
    45,
      48.9,
      'p'
  };
  Arr<ABC> abcs = {
    abcs_internal.data(),
    abcs_internal.size()
  };
}

int main() {
  auto L = luaL_newstate();
  abcs_internal.at(8).i = 45;
  abcs_internal.at(8).k = 235.89;
  abcs_internal.at(8).awer = 'p';
  luaL_openlibs(L);
  if ( luaL_dofile( L, "./q.lua" ) != LUA_OK ) {
    auto errStr = std::string(lua_tostring( L, -1) );
    std::cerr << errStr << '\n';
    lua_pop(L, 1);
    return 1;
  }
  return 0;
}
