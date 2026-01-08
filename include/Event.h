#include <vector>
#include <functional>

#include "YmlNode.h"
#include "Condition.h"

enum class EventState { READY, FAILED, CONDITION_FAILED, IN_PROGRESS, SUCCESS };

// TODO move Condition to its own file so Edges can use them too.
class Event {
  public: 

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
    EventState _state{};
    std::vector<Condition> _conditions{};
    std::function<EventState()> _func{};
};

