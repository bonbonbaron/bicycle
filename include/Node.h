#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include "Agent.h"

/* Okay, nice and simple. But how am I going to handle things like interacting with other characters?
 */
class Node {
  public:
    Node();
    void load();
    void run();  // traverses event tree

  private:
    std::vector<Node> _neighbors;
    std::string _name;
    std::string _description;
};
