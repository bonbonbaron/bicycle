#pragma once
#include <cursesw.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <variant>

#include "bicycle.h"
#include "m/TypeTag.h"
#include "Config.h"
#include "Constants.h"
#include "m/Blackboard.h"
#include "c/Timer.h"
#include "c/InputData.h"
#include "c/Timeout.h"
#include "c/Collision.h"

#define I(_iname_) void _iname_ ( Entity entity, const InputState& input )
#define T(_tname_) void _tname_ ( Entity entity, const Timeout& timeout )
#define C(_cname_) void _cname_ ( Entity entity, const Collision& collData )

// Function template
template<typename T>
// using Cb = std::function<void(Blackboard&, const T&)>;  // callback that returns a priority
using Cb = std::function<void(Entity, const T&)>;  // callback that returns a priority
using Quirk = std::variant< Cb<const InputState>, Cb<const Collision>, Cb<const Timeout> >;
// TypeTag resolves to const void*.
// Example: personality.find( getTypeTag<InputState>() )
class Personality {
  public:
    template<typename T>
    void setQuirk( Cb<T> quirk ) {
      _quirks[ getTypeTag<T>() ] = quirk;
    }
    auto operator[]( const void* key ) -> Quirk {
      return _quirks.at( key );
    }
    Personality() = default;
  private:
    std::map< const void*, Quirk > _quirks;
};
