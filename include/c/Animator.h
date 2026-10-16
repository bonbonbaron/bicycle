#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <cstdint>
#include "data/Map.h"

#include "m/Entity.h"
#include "m/Rect.h"
#include "c/Collision.h"
#include "c/Controllable.h"
#include "c/Timer.h"

class Animator : public Controllable {
  public:
    static auto getInstance() -> Animator&;
    void tick( const Entity entity );
    auto set( const Entity entity, const uint8_t stripId ) -> bool;  // ids::Anim (gen/ids.h)
    void start( const Entity entity ) override;
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const Entity entity ) override;
    void newStrip( const Entity entity, const uint8_t stripId, const bool repeat );
    void addFrame( const Entity entity, const uint8_t stripId, const unsigned durationMs, const Rect& srcRect );
    auto getFrameIdx( const Entity entity ) const -> unsigned;
    void forget( const Entity entity );  // drops a despawned entity's strips (its timers stop with Timer::stopEntity)

  private:
    Animator() = default;
    Animator(const Animator&) = delete;
    Animator operator=(const Animator&) = delete;
    Animator(const Animator&&) = delete;
    Animator operator=(const Animator&&) = delete;

    struct AnimFrame {
      unsigned durationMs{};   // total duration -- don't decrement here, tell timer to
      Rect srcRect{};          // source portion of the entity's image (the whole strip), in character cells
      CollBox collisionBox{};  // collision boxes may change from frame to frame
    };

    struct AnimStrip {
      unsigned nFrames{};
      bool pingpong{};
      bool repeat{};
      std::vector<AnimFrame> frames{};
    };

    // An entity's strips by animation tag id (ids::Anim): a popcount-indexed map, no hashing or strings.
    using Animation = bicycle::map<std::shared_ptr<AnimStrip>>;

    struct AnimState {
      Animation animation{};
      unsigned frameIdx{};
      int increment{1};
      TimerId timerId{};
      bool running{};
      std::shared_ptr<AnimStrip> currStrip{};
    };

    void startTimer( const Entity entity, AnimState& state );
    void showFrame( const Entity entity, AnimState& state );

    // TODO make matching entities map to the same animation set
    std::array<AnimState, NUM_SUPPORTED_ENTITIES> _animStates{};

};

