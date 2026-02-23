#include <vector>
#include <bitset>
#include <iostream>
#include <iterator>

using namespace std;

constexpr unsigned BITS_PER_FIELD{64};
constexpr unsigned BITSHIFT{6};
constexpr unsigned MASK{BITS_PER_FIELD - 1};

void set( vector<bitset<BITS_PER_FIELD>>& v, const unsigned bitNum ) {
  v.at( bitNum >> BITSHIFT ).set( bitNum & MASK, true );
}

int main() {
  vector<bitset<BITS_PER_FIELD>> v;
  v = vector<bitset<BITS_PER_FIELD>>(5);

  // Customize data here
  set( v, 20 );
  set( v, 22 );
  set( v, 25 );
  set( v, 58 );
  set( v, 72 );
  
  // Loop
  int d{};
  for ( const auto& bs : v ) {
    if ( bs.any() ) {
      for ( size_t i = bs._Find_first(); i < bs.size(); i = bs._Find_next(i)) {
        cout << "bitset " << d << ", idx " << i << ": " << bs.test(i) << '\n';
      }
    }
    ++d;
  }
  return 0;
}
