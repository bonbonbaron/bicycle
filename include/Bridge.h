#include <lua.hpp>

class Bridge {
  public:
    Bridge();
  private:
    lua_State* luaState{};
};
