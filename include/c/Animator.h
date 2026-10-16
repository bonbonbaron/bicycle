#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "m/Entity.h"
#include "m/Rect.h"
#include "c/Collision.h"
#include "c/Controllable.h"
#include "c/Timer.h"

class Animator : public Controllable {
  public:
    static auto getInstance() -> Animator&;
    void tick( const Entity entity );
    auto set( const Entity entity, const std::string& stripName ) -> bool;
    void start( const Entity entity ) override;
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const Entity entity ) override;
    void newStrip( const Entity entity, const std::string& stripName, const bool repeat );
    void addFrame( const Entity entity, const std::string& stripName, const unsigned durationMs, const Rect& srcRect );
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

    struct Animation : std::unordered_map<std::string, std::shared_ptr<AnimStrip>> {
      Animation() = default;
    };

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

