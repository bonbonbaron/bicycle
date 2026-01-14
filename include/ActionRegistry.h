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
        auto v = _bb.find( key ); 
        if ( v == _bb.end() ) {
          endwin();
          std::cerr << "Couldn't find key " << key << " in blackboard! Exiting...\n";
          exit(1);
        }
        return std::any_cast<T>(v->second);
      }

    template <typename T>
    void set ( const BbKey& key, const T& val ) {
      _bb[ key ] = val;
    }

    template <typename T>
    void set ( const BbKey&& key, const T& val ) {
      _bb[ key ] = val;
    }

  private:
    std::map<BbKey, std::any> _bb{};
};

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
