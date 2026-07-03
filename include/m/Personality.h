#pragma once
#include <cursesw.h>
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <variant>

#include "bicycle.h"
#include "Config.h"
#include "Constants.h"
#include "m/Blackboard.h"
#include "c/Timer.h"
#include "c/InputData.h"
#include "m/TypeTag.h"

// TODO resolve these to real datatypes
using CollisionData = int;
using TimerData = double;

#define I(_iname_) void _iname_ ( Entity entity, const InputState& input )
#define T(_tname_) void _tname_ ( Entity entity, const TimerData& timerData )
#define C(_cname_) void _cname_ ( Entity entity, const CollisionData& collData )

// Function template
template<typename T>
// using Cb = std::function<void(Blackboard&, const T&)>;  // callback that returns a priority
using Cb = std::function<void(Entity, const T&)>;  // callback that returns a priority
using Quirk = std::variant< Cb<const InputState>, Cb<const CollisionData>, Cb<const TimerData> >;
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
