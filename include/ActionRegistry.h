#pragma once
#include <functional>
#include <string>
#include <map>

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

class ActionRegistry : public std::map<std::string, std::function<ActionState()>> {
  public:
    static auto get() -> ActionRegistry& {
      static ActionRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    void add( const std::string& name, const std::function<ActionState()>& eventFunc );
  private:
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

