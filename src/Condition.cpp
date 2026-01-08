#include "Condition.h"
#include "GameState.h"   // Shall this be standardized as well?
#include <cursesw.h>

Condition::Condition( const YmlNode& c ) {
  auto operand1Key = c.readRequired( "operand1" );
  auto gs = GlobalState::getInstance();

  // Train of thought here is, I should be able to access any int or bitset in the whole game.
  // Even those nested beneath several layers of structures.
  // That's the potential beauty of the selectFrom() idea. I think this is where I go off and develop it.
  // For example, how do I get Cyan's equipped sword's strength stat?

  const std::string opString = c.readRequired<std::string>( "op" );
  std::string opUpStr;

  // Free users from tip-toeing around case sensitivity.
  for ( const auto c& : opString ) {
    opUpStr += toupper( c );
  }

  if ( opUpStr == "GT" ) {
    op = ConditionOp::GT;
  }
  else if ( opUpStr == "GTE" ) {
    op = ConditionOp::GTE;
  }
  else if ( opUpStr == "LT" ) {
    op = ConditionOp::LT;
  }
  else if ( opUpStr == "LTE" ) {
    op = ConditionOp::LTE;
  }
  else if ( opUpStr == "EQ" ) {
    op = ConditionOp::EQ;
  }
  else if ( opUpStr == "NE" ) {
    op = ConditionOp::NE;
  }
  else if ( opUpStr == "IS_SET" ) {
    op = ConditionOp::IS_SET;
  }
  else if ( opUpStr == "IS_UNSET" ) {
    op = ConditionOp::IS_UNSET;
  }
  else if ( opUpStr == "IS_ANY_SET" ) {
    op = ConditionOp::IS_ANY_SET;
  }
  else if ( opUpStr == "IS_ALL_SET" ) {
    op = ConditionOp::IS_ALL_SET;
  }
  else if ( opUpStr == "IS_NONE_SET" ) {
    op = ConditionOp::IS_NONE_SET;
  }
  else if ( opUpStr == "UNCONDITIONAL" ) {
    op = ConditionOp::UNCONDITIONAL;
  }
  else {
    std::cerr << "Invalid condition operator: " << opUpStr << "\n";
    std::cerr << "Exiting...\n";
    endwin();
    exit(1);
  }

  auto operand1 = c.readRequired( "operand1" );
  auto operand2 = c.readRequired( "operand2" ); // TODO only needs populating if op <= IS_UNSET
};

auto Condition::isMet() const -> bool {
  assert( operand1 != nullptr );
  switch ( op ) {
    case ConditionOp::GT:
      return std::get<int>(*operand1) > operand2;
    case ConditionOp::GTE:
      return std::get<int>(*operand1) >= operand2;
    case ConditionOp::LT:
      return std::get<int>(*operand1) < operand2;
    case ConditionOp::LTE:
      return std::get<int>(*operand1) <= operand2;
    case ConditionOp::EQ:
      return std::get<int>(*operand1) == operand2;
    case ConditionOp::NE:
      return std::get<int>(*operand1) != operand2;
    case ConditionOp::IS_SET:
      return std::get<Bitset>(*operand1)[static_cast<size_t>(operand2)];
    case ConditionOp::IS_UNSET:
      return ! (std::get<Bitset>(*operand1)[static_cast<size_t>(operand2)]);
    case ConditionOp::IS_ANY_SET:
      return std::get<Bitset>(*operand1).any();
    case ConditionOp::IS_ALL_SET:
      return std::get<Bitset>(*operand1).all();
    case ConditionOp::IS_NONE_SET:
      return std::get<Bitset>(*operand1).none();
    case ConditionOp::UNCONDITIONAL:
      return true;
  }
  return false;
}
