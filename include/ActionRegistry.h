#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <cursesw.h>

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using ActFunc = std::function<ActionState()>;
using BbKey = std::string;
class Blackboard {
  public:
    template<typename T>
      auto get( const BbKey& key ) -> T {
        try {
          auto& val = _bb.at( key );
          try {
            return std::any_cast<T>( _bb.at( key ) ); 
          }
          // Catch bad casting of std::any.
          catch ( const std::bad_any_cast &e ) {
            std::cerr << "blackboard tried to cast key " << key << "'s value to the wrong type. It contains a " << val.type().name() << ".\n";
            endwin();
            exit(1);
          }
        }
        // Catch blackboard missing a key.
        catch ( const std::out_of_range& e ) {
          std::cerr << "blackboard hasn't mapped for key " << key << " yet.\n";
          endwin();
          exit(1);
        }
      }

    template <typename T>
    void set ( const BbKey& key, const T& val ) {
      _bb[ key ] = val;
    }

    template <typename T>
    void set ( const BbKey&& key, const T&& val ) {
      _bb[ key ] = val;
    }

  private:
    std::map<BbKey, std::any> _bb{};
};

class Action;
using ActionPtr = Action*;  // TODO Figure out how to make this a smart pointer due to circular dependency.

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    static auto getInstance() -> ActionRegistry& {
      static ActionRegistry registry;
      return registry;
    }
    // Allows you to more easily make an event mapping
    auto get( const std::string& name ) const -> ActionPtr {
      return at( name );
    }
    // Allows you to more easily make an event mapping
    void set( const std::string& name, const ActionPtr& action ) {
      if ( action == nullptr ) {
        std::cerr << "ActionRegistry::set(): ActionPtr " << name << " is null! Exiting...\n";
        endwin();
        exit(1);
      }
      (*this)[name] = action;
    }

  private:
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};


class Action {
  public:
    Action() = default;
    Action( const std::string&& name, const ActFunc& f ) : f(f) {
      auto& reg = ActionRegistry::getInstance();
      // Protect devs from null function pointers.
      if ( f == nullptr ) {
        std::cerr << "Action::Action(): Action " << name << "'s function is null! Exiting...\n";
        endwin();
        exit(1);
      }
      reg.set( name, this );
    }

    void setBlackboard( std::shared_ptr<Blackboard> bb ) {
      _bb = bb;
    }

    // TODO add function for getting any arbitrary object out of Blackboard.
    auto getBlackboard() -> std::shared_ptr<Blackboard> {
      return _bb;
    }

    void set( const ActFunc f ) {
      this->f = f;
    }

    ActFunc f{};  // nullptr by default; guess they're ultimately function pointers

  private:
    std::shared_ptr<Blackboard> _bb{ std::make_shared<Blackboard>() };
};
