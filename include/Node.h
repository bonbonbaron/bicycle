#pragma once
#include <vector>
#include <string>
#include "Event.h"

class Node {
  public:
    Node();
    void load();
    void run();  // traverses event tree
    void traverse( const std::string& neighbor ) const;

  private:
    std::map<std::string, Node> _neighbors{};
    Event eventTreeRoot;
    std::string _name;
    std::string _description;
};
