#pragma once 
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

    struct FieldInfo {
      const char* name;
      size_t offset;
      size_t size;
    };

    template<typename T>
      struct TypeInfo {
        static constexpr FieldInfo fields[0] = {};   // Use raw array instead of std::array for N=0
        static constexpr size_t field_count = 0;
      };

#define BEGIN_REFLECT(T) \
    template<> \
    struct TypeInfo<T> { \
      static constexpr FieldInfo fields[] = {

#define REFLECT_FIELD(T, member) \
        { #member, offsetof(T, member), sizeof(decltype(T::member)) },

#define END_REFLECT() \
      }; \
      static constexpr size_t field_count = std::size(fields); \
    };

    template<typename T>
      void pushStruct(const T& obj) {
        lua_newtable(_luaState);

        const auto& fields = TypeInfo<T>::fields;
        for (const auto& f : fields) {
          const void* ptr = reinterpret_cast<const char*>(&obj) + f.offset;

          if (f.size == sizeof(int)) {
            lua_pushinteger(_luaState, *static_cast<const int*>(ptr));
          } else if (f.size == sizeof(float)) {
            lua_pushnumber(_luaState, *static_cast<const float*>(ptr));
          } else {
            // Add more types as needed
            lua_pushnil(_luaState);
          }

          lua_setfield(_luaState, -2, f.name);
        }
      }

#define addFunction( f ) \
    { \
      auto& trig = Trigger::getInstance(); \
      auto& bridge = trig.getBridge(); \
      bridge._addFunction( #f, &f ); \
    }

    void _addFunction( const char* name, lua_CFunction f );

    auto getState() -> lua_State*;

    // TODO oninput, oncollision, ontimer
  private:
    lua_State* _luaState{};
};
