#pragma once
#include <functional>
#include <string>
#include <map>

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };
using Action = std::function<ActionState()>;

class ActionRegistry : public std::map<std::string, Action> {
  public:
    static auto get() -> ActionRegistry& {
      static ActionRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    void add( const std::string& name, const Action& actionFunc );
  private:
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

