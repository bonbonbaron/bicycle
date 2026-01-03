#include <yaml-cpp/yaml.h>
#include <string_view>
#include <iostream>
#include <cursesw.h>
#include <memory>
#include <optional>

class Config {

  static constexpr std::string_view CONFIG_FILEPATH{ "./config/test.yml" };

  public:
  Config();
  private:

  class Node {
    public:
      // ==343208==    by 0x1171E3: Config::Node::Node(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&&) (Config.h:18)

      Node( const std::string&& cfgFilepath ) : _filepath( std::make_shared<std::string>( cfgFilepath ) ), _wrappedNode( YAML::LoadFile( cfgFilepath ) ){}
      Node() = default;
      explicit Node( const YAML::Node& yamlNode, const std::shared_ptr<std::string>& filepath ) : _wrappedNode( yamlNode ), _filepath(filepath) {}
      explicit Node( const YAML::Node&& yamlNode, const std::shared_ptr<std::string>& filepath ) : _wrappedNode( yamlNode ), _filepath(filepath) {} 

      void stealFilepath( std::shared_ptr<std::string> filepath ) {
        _filepath = filepath;
      }

      auto getChild( const std::string&& childPath ) const -> Node {
        return Node( _wrappedNode[childPath], _filepath );
      }

      template<typename T>
        auto readRequired( const std::string&& key ) -> T {
          if ( YAML::Node node = _wrappedNode[key] ) {
            try {
              stealFilepath( _filepath );  // prevents us from having to make an annoying getRoot() function
              // Return the appropriate type.
              return node.as<T>();  // We can handle maps and sequences with T as well.
            }
            catch ( YAML::Exception& e ) {
              std::cerr << *_filepath << ":" << node.Mark().line + 1 << " couldn't convert to desired type. Details:\n";
              std::cerr << e.what();
              std::cerr << "Exiting...\n";
              endwin();
              exit(1);
            }
          }
          else {
            std::cerr << *_filepath << ":" << _wrappedNode.Mark().line + 1 << " " << _wrappedNode.Tag() << " didn't have expected member " << key << ". Exiting...\n";
            endwin();
            exit(1);
          }
        }  // readRequired()

      template<typename T>
        auto readOptional( const std::string&& key ) -> std::optional<T> {
          if ( YAML::Node node = _wrappedNode[key] ) {
            try {
              return {node.as<T>()};
            }
            catch ( YAML::Exception& e ) {
              std::cerr << *_filepath << ":" << node.Mark().line + 1 << " couldn't convert to desired type. Details:\n";
              std::cerr << e.what();
              std::cerr << "Exiting...\n";
              endwin();
              exit(1);
            }
          }
          return {};
        }  // readOptional()
    private:
      std::shared_ptr<std::string> _filepath{};  // If this is empty, this isn't the root node for this file.
      YAML::Node _wrappedNode{};
  };  // private class Node

  Node _root;
};
