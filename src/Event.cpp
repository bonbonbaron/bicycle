#include "Event.h"
#include <cassert>

auto Event::getState() const -> EventState {
  return _state;
}

auto Event::Condition::isMet() const -> bool {
  assert( operand1 != nullptr );
  switch ( op ) {
    case Event::ConditionOp::GT:
      return std::get<int>(*operand1) > operand2;
    case Event::ConditionOp::GTE:
      return std::get<int>(*operand1) >= operand2;
    case Event::ConditionOp::LT:
      return std::get<int>(*operand1) < operand2;
    case Event::ConditionOp::LTE:
      return std::get<int>(*operand1) <= operand2;
    case Event::ConditionOp::EQ:
      return std::get<int>(*operand1) == operand2;
    case Event::ConditionOp::NE:
      return std::get<int>(*operand1) != operand2;
    case Event::ConditionOp::IS_SET:
      return std::get<Bitset>(*operand1)[static_cast<size_t>(operand2)];
    case Event::ConditionOp::IS_UNSET:
      return ! (std::get<Bitset>(*operand1)[static_cast<size_t>(operand2)]);
    case Event::ConditionOp::IS_ANY_SET:
      return std::get<Bitset>(*operand1).any();
    case Event::ConditionOp::IS_ALL_SET:
      return std::get<Bitset>(*operand1).all();
    case Event::ConditionOp::IS_NONE_SET:
      return std::get<Bitset>(*operand1).none();
  }
  return false;
}

auto Event::meetsAnyCondition() const -> bool {
  for ( const auto& c : _conditions ) {
    if ( c.isMet() ) {
      return true;
    }
  }
  return false;
}

auto Event::meetsAllConditions() const -> bool {
  for ( const auto& c : _conditions ) {
    if ( ! c.isMet() ) {
      return false;
    }
  }
  return true;
}
