#pragma once
#include "bicycle.h"
#include <dlfcn.h>

void config( const std::string& gameName );

template< typename T > 
auto extGrab( void* handle, const std::string& symbol ) -> T* {
  if ( handle == nullptr ) {
    bicycle::die( "dlopen() gave us a nullptr. Did we actually dlopen the shared library before calling this?" );
  }

  T* ptr = reinterpret_cast<T*>( dlsym( handle, symbol.c_str() ) );
  if ( ptr == nullptr ) {
    dlclose( handle );
    bicycle::die( "Couldn't find symbol '" + symbol + "'. Tried dlsym( handle, \"" + symbol.c_str() + "\"" );
  }

  return ptr;
}
