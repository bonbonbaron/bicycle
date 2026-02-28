#include "Bar.h"
#include <cursesw.h>

Bar::Bar(
    std::shared_ptr<unsigned> val, 
    const unsigned max, 
    const int x, 
    const int y, 
    const int len ) : 
  _curr( val ), 
  _max( max ),
  _x( x ),
  _y( y ),
  _len( len ),
  ONE_THIRD_MAX_BAR_LENGTH( _len / 3),  // red threshold
  TWO_THIRDS_BAR_LENGTH( static_cast<int>(2 * ONE_THIRD_MAX_BAR_LENGTH) ) {} // yellow threshold

void Bar::setPos( const int x, const int y ) {
  _x = x;
  _y = y;
}

void Bar::draw() const {

                                                                                     //
  constexpr int GREEN{1};
  constexpr int YELLOW{2};
  constexpr int RED{3};
  init_pair( GREEN, COLOR_WHITE, COLOR_GREEN );
  init_pair( YELLOW, COLOR_WHITE, COLOR_YELLOW );
  init_pair( RED, COLOR_WHITE, COLOR_RED );
  long unsigned filledLength{ static_cast<long unsigned>( _len * *_curr / _max ) };
  mvprintw( _y, _x, "[" );
  if ( has_colors() && can_change_color() ) {
    int colorNum{};
    if ( filledLength > TWO_THIRDS_BAR_LENGTH ) {
      colorNum = GREEN;
    }
    else if ( filledLength > ONE_THIRD_MAX_BAR_LENGTH ) {
      colorNum = YELLOW;
    }
    else {
      colorNum = RED;
    }
    attron( COLOR_PAIR(colorNum) );
    printw( "%.*s", filledLength, "          " );  // the filled-in portion
    attroff( COLOR_PAIR(colorNum) );
    printw( "%.*s", _len - filledLength, "          " );  // the empty portion
  }
  else {
    printw( "%.*s", filledLength, "@@@@@@@@@@" );
    printw( "%.*s", _len - filledLength, "          " );
  }
  printw( "]" );
}
