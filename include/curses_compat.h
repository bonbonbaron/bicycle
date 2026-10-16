#pragma once
// One curses for every platform: ncurses on Linux and macOS, PDCurses on Windows. Both provide <curses.h>.
// curses.h may define some functions as macros, which would clobber member functions with the same names
// (Window::clear(), Window::move()...). ncurses' C++ header cursesw.h used to undefine them; do the same here.
#include <curses.h>
#undef clear
#undef erase
#undef move
#undef refresh
#undef getch
#undef addch
#undef box
#undef border
#undef timeout
#undef scroll
#undef instr
#undef insch
#undef delch
#undef deleteln
#undef insertln
#undef attron
#undef attroff
#undef attrset
#undef standout
#undef standend
#undef getstr
