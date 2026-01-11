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
  this_thread::sleep_until( Clock::now() + chrono::milliseconds( 300 ) );
  return 0;
}

/* Current problem: Even if the node has an input reader, it doesn't know whom to send that to. Is it for the current menu? Windows aren't reacting anymore.
 *                  It's the controller's job to mess with the model now, and the view's job to represent it.
 *                  How can MVC marry up with the stack architecture?
 *                  If things have keys, maybe we can align them that way. 
