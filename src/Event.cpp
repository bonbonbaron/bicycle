#include "Event.h"

auto Event::getState() const -> EventState {
  return _state;
}

auto Condition::isMet() const -> bool {
    switch ( c.op ) {
      case GT:
        return c.operand1 > c.operand2;
      case GTE:
        return c.operand1 >= c.operand;
      case LT:
        return c.operand1 < c.operand2;
      case LTE:
        return c.operand1 <= c.operand2;
      case EQ:
        return c.operand1 == c.operand2;
      case NE:
        return c.operand1 != c.operand2;
      case IS_SET:
        return c.operand1 & c.operand2;
      case IS_UNSET:
        return ! (c.operand1 & c.operand2);
      default: 
        return false;
    }
}

auto Event::meetsAnyCondition() const -> bool {
  for ( const c& : _conditions ) {
    if ( c.isMet() ) {
      return true;
    }
  }
  return false;
}

auto Event::meetsAllConditions() const -> bool {
  for ( const c& : _conditions ) {
    if ( ! c.isMet() ) {
      return false;
    }
  }
  return true;
}
