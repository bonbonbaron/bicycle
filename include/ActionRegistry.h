#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <any>
#include <cursesw.h>
#include <cxxabi.h>

/* It seems like it shoudl be possible to determine the type of BB element, 
 * and then get it however it is. Because we could map keys to demangled
 * type names, and demangled type names to... but then how would the compiler
 * know how to operate on it in the caller? Yeah, that doesn't make sense.
 * But it would help to know what types "someVar" is expected to be everywhere.
 * I'd hate to have to memorize and do constant looking up of types.

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
            int i;
            auto s = __cxxabiv1::__cxa_demangle( val.type().name(), nullptr, 0, &i );
            std::cerr << "blackboard tried to cast key " << key << "'s value to the wrong type. It contains type " << s << ".\n";
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
      try {
        return at( name );
      }
      catch ( const std::out_of_range& e ) {
        std::cerr << "Action Registry hasn't mapped anything yet to key " << name << ".\n";
        endwin();
        exit(1);
      }
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

    // TODO there may need to be a reference variant of these. 
    //      Wonder how we do that since any_cast() forces copying.
    template<typename T>
      auto get( const BbKey& key ) -> T {
        return _bb->get<T>( key );
      }

    template<typename T>
      void set( const BbKey& key, T&& val ) {
        _bb->set<T>( key, std::forward<T>( val ) );
      }

    ActFunc f{};  // nullptr by default; guess they're ultimately function pointers

  private:
    std::shared_ptr<Blackboard> _bb{ std::make_shared<Blackboard>() };
};

// macro for defining actions
#define ACT( _type_, ... )\
  namespace _type_ {\
    struct _type_ : public Action {\
      _type_() : Action( #_type_, __VA_ARGS__ ) {}\
    };\
    static _type_ _;\
  }
