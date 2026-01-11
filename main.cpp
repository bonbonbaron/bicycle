#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <bicycle/Edge.h>
#include <bicycle/Node.h>
#include <bicycle/Event.h>
#include <bicycle/Timer.h>

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
  for ( const auto& [k,edge] : n.getEdges() ) {
    cout << "\tname: " << k << "\n";
    cout << "\tweight: " << edge.getWeight() << "\n";
    cout << "\tendpointFilename: " << edge.getEndpoint() << "\n";
    edge.loadEndpoint();
  }
  n.run();

  Timer timer{ []() { cout << "timer's doin' stuff\n"; }, };
  this_thread::sleep_until( Clock::now() + chrono::seconds( 3 ) );
  return 0;
}
