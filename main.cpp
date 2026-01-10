#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Edge.h"
#include "Node.h"
#include "Event.h"

using namespace std;

EventState idk() {
  cout << "i did an event!\n";
  return EventState::SUCCESS;
}

int main() {
  auto cfg = YAML::LoadFile("./test.yml");

  auto& er = EventRegistry::get();
  er["idk"] = idk;
  er["i do know"] = [](){ cout << "I'm actually quite straight.\n"; return EventState::SUCCESS; };

  auto& cr = ConditionRegistry::get();
  cr["imnotgay"] = [](){ cout << "I'm NOT GAY I SWEAR\n"; return true; };

  auto n = cfg.as<bicycle::Node>();
  for ( const auto& [k,edge] : n.edges ) {
    cout << "\tname: " << k << "\n";
    cout << "\tweight: " << edge.getWeight() << "\n";
    cout << "\tendpointFilename: " << edge.getEndpoint() << "\n";
    edge.loadEndpoint();
  }
  n.event.run();

  return 0;
}
