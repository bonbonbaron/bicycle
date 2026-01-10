#pragma once
#include <functional>
#include <string>
#include <map>

class ConditionRegistry : public std::map<std::string, std::function<bool()>> {
  public:
    static auto get() -> ConditionRegistry& {
      static ConditionRegistry registry;
      return registry;
    }
    void add( const std::string& name, const std::function<bool()>& conditionFunc );  // Allows you to more easily make an condition mapping
  private:
    ConditionRegistry() = default;
    ConditionRegistry( const ConditionRegistry& rhs ) = delete;
    ConditionRegistry& operator=( const ConditionRegistry& ) = delete;
};

