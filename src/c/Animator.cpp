#include <cassert>
#include "c/Animator.h"
#include "m/World.h"
#include "c/Timeout.h"

auto Animator::getInstance() -> Animator& {
  static Animator kin{};
  return kin;
}

// TODO you should probably dumb down the logic in here so it's not so hard to look at.
void Animator::tick( const Entity entity ) {
  // Get state
  auto& state = _animStates.at(entity);

  if (state.frameIdx == state.currStrip->nFrames - 1) {
    // If this is a repeating animation strip, reset index, time left, and srcRectP.
    if (state.currStrip->pingpong) {
      state.increment *= -1;
      state.frameIdx += state.increment;
    }
    else if (state.currStrip->repeat) {
      start( entity );
    } 
    // Deactivate if not repeating or reversing order for pingpong.
    else {
      stop( entity );
      return;
    }
  }
  // Otherwise, if this is a ping pong strip returning to the first frame, reverse directions.
  else if (state.frameIdx == 0) {
    if (state.currStrip->pingpong) {
      // If we just finished going backwards in a pingpong
      if (state.increment < 0) {
        // Start over if we're repeating
        state.increment = 1;
        if (state.currStrip->repeat) {
          state.frameIdx  = 1;
        }
        // Deactivate if not repeating pingpong.
        else {
          stop( entity );
          return;
        }
      }
      else {
        state.frameIdx += state.increment;
      }
    }
    else {
      state.frameIdx += state.increment;
    }
  }
  // If we're not at either extreme of the strip, increment normally.
  else {
    state.frameIdx += state.increment;
  }
  // Advance the animation frame in whatever direction we're going.
  // TODO populate world rects
  auto& currFrame = state.currStrip->frames.at(state.frameIdx);
  World::set<Rect>( entity, currFrame.srcRect );
  // Assume we don't want to change the depth or z-dimension of box to new rect w&h.
  auto& box = World::get<Box>( entity );
  box.size.w = currFrame.srcRect.w;
  box.size.h = currFrame.srcRect.h;
  World::set<CollBox>( entity, currFrame.collisionBox );
}  // tick()

void Animator::set( const Entity entity, const std::string& stripName ) {
  auto& state = _animStates.at( entity );
  auto strip = state.animation.find( stripName );
  if ( strip != state.animation.end() ) {
    state.currStrip = strip->second;
  }
  // TODO make some kind of status bar at the bottom to tell user strip isn't found.
}

void Animator::start( const Entity entity ) {
  auto& state = _animStates.at( entity );
  state.frameIdx = 0;
  state.increment = 1;
  assert( state.currStrip != nullptr );
  auto& timer = Timer::getInstance();
  state.timerId = timer.create( 
    state.currStrip->frames.at(0).durationMs, 
    entity, 
    0,      // timeout type doesn't matter here
    false,  // don't repeat
    TimeoutAddr::ANIMATION
  );
}

void Animator::stop( const Entity entity ) {
  auto& state = _animStates.at( entity );
  auto& timer = Timer::getInstance();
  timer.stop( state.timerId );
}

void Animator::pause( const Entity entity ) {
  auto& state = _animStates.at( entity );
  auto& timer = Timer::getInstance();
  timer.pause( state.timerId );
}

void Animator::unpause( const Entity entity ) {
  auto& state = _animStates.at( entity );
  auto& timer = Timer::getInstance();
  timer.unpause( state.timerId );
}

// TODO 
void Animator::newStrip( Entity follower, const std::string& stripName ) {
}


