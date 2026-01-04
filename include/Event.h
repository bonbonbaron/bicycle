#include <vector>
#include <memory>
#include <variant>
#include <bitset>
#include <yaml-cpp/yaml.h>
#include <functional>

enum class EventState { READY, FAILED, CONDITION_FAILED, IN_PROGRESS, SUCCESS };

class Event {
  public: 

    static constexpr size_t NUM_SUPPORTED_BITS_PER_SET{32};
    using Bitset = std::bitset<NUM_SUPPORTED_BITS_PER_SET>;
    using Operand = std::shared_ptr<std::variant<int, Bitset>>;

    enum class ConditionOp {
      GT, GTE, LT, LTE, EQ, NE, IS_SET, IS_UNSET, IS_ANY_SET, IS_ALL_SET, IS_NONE_SET,
    };

    Event() = default;
    Event( YAML::Node& cfg );
    auto getState() const -> EventState;
    auto meetsAllConditions() const -> bool;  // THIS, my boy, allows games to be pure data.
    auto meetsAnyCondition() const -> bool;  // THIS, my boy, allows games to be pure data.
    void addCondition( const Operand operand1, const int operand2, const ConditionOp& op );
    void setState( const EventState state );
    virtual void reset();

    /// May be overridden.
    virtual auto run() -> EventState;  // Absence of args implies events can be unconditional.

  private:

    // A more accurate name is "precondition," as one unmet blocks the event.
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
    std::function<EventState()> _func{};
};

/* Categories of actions:
 *  bicycle (oh, please get rid of Constellation
 */
