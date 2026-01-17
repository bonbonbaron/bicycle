#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <any>
#include <cxxabi.h>
#include <typeinfo>

#include "bicycle.h"

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
            T t;
            auto actual = __cxxabiv1::__cxa_demangle( val.type().name(), nullptr, 0, &i );
            auto expect = __cxxabiv1::__cxa_demangle( typeid(t).name(), nullptr, 0, &i );
            bicycle::die( std::string("Your call to get<") + expect + ">( \"" + key + "\" ) should be get<" + actual + ">( \"" + key + "\" ).\n" );
          }
        }
        // Catch blackboard missing a key.
        catch ( const std::out_of_range& e ) {
          bicycle::die( "blackboard hasn't mapped for key " + key + " yet." );
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
        bicycle::die( "Action Registry hasn't mapped anything yet to key " + name + "." );
      }
    }
    // Allows you to more easily make an event mapping
    void set( const std::string& name, const ActionPtr& action ) {
      if ( action == nullptr ) {
        bicycle::die( "ActionRegistry::set(): ActionPtr " + name + " is null!" );
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
        bicycle::die( "Action::Action(): Action " + name + "'s function is null!" );
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
