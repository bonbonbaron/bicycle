#pragma once
// The true-3D layer (World3D in c/Physics3D.h, Kinematics' 3D motions) as the rest of the engine sees it.
// 3D builds link src/c/Layer3D.cpp; ASCII and 2D builds link src/c/Layer3DOff.cpp instead, where these do nothing,
// so none of the 3D physics, camera rig or their tables are in those executables (see CMakeLists.txt).
#include "m/Entity.h"

namespace layer3d {
void step( float dt );            // once a frame after 2D motion: 3D physics, then the camera rig and view riders
void removeEntity( Entity e );    // a despawned entity: its transform, body and overlaps
}
