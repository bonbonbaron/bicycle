// ASCII and 2D builds: no true-3D layer (include/c/Layer3D.h). Kinematics' 3D motions never start here (their Lua
// API, src/c/World3DApi.cpp, is 3D-only), so its 3D pass has nothing to do.
#include "c/Layer3D.h"
#include "c/Kinematics.h"

namespace layer3d {
void step( float ) {}
void removeEntity( Entity ) {}
}
void Kinematics::runDriven3D( Entity, Driven& ) {}
