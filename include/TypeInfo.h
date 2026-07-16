#pragma once 
#include <lua.hpp>

struct FieldInfo {
  const char* name;
  size_t offset;
  size_t size;
  void (*push)(lua_State*, const void*);
};

template<typename T>
struct TypeInfo {
  static constexpr FieldInfo fields[0] = {};   // Use raw array instead of std::array for N=0
  static constexpr size_t field_count = 0;
};


// =============================================
//  Macros (unchanged from before)
// =============================================
#define BEGIN_REFLECT(T) \
  template<> \
  struct TypeInfo<T> { \
    static constexpr FieldInfo fields[] = {

#define REFLECT_FIELD(T, member) \
      { #member, offsetof(T, member), sizeof(decltype(T::member)) },

#define END_REFLECT(T) \
    }; \
    static constexpr size_t field_count = std::size(fields); \
  };

