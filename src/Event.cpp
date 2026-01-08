#include "Event.h"
#include <cassert>
#include <unordered_map>
#include <functional>

std::unordered_map<int, std::function<void(int)>> map{};

// YAML can handle primitive operands. Fields in data structures need to be added manually.
Event::Event( YAML::Node& rootCfg ) {

}

auto Event::run() -> EventState {
  return _func();
}

auto Event::getState() const -> EventState {
  return _state;
}

void Event::setState( const EventState state ) {
  _state =  state;
}

void Event::reset() {
  _state =  EventState::READY;
}


void Event::addCondition( const Operand operand1, const int operand2, const ConditionOp& op ) {
  _conditions.emplace_back( operand1, operand2, op );
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
