#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <cursesw.h>

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using ActFunc = std::function<ActionState()>;
using BbKey = std::string;
using Blackboard = std::map<BbKey, std::any>;

class Action;
using ActionPtr = Action*;  // TODO how can i use smart pointers if it bombs saying the weak ptr is not set on construction?

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    static auto get() -> ActionRegistry& {
      static ActionRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    void add( const std::string& name, const ActionPtr& actionFunc =  nullptr ) {
      if ( actionFunc == nullptr ) {
        std::cerr << "function " << name << " is null! Exiting...\n";
        endwin();
        exit(1);
      }
      (*this)[name] = actionFunc;
    }

  private:
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};


class Action {
  public:
    Action( const std::string&& name ) {
      auto& reg = ActionRegistry::get();
      reg.add( name, this );
    }

    void setBlackboard( std::shared_ptr<Blackboard> bb ) {
      _bb = bb;
    }

    // TODO add function for getting any arbitrary object out of Blackboard.
    auto getBlackboard() const -> std::shared_ptr<Blackboard> {
      return _bb;
    }

    void set( const ActFunc f ) {
      this->f = f;
    }

    ActFunc f{};

  private:
    std::shared_ptr<Blackboard> _bb{};
};
