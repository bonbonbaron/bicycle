#include "System.h"
#include <cursesw.h>

// I like Grok's idea of storing everything in global arrays.
// Archetypes are also a cool concept. Some systems ought to be allowed to process multiple thigns
// together in order to avoid awkward messaging.
class RenderSystem : public System<Image> {
  public:
    RenderSystem();
    void run() override;
    void setColor( const unsigned entityId, const Color color );
};
