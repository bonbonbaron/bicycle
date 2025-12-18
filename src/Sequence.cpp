#include "Sequence.h"
#include "WindowManager.h"

// Sequence is a FIFO queue.
void Sequence::push( std::shared_ptr<Window> win ) {
  _windows.push( win );
}

// Sequence is a FIFO queue.
void Sequence::pop() {
  _windows.pop();
}

void Sequence::tick() {
  auto ptr = _currWindow.lock();
  // If the place where the first sequential window went is empty, load the next one on.
  if ( ptr == nullptr ) {
    assert( _windows.size() > 0 );
    // Get the first-out window.
    _currWindow = _windows.front();
    // Push the first-out (as in FIFO) window onto bicycle's window stack.
    bicycle::push( _windows.front() );
    // Now that the window's in bicycle's window list, we can remove it from our sequence.
    pop();  
  }
}

auto Sequence::size() -> int {
  return _windows.size();
}

auto Sequence::isComplete() -> bool {
  return ( size() == 0 ) && ( _currWindow.lock() == nullptr );
}
