#pragma once
#include <functional>
#include <string>
#include <map>

enum class EventState { READY, FAILED, CONDITION_FAILED, IN_PROGRESS, SUCCESS };

class EventRegistry : public std::map<std::string, std::function<EventState()>> {
  public:
    static auto get() -> EventRegistry& {
      static EventRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    void add( const std::string& name, const std::function<EventState()>& eventFunc );
  private:
    EventRegistry() = default;
    EventRegistry( const EventRegistry& rhs ) = delete;
    EventRegistry& operator=( const EventRegistry& ) = delete;
};

