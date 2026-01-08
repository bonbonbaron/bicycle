#pragma once
#include <memory>
#include <variant>
#include <bitset>
#include "YmlNode.h"

static constexpr size_t NUM_SUPPORTED_BITS_PER_SET{32};
using Bitset = std::bitset<NUM_SUPPORTED_BITS_PER_SET>;
using Operand = std::shared_ptr<std::variant<int, Bitset>>;

enum class ConditionOp {
  GT, GTE, LT, LTE, EQ, NE, IS_SET, IS_UNSET, IS_ANY_SET, IS_ALL_SET, IS_NONE_SET, UNCONDITIONAL
};

struct Condition {
  Condition( const YmlNode& cfg );
  Condition( const Operand operand1, const int operand2, const ConditionOp op ) :
    op(op) {},
    operand1(operand1),
    operand2(operand2)
  ConditionOp op{UNCONDITIONAL};
  Operand operand1;
  int operand2;
  auto isMet() const -> bool;
};
