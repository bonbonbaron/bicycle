#pragma once
#include <vector>
#include <bitset>

constexpr unsigned BITS_PER_FIELD{64};
constexpr unsigned BITSHIFT{6};
constexpr unsigned MASK{ BITS_PER_FIELD - 1 };

template<typename T>
class System {
  public:
    System( const std::string&& id ) : _id( id ) {}

    // Virtual member functions
    virtual void run() = 0;

    void enable( Entity entity );
  private:
    std::string _id;                 
    std::vector<std::bitset<BITS_PER_FIELD>> _entitiesEnabled;
};
