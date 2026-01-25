#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>
#include <any>
#include <cxxabi.h>
#include <typeindex>

#include "bicycle.h"

enum class ActionState { READY, FAILED, IN_PROGRESS, SUCCESS };

using BbKey = std::string;
using PortSet = std::map<BbKey, std::type_index>;;

using Blackboard = std::map<BbKey, std::any>;

class ActArg {
  public:
    template<typename T>
    auto get( const BbKey key ) -> T& {
      try {
        auto& type = _ps->at( key );
        if ( type.name() != typeid(T).name() ) {
          int i;
          auto actual = __cxxabiv1::__cxa_demangle( type.name(), nullptr, 0, &i );
          auto expect = __cxxabiv1::__cxa_demangle( typeid(T).name(), nullptr, 0, &i );
          bicycle::die( std::string("Your call to get<") + expect + ">( \"" + key + "\" ) should be get<" + actual + ">( \"" + key + "\" ).\n" );
        }
        return std::any_cast<T&>( _bb->at( key ) );
      }
      // Catch blackboard missing a key.
      catch ( const std::out_of_range& e ) {
        bicycle::die( "blackboard hasn't mapped for key " + key + " yet." );
      }
    }
    template<typename T>
    void set( const BbKey key, const T& val ) {
      auto& type = _ps->at( key );
      if ( type.name() != typeid(T).name() ) {
        bicycle::die( "bad arg" );  // IDGAF right now, jsut make it work
      }
      return std::any_cast<T&>( _bb->at( key ) );
    }
    // BB is fed by ActionNode.
    void setBlackboard( std::shared_ptr<Blackboard> bb );
    // PS is fed by Action.
    void setPortSet( std::shared_ptr<PortSet> ps );
  private:
    std::shared_ptr<Blackboard> _bb;
    std::shared_ptr<PortSet> _ps;
};  // ActArg

using ActFunc = std::function<ActionState(ActArg&)>;

class Action;
using ActionPtr = std::shared_ptr<Action>;

class ActionRegistry : public std::map<std::string, ActionPtr> {
  public:
    // Allows you to more easily make an event mapping
    static auto get( const std::string& name ) -> ActionPtr;
    // Allows you to more easily make an event mapping
    static void add( const std::string& name, const ActionPtr& action );

  private:
    static auto getInstance() -> ActionRegistry&;
    ActionRegistry() = default;
    ActionRegistry( const ActionRegistry& rhs ) = delete;
    ActionRegistry& operator=( const ActionRegistry& ) = delete;
};

/* Although ActionFunc and ActionNode may seem already sufficient for behavior trees,
 * the Action class exists as a practical necessity: parameter-less action funcs can't
 * know about blackboards unless they're declared within the scope of a class/struct. 
 * And ActionNodes are read from YAMLs; we're not going to define a different YAML type
 * for every Action, so it's more feasible to separate the two. */

/* Since we can't make static member functions virtual, we'll enforce a concept on 
 * on Action's derived children. */

class PortTypeRegistry : public std::map< const std::string, std::type_index > {
  public:
    static void add( const std::string&& key, const std::type_index& val );
    static auto get( const std::string& key ) -> const std::type_index&;
  private:
    static auto getInstance() -> PortTypeRegistry&;
    PortTypeRegistry() = default;
    PortTypeRegistry( const PortTypeRegistry& ) = delete;
    PortTypeRegistry operator=( const PortTypeRegistry& ) = delete;
};

class Action {
  public:
    Action( const std::string& name, ActFunc f, std::vector<BbKey>&& ports = {} );
    auto getPortSet() -> std::shared_ptr<PortSet>;
    ActFunc f;
  protected:
    std::shared_ptr<PortSet> _portSet{ std::make_shared<PortSet>() };  // universal for all entities
};  // Action

// TODO see if there's some clever way you can auto-call ACTION macro after function definition.
#define F_( _funcName_, ... ) \
  static auto _funcName_( ActArg& arg ) -> ActionState {\
    __VA_ARGS__\
  }
 
#define ACT( _funcName_, ... ) \
  auto _funcName_##Ptr = std::make_shared<Action>( std::string(#_funcName_), _funcName_ __VA_OPT__(, std::vector<BbKey>{) __VA_ARGS__ __VA_OPT__(}) );\
  ActionRegistry::add( #_funcName_, _funcName_##Ptr );  // Call this in the context of a registration function so you only have to get the instance once.

#define PORT( _portName_, _type_ )\
  PortTypeRegistry::add( string(#_portName_), std::type_index( typeid( _type_ ) ) );
