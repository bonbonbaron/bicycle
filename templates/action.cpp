#include "<func-name>.h"

/* An action is the basic building block of an entity's personality.
 * It must return an ActionState enum and consume an ActArg containing the entity's blackboard.
 */

using enum ActionState; 

ActionState <func-name>( ActArg& arg ) {
  return COMPLETE;  // { READY, FAILED, IN_PROGRESS, COMPLETE };
}

