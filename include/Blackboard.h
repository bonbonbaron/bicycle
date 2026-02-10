#pragma once
#include <any>
#include <string>
#include <map>
#include <memory>

using BbKey = std::string;
using Blackboard = std::map<BbKey, std::any>;

class BlackboardRegistry : public std::map<std::string, Blackboard> {
  public:
    static auto getInstance() -> BlackboardRegistry&;
    static void add( const BlackboardRegistry::value_type& val );
  private:
    BlackboardRegistry() = default;
    BlackboardRegistry( const BlackboardRegistry& rhs ) = delete;
    BlackboardRegistry& operator=( const BlackboardRegistry& ) = delete;
};

#define BB( _bbName_ )  { #_bbName_, _bbName_ }
