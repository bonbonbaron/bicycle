#pragma once
#include <unordered_map>
#include "c/InputData.h"
#include "c/Timeout.h"
#include "c/Collision.h"
#include "m/Personality.h"
#include "c/WindowManager.h"
#include "m/Position.h"

 /* "I am the way... Nobody comes to the Father except through me." -John 14:6 
  *
  * Trigger is the gateway through which every signal must pass in the game engine.
  * This is designed to correlate tasks with each other.
  */

class Trigger {
  public:
    static auto getInstance() -> Trigger&;

    // Le trifecta... These wrap templatized calls to onTrigger() below.
    static void onInput( const InputState& input );  // straightforward feeding to whatever holds context
    static void onTimer( const Timeout& timeout );  
    static void onCollision( const Collision& collision );  // TODO

    // General trigger of input, timer, and collision actions
    template<typename T>
      static void onTrigger( const Entity entity, const T& t ) {
        auto& trigger = getInstance();
        auto  personality = trigger.getPersonality( entity );
        auto  quirk = personality[ getTypeTag<T>() ];
        auto& triggeredAction = std::get<Cb<T>>( quirk );
        triggeredAction( entity, t );
      } 

    auto getPersonality( Entity entity ) -> Personality;
    void setPersonality( Entity entity, const Personality& personality );

  private:
    Trigger() = default;
    Trigger(const Trigger&) = delete;
    Trigger operator=(const Trigger&) = delete;
    Trigger(const Trigger&&) = delete;
    Trigger operator=(const Trigger&&) = delete;

    struct Activity {
      Priority priority{};
      TimerId animTimerId{};
      TimerId cbTimerId{};
    };

    std::array<Personality, NUM_SUPPORTED_ENTITIES> _personalities{};
    std::array<Activity, NUM_SUPPORTED_ENTITIES>    _activities{};

};  // class Trigger
