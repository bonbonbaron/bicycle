#pragma once
// #include "m/Entity.h"  // TODO uncomment this when you're ready
using Entity = int; // TODO delete this line when you're ready

class System {
  public:
    System( const std::string&& id ) : _id( id ) {}

    // Virtual member functions
    virtual void run() = 0;
    virtual void enable( const Entity entity ) = 0;
    virtual void disable( const Entity entity ) = 0;

  private:
    std::string _id;                 
};
