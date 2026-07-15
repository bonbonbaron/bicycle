#include <lua.hpp>
#include <vector>
#include <string>

class Bridge {
  public:
    Bridge();

    template<typename T>
    void addKeyVal( const char* key, T val ) {
      lua_pushinteger( _luaState, static_cast<T>( val ) );
      lua_setfield( _luaState, -2, key );
    }

    template<typename T>
    struct KeyVal {
      const char* key;
      T val;
    };

    template<typename T>
    void addTable( const char* tblName, const std::vector<KeyVal<T>> rows ) {
      lua_newtable(_luaState);
      for ( const auto& row : rows ) {
        addKeyVal<int>( row.key, row.val );
      }
      lua_setglobal( _luaState, tblName );
    }
    
    void addFunction( const char* name, lua_CFunction f );
    // TODO oninput, oncollision, ontimer

  private:
    lua_State* _luaState{};
};
