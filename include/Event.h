#include <vector>
#include <memory>
#include <variant>
#include <bitset>
#include <yaml-cpp/yaml.h>

enum class EventState { FAILED, IN_PROGRESS, SUCCESS };

class Event {
  public: 

    static constexpr size_t NUM_SUPPORTED_BITS_PER_SET{32};
    using Bitset = std::bitset<NUM_SUPPORTED_BITS_PER_SET>;
    using Operand = std::shared_ptr<std::variant<int, Bitset>>;

    enum class ConditionOp {
      GT, GTE, LT, LTE, EQ, NE, IS_SET, IS_UNSET, IS_ANY_SET, IS_ALL_SET, IS_NONE_SET,
    };

    Event() = default;
    Event( YAML::Node& ymlConditions );
    auto getState() const -> EventState;
    auto meetsAllConditions() const -> bool;  // THIS, my boy, allows games to be pure data.
    auto meetsAnyCondition() const -> bool;  // THIS, my boy, allows games to be pure data.
    void addCondition( const Operand operand1, const int operand2, const ConditionOp& op );

    /// Must be overridden.
    virtual void run( bool condition ) = 0;

  private:

    struct Condition {
      Condition( const Operand operand1, const int operand2, const ConditionOp op ) :
        operand1(operand1),
        operand2(operand2),
        op(op) {}
      Operand operand1;
      int operand2;
      ConditionOp op;
      auto isMet() const -> bool;
    };
    EventState _state{};
    std::vector<Condition> _conditions{};
};
