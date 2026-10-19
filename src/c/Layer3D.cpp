// 3D builds: the engine's hooks into the true-3D layer (include/c/Layer3D.h).
#include "c/Layer3D.h"
#include "c/Kinematics.h"
#include "c/Physics3D.h"

namespace layer3d {
void step( float dt ) {
  phys3d::World3D::get().step( dt );
  Kinematics::getInstance().runCamera3D( dt );
  Kinematics::getInstance().runViews3D( dt );
}
void removeEntity( Entity e ) { phys3d::World3D::get().removeEntity( e ); }
}
