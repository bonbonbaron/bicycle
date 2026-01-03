#include "Event.h"
#include <cassert>

// YAML can handle primitive operands. Fields in data structures need to be added manually.
Event::Event( YAML::Node& ymlConditions ) {
  // The first operand of every condition is a smart pointer.
  // At construction time we look for the data that goes by a key.
  // If we find it, we point at that object and apply the second operand.
  // If we want to use structs, we need to extend this class and add specific getters 
  // for the fields of the structures we want.
}

auto Event::getState() const -> EventState {
  return _state;
}

void Event::addCondition( const Operand operand1, const int operand2, const ConditionOp& op ) {
  _conditions.emplace_back( operand1, operand2, op );
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
