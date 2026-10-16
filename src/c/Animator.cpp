#include <cassert>
#include "c/Animator.h"
#include "m/World.h"
#include "c/Timeout.h"

auto Animator::getInstance() -> Animator& {
  static Animator kin{};
  return kin;
}

// Called when the current frame's timer runs out: advance to the next frame (looping if the strip repeats).
// Aseprite directions (reverse, ping-pong) are expanded into plain frame order before they get here.
void Animator::tick( const Entity entity ) {
  auto& state = _animStates.at( entity );
  if ( !state.running || state.currStrip == nullptr ) {
    return;
  }
  auto& timer = Timer::getInstance();
  timer.stop( state.timerId );  // one-shot timer: free it before making the next one
  if ( state.frameIdx + 1 < state.currStrip->frames.size() ) {
    ++state.frameIdx;
  }
  else if ( state.currStrip->repeat ) {
    state.frameIdx = 0;
  }
  else {
    state.running = false;  // non-repeating strip holds its last frame
    return;
  }
  showFrame( entity, state );
  startTimer( entity, state );
}  // tick()

// Point the entity's src rect at the current frame; Camera draws only that part of the image.
void Animator::showFrame( const Entity entity, AnimState& state ) {
  const auto& currFrame = state.currStrip->frames.at( state.frameIdx );
  World::set<Rect>( entity, currFrame.srcRect );
  // Box is in fixed point; assume we don't want to change its depth.
  auto& box = World::get<Box>( entity );
  box.size.w = currFrame.srcRect.w << FIXEDPT_DEC_BITS;
  box.size.h = currFrame.srcRect.h << FIXEDPT_DEC_BITS;
  if ( currFrame.collisionBox.size.w > 0 ) {
    World::set<CollBox>( entity, currFrame.collisionBox );
  }
}

void Animator::startTimer( const Entity entity, AnimState& state ) {
  // Set timer for this frame's duration.
  auto& timer = Timer::getInstance();
  state.timerId = timer.create( 
    state.currStrip->frames.at( state.frameIdx ).durationMs, 
    entity, 
    0,      // timeout type doesn't matter here, we only have one use for timer in animation
    false,  // don't repeat; each frame has its own duration
    TimeoutAddr::ANIMATION
  );
}

// Switches to (and starts) a strip. Returns false if it's already playing or doesn't exist.
auto Animator::set( const Entity entity, const std::string& stripName ) -> bool {
  auto& state = _animStates.at( entity );
  auto strip = state.animation.find( stripName );
  if ( strip == state.animation.end() || strip->second->frames.empty() ) {
    return false;  // TODO make some kind of status bar at the bottom to tell user strip isn't found.
  }
  if ( strip->second == state.currStrip && state.running ) {
    return false;
  }
  stop( entity );
  state.currStrip = strip->second;
  start( entity );
  return true;
}

void Animator::start( const Entity entity ) {
  auto& state = _animStates.at( entity );
  assert( state.currStrip != nullptr );
  state.frameIdx = 0;
  state.increment = 1;
  state.running = true;
  showFrame( entity, state );
  startTimer( entity, state );
}

void Animator::stop( const Entity entity ) {
  auto& state = _animStates.at( entity );
  if ( state.running ) {
    auto& timer = Timer::getInstance();
    timer.stop( state.timerId );
    state.running = false;
  }
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

// Strips are built from Lua (see Genome.lua's sprite loader) via newAnimStrip() and addAnimFrame().
void Animator::newStrip( const Entity entity, const std::string& stripName, const bool repeat ) {
  auto& state = _animStates.at( entity );
  auto strip = std::make_shared<AnimStrip>();
  strip->repeat = repeat;
  state.animation[ stripName ] = strip;
}

void Animator::addFrame( const Entity entity, const std::string& stripName, const unsigned durationMs, const Rect& srcRect ) {
  auto& state = _animStates.at( entity );
  auto strip = state.animation.find( stripName );
  assert( strip != state.animation.end() );
  strip->second->frames.push_back( AnimFrame{ durationMs, srcRect, {} } );
  strip->second->nFrames = strip->second->frames.size();
}

auto Animator::getFrameIdx( const Entity entity ) const -> unsigned {
  return _animStates.at( entity ).frameIdx;
}

void Animator::forget( const Entity entity ) {
  if ( entity < _animStates.size() ) _animStates[entity] = AnimState{};
}
