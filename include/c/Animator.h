#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "m/Animation.h"
#include "m/Entity.h"
#include "m/Rect.h"
#include "c/Collision.h"
#include "c/Controllable.h"

class Animator : public Controllable {
  public:
    static auto getInstance() -> Animator&;
    void tick( const Entity entity );
    void set( const Entity entity, const std::string& stripName );
    void start( const Entity entity ) override;
    void stop( const Entity entity ) override;
    void pause( const Entity entity ) override;
    void unpause( const Entity entity ) override;

  private:
    Animator() = default;
    Animator(const Animator&) = delete;
    Animator operator=(const Animator&) = delete;
    Animator(const Animator&&) = delete;
    Animator operator=(const Animator&&) = delete;

    struct AnimFrame {
      unsigned duration{};     // total duration -- don't decrement here, tell timer to
      Rect srcRect{};          // source portion of origin iamge for destination image
      CollBox collisionBox{};  // collision boxes may change from frame to frame
    };

    struct AnimStrip {
      unsigned nFrames{};
      bool pingpong{};
      bool repeat{};
      AnimType type{};
      std::vector<AnimFrame> frames{};
    };

    struct Animation : std::unordered_map<std::string, std::shared_ptr<AnimStrip>> {
      Animation() = default;
    };

    struct AnimState {
      Animation animation{};
      unsigned frameIdx{};
      int increment{1};
      std::shared_ptr<AnimStrip> currStrip{};
    };

    // TODO make matching entities map to the same animation set
    std::array<AnimState, NUM_SUPPORTED_ENTITIES> _animStates{};
    std::array<Animation, NUM_SUPPORTED_ENTITIES> _animations{};

};

