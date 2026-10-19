#pragma once
// ActiveFirst: a table whose active rows come first (jollybean's fray, src/data/fray.c, as a typed C++ class).
//
//   rows  [0, nActive())        active   (moving entities, running timers): what a system's frame loop visits
//         [nActive(), size())   inactive (still entities, paused timers): kept, but skipped by those loops
//
// Each column is one contiguous array (structure of arrays), so a loop over the active prefix of a column is a plain
// counted loop over contiguous memory, which compilers can vectorize. Rows are found by key (an entity, a timer id)
// through an index map, so add, remove, activate and deactivate are O(1): a row changes state by swapping places
// with the row at the active/inactive boundary. Row order is therefore not stable; keys are.
//
//   ActiveFirst<int, float> t;     // two columns
//   t.add( key, 1, 2.f );          // inactive at first
//   t.activate( key );
//   int* a = t.col<0>();  for ( uint32_t i = 0; i < t.nActive(); ++i ) a[i] += 1;
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

template <typename... Columns>
class ActiveFirst {
  public:
    using Key = uint32_t;
    static constexpr uint32_t NONE = ~0u;

    auto size() const -> uint32_t { return static_cast<uint32_t>( _keys.size() ); }
    auto nActive() const -> uint32_t { return _nActive; }
    auto contains( Key k ) const -> bool { return k < _rowOf.size() && _rowOf[k] != NONE; }
    auto isActive( Key k ) const -> bool { return contains( k ) && _rowOf[k] < _nActive; }
    auto row( Key k ) const -> uint32_t { return contains( k ) ? _rowOf[k] : NONE; }
    auto key( uint32_t row ) const -> Key { return _keys[row]; }
    auto keys() const -> const Key* { return _keys.data(); }

    // Column I: its rows in order (the active ones first), or one key's cell.
    template <size_t I> auto col() { return std::get<I>( _cols ).data(); }
    template <size_t I> auto col() const { return std::get<I>( _cols ).data(); }
    template <size_t I> auto& at( Key k ) { return std::get<I>( _cols )[_rowOf[k]]; }
    template <size_t I> const auto& at( Key k ) const { return std::get<I>( _cols )[_rowOf[k]]; }

    // A new, inactive row. False (and nothing changes) if the key is already in.
    auto add( Key k, Columns... cells ) -> bool {
      if ( contains( k ) ) return false;
      if ( _rowOf.size() <= k ) _rowOf.resize( k + 1, NONE );
      _rowOf[k] = size();
      _keys.push_back( k );
      push( std::index_sequence_for<Columns...>{}, std::move( cells )... );
      return true;
    }

    void remove( Key k ) {
      if ( !contains( k ) ) return;
      deactivate( k );                    // now it's an inactive row...
      swapRows( _rowOf[k], size() - 1 );  // ...and the last one
      _rowOf[k] = NONE;
      _keys.pop_back();
      std::apply( []( auto&... c ) { ( c.pop_back(), ... ); }, _cols );
    }

    // True if the row changed state.
    auto activate( Key k ) -> bool {
      if ( !contains( k ) || _rowOf[k] < _nActive ) return false;
      swapRows( _rowOf[k], _nActive++ );
      return true;
    }
    auto deactivate( Key k ) -> bool {
      if ( !isActive( k ) ) return false;
      swapRows( _rowOf[k], --_nActive );
      return true;
    }

    void clear() {
      for ( const Key k : _keys ) _rowOf[k] = NONE;
      _keys.clear();
      std::apply( []( auto&... c ) { ( c.clear(), ... ); }, _cols );
      _nActive = 0;
    }

  private:
    template <size_t... I> void push( std::index_sequence<I...>, Columns&&... cells ) {
      ( std::get<I>( _cols ).push_back( std::move( cells ) ), ... );
    }
    void swapRows( uint32_t a, uint32_t b ) {
      if ( a == b ) return;
      std::swap( _keys[a], _keys[b] );
      _rowOf[_keys[a]] = a;
      _rowOf[_keys[b]] = b;
      std::apply( [a, b]( auto&... c ) { ( std::swap( c[a], c[b] ), ... ); }, _cols );
    }

    std::vector<Key> _keys;                  // row -> key
    std::vector<uint32_t> _rowOf;            // key -> row (NONE: not in the table)
    std::tuple<std::vector<Columns>...> _cols;
    uint32_t _nActive{};
};
