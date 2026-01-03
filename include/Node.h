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
    void run();
    /* I think run() will iterate through the events, like a behavior tree.
     * That lets us loop through a random number of battles too.
     * AND it lets us play music conditionally! If the dice roll says so.
     *
     * I think these will all be loaded at startup time.
     *
     * However, one doubt in my mind: Games are no longer pure data.
     * That is, if I don't specify my event types.
     *
     */

  private:
    std::vector<Node> _neighbors;
    std::string _name;
    std::string _description;
};
