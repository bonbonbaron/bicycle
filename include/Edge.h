#pragma once
#include <memory>

#include "YmlNode.h"
#include "Node.h"
#include "Condition.h"

struct Edge {
  Edge( const YmlNode& e ) {
    name = e.readRequired( "name" );
    open = e.readOptional( "open" ).value_or;
    weight = e.readOptional( "weight" ).value_or( 0 );
    endpointFilename = e.readRequired( "endpoint" );
  }

  std::string name;              // e.g. Door
  Condition open{};              // e.g. "Do you have more than 0 keys?"
  int weight{};                  // e.g. Number of random battles may be proportional to travel distance.
  std::string endpointFilename;  // Endpoint file's basename (no path or extension)
};
