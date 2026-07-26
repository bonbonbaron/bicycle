#include "c/Kinematics.h"

auto Kinematics::getInstance() -> Kinematics& {
  static Kinematics kin{};
  return kin;
}

void Kinematics::run() {

}

void Kinematics::create() {

}

void Kinematics::start( const Entity entity ) {

}

void Kinematics::stop( const Entity entity ) {

}

void Kinematics::pause( const Entity entity ) {

}

void Kinematics::unpause( const Entity entity ) {

}

void Kinematics::track( Entity follower, Entity tgt ) {

}

