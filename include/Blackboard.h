#pragma once
#include <any>
#include <string>
#include <map>
#include <memory>

using BbKey = std::string;
using Blackboard = std::map<BbKey, std::any>;

class BlackboardRegistry : public std::map<std::string, std::shared_ptr<Blackboard>> {
  public:
    static auto getInstance() -> BlackboardRegistry&;
    static void add( const std::string& name, const std::shared_ptr<Blackboard>& bb );
  private:
    BlackboardRegistry() = default;
    BlackboardRegistry( const BlackboardRegistry& rhs ) = delete;
    BlackboardRegistry& operator=( const BlackboardRegistry& ) = delete;
};

#define BB( _bbName_, ... ) auto _bbName_ = std::make_shared<Blackboard>( Blackboard { __VA_ARGS__ } );
