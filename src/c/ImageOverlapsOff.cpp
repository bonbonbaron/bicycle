// 3D builds: entities are drawn with a depth buffer, so there are no image overlap groups (include/c/ImageOverlaps.h).
#include "c/ImageOverlaps.h"

namespace imageOverlaps {
void drawnOn( Entity, Entity ) {}
void moving( Entity, bool ) {}
void forget( Entity ) {}
void update() {}
}  // namespace imageOverlaps
