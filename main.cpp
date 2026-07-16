#include <lua.hpp>
#include <iostream>
#include <string>
#include <array>
#include <vector>

using namespace std;
// =============================================
//  Improved Reflection System - Automatic Count
// =============================================

// =============================================
//  Fixed Reflection System with Automatic Count
// =============================================

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


// =============================================
//  Usage
// =============================================

struct Stats {
    int hp = 100;
    int mp = 50;
    int strength = 10;
    double speed = 5.5f;
};

BEGIN_REFLECT(Stats)
REFLECT_FIELD(Stats, hp)
REFLECT_FIELD(Stats, mp)
REFLECT_FIELD(Stats, strength)
REFLECT_FIELD(Stats, speed)
END_REFLECT();

// =============================================
//  Usage Example
// =============================================

template<typename T>
void addKeyVal( lua_State* L, const char* key, T val ) {
  lua_pushinteger( L, static_cast<T>( val ) );
  lua_setfield( L, -2, key );
}

template<typename T>
struct KeyVal {
  const char* key;
  T val;
};

template<typename T>
void addTable( lua_State* L, const char* tblName, const std::vector<KeyVal<T>> rows ) {
  lua_newtable(L);
  for ( const auto& row : rows ) {
    addKeyVal<int>( row.key, row.val );
  }
  lua_setglobal( L, tblName );
}

template<typename T>
void pushStruct(lua_State* L, const T& obj) {
    lua_newtable(L);

    const auto& fields = TypeInfo<T>::fields;
    for (const auto& f : fields) {
        const void* ptr = reinterpret_cast<const char*>(&obj) + f.offset;

        if (f.size == sizeof(int)) {
            lua_pushinteger(L, *static_cast<const int*>(ptr));
        } else if (f.size == sizeof(double)) {
            lua_pushnumber(L, *static_cast<const double*>(ptr));
        } else {
            // Add more types as needed
            lua_pushnil(L);
        }

        lua_setfield(L, -2, f.name);
    }
}

// TODO make world library registrations
int getStats( lua_State *L ) {
  auto int1 = luaL_checkinteger( L, 1 );
  cout << "int1 is " << int1 << '\n';
  Stats s{ 1, 2, 3, 4.8 };
  pushStruct( L, s );
  return 1;
}

int main( int argc, char** argv ) {
  lua_State* L = luaL_newstate();
  lua_register( L, "gets", &getStats );
  TypeInfo<Stats> ts{};
	lua_createtable(L, 0, ts.field_count);

	auto filename = string{argv[1]};
  luaopen_base(L);
	auto filepath = "./" + filename + ".lua";
	if (luaL_dofile(L, filepath.c_str()) != LUA_OK) {
    auto errStr = string( lua_tostring(L, -1) );
    cerr << errStr << '\n';
		lua_pop(L, 1);
	}
	return 0;
}
