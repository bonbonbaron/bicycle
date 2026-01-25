#pragma once
#include <functional>
#include <string>
#include <map>
#include <memory>

using Condition = std::function<bool()>;

class ConditionRegistry : public std::map<std::string, std::shared_ptr<Condition>> {
  public:
    static auto getInstance() -> ConditionRegistry&;
    static void add( const std::string& name, const std::shared_ptr<Condition>& conditionFunc );  // Allows you to more easily make an condition mapping
  private:
    ConditionRegistry() = default;
    ConditionRegistry( const ConditionRegistry& rhs ) = delete;
    ConditionRegistry& operator=( const ConditionRegistry& ) = delete;
};

 
#define COND( _condName_ ) \
  auto _condName_##Ptr = std::make_shared<Condition>( _condName_ );\
  ConditionRegistry::add( #_condName_, _condName_##Ptr );  
