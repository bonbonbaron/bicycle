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

    // Concrete member functions
    void addEntity( const unsigned entityId, const T& component, const bool enabled ) {
      if ( _components.size() < entityId ) {
        _components.resize( entityId * 2 );  // double entityId to minimize number of reallocations
        _entitiesEnabled.resize( entityId * 2 );
      }
      _components.at( entityId ) = component;
      _entitiesEnabled.at( entityId >> BITSHIFT ).set( entityId & MASK, enabled );
    }


    void setEnabled( const unsigned entityId, const bool enabled ) {
      if ( _entitiesEnabled.size() >= entityId ) {
        _entitiesEnabled.at( entityId >> BITSHIFT ).set( entityId & MASK, enabled );
      }
    }

  private:
    std::string _id;                 
    std::vector<T> _components{};
    std::vector<std::bitset<BITS_PER_FIELD>> _entitiesEnabled;
};
