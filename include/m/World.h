#pragma once
#include <typeindex>
// #include <string>
#include <variant>
#include <array>
#include <unordered_map>
#include <cassert>

// TODO delete this line when you sort Entities out
using Entity = unsigned;
constexpr unsigned NUM_SUPPORTED_ENTITIES{256};

struct Position {
  int x{};
  int y{};
};

struct Size {
  unsigned w{};
  unsigned h{};
};

using Color = unsigned int;

struct Image {
  Size size{};
  Color color{};
  unsigned char data[];
};

using Time = unsigned;

using ArrayVar = std::variant<
  std::array<Position, NUM_SUPPORTED_ENTITIES>,
  std::array<Size, NUM_SUPPORTED_ENTITIES>,
  std::array<Image, NUM_SUPPORTED_ENTITIES>,
  std::array<Time, NUM_SUPPORTED_ENTITIES>
>;

template <typename T>
struct TypeTag {
  static inline const char dummy{};
};

template <typename T>
inline const void* getTypeTag() noexcept {
  return &TypeTag<T>::dummy;
}

class World {  // The name "World" is too dramatic. It doesn't include BBs. I'll rename later.
  public:
    // Allows you to more easily make an event mapping
    template <typename T>
    static void add( const Entity entity, const T& val ) {
      auto& world = getInstance();
      //world.
    }

    template <typename T>
    static void set( const unsigned idx, const T& val ) {
      auto& world = getInstance();
      auto& arr = world.get( getTypeTag<T>() );
      auto& t_arr = std::get< std::array< T, NUM_SUPPORTED_ENTITIES > >( arr );
      t_arr.at( idx ) = val;
    }

    template <typename T>
    static auto get() -> std::array<T, NUM_SUPPORTED_ENTITIES>& {
      auto& world = getInstance();
      auto& arr = world.get( getTypeTag<T>() );
      return std::get< std::array< T, NUM_SUPPORTED_ENTITIES > >( arr );
    }

    template <typename T>
    static auto get( const Entity entity ) -> T& {
      auto& world = getInstance();
      auto& arr = world.get( getTypeTag<T>() );
      auto& castArr = std::get< std::array< T, NUM_SUPPORTED_ENTITIES > >( arr );
      return castArr.at(entity);
    }

    auto get( const void* type ) -> ArrayVar& {
      assert( _m.contains( type ) );
      return _m.at( type );
    }

    // Variadic template - initialize any number of types (including zero)
    template <typename... Ts>
      void initialize() {
        (initialize_one<Ts>(), ...);   // C++17 fold expression
      }

    // Convenience: initialize all known types in one call
    void initialize_all() {
      initialize<Position, Size, Image, Time>();
    }

    // Optional: check whether a type is already initialized
    template <typename T>
      bool is_initialized() const {
        return _m.contains( getTypeTag<T>() );
      }

  private:
    template <typename T>
      void initialize_one() {
        auto [it, inserted] = _m.try_emplace( getTypeTag<T>() );
        if (inserted) {
          it->second.template emplace<std::array<T, NUM_SUPPORTED_ENTITIES>>();
          // Optional: pre-fill with default values
          // std::get<std::array<T, NUM_SUPPORTED_ENTITIES>>(it->second).fill(T{});
        }
        // else: already exists → do nothing / or throw / log / etc.
      }

    World();
    World( const World& rhs ) = delete;
    World& operator=( const World& ) = delete;
    static auto getInstance() -> World&;

    std::unordered_map< const void*, ArrayVar > _m;
}; 

