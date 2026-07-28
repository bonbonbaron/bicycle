#include "c/Animator.h"
#include "m/World.h"

auto Animator::getInstance() -> Animator& {
  static Animator kin{};
  return kin;
}

// TODO implement fixed point math here
void Animator::tick( const Entity entity ) {
  // Get state
  auto& state = _animStates.at(entity);

  if (state.frameIdx == state.currStrip->nFrames - 1) {
    // If this is a repeating animation strip, reset index, time left, and srcRectP.
    if (state.currStrip->pingPong) {
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
    if (state.currStrip->pingPong) {
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
        state.frameIdx += state.increment
      }
    }
    else {
      state.frameIdx += state.increment
    }
  }
  else {
    state.frameIdx += state.increment
  }
  // Advance the animation frame in whatever direction we're going.
  // TODO populate world rects
  *cP->srcRectP = frameA.at(state.frameIdx).rect;
  cP->dstRectP->w = cP->srcRectP->w;
  cP->dstRectP->h = cP->srcRectP->h;
}  // tick()

void Animator::config( const Entity entity, const MotionConfig& src ) {
  auto& dst = _cfgs.at(entity);
  dst.initVel = src.initVel;
  dst.termVel = src.termVel;
  dst.initAcc = src.initAcc;
  dst.type = src.type;
  dst.tgt = src.tgt;
}

void Animator::start( const Entity entity ) {
  const auto& cfg = _cfgs.at(entity);
  _vels.at(entity) = cfg.initVel;
  _terminalVels.at(entity) = cfg.termVel;
  _accs.at(entity) = cfg.initAcc;
}

void Animator::stop( const Entity entity ) {
  _vels.at(entity) = Box();
  _accs.at(entity) = Box();
}

void Animator::pause( const Entity entity ) {
  stop(entity);
}

void Animator::unpause( const Entity entity ) {
  start(entity);
}

void Animator::track( Entity follower, Entity tgt ) {
  _cfgs.at(follower).tgt = tgt;
}



