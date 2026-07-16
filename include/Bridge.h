#pragma once 
#include <lua.hpp>
#include <vector>
#include <string>
#include <array>
#include <cstddef>
#include <type_traits>
#include "TypeInfo.h"

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
		// =============================================
		//  Recursive pushStruct
		// =============================================
		//template<typename T>
			//void pushStruct(lua_State* L, const T& obj);

		template<typename T>
			void pushStruct(lua_State* L, const T& obj)
			{
				if constexpr ( TypeInfo<T>::field_count > 0 ) {
					// This is a reflected struct → create a table
					lua_newtable(L);

					for (const auto& f : TypeInfo<T>::fields) {
						const void* ptr = reinterpret_cast<const char*>(&obj) + f.offset;
            f.push(L, ptr);                    // Call the correct push function
            lua_setfield(L, -2, f.name);
					}
				} else {
					lua_pushnil(L);
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
