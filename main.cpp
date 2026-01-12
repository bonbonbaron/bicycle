#include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

struct Person {
  Person() {}
  Person( const Person& e  ) = default;
  string name;
  string hobby;
  unsigned age{};
  std::shared_ptr<Person> _friend{};
};

void dispPerson( const Person& p ) {
  cout << "name: " << p.name << "\n";
  cout << "hobby: " << p.hobby << "\n";
  cout << "age: " << p.age << "\n";
  if ( p._friend != nullptr ) {
    cout << "friend:\n";
    dispPerson( *p._friend );
  }
}

static auto convertPerson( const YAML::Node& node ) -> Person {
  Person p;
  p.name = node["name"].as<string>();
  p.hobby = node["hobby"].as<string>();
  p.age = node["age"].as<unsigned>();
  if ( auto n = node["friend"] ) {
    p._friend = std::make_shared<Person>( convertPerson( n ) );
  }
  return p;
}

// Provide yaml-cpp library with template candidate for SequenceNode's specific struct
template<>
struct YAML::convert<Person> {
  static YAML::Node encode(const string& rhs) { return YAML::Node(rhs); }
  static bool decode(const YAML::Node& node, Person& rhs) {
    if (!node.IsMap()) {
      return false;
    }

    rhs = convertPerson( node );
      
    return true;
  }
};

int main() {
  auto cfg = YAML::LoadFile("./test.yml");
  auto peeps = cfg.as<vector<Person>>();

  for ( const auto& p : peeps ) {
    dispPerson( p );
  }
  

  return 0;
}
