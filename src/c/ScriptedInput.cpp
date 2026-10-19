#include "c/ScriptedInput.h"
#include "c/InputState.h"
#include "c/Trigger.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

namespace {
// The items of an environment variable split at sep (none if it's unset).
auto items( const char* var, char sep ) -> std::vector<std::string> {
  std::vector<std::string> out;
  if ( const char* s = std::getenv( var ) ) {
    std::stringstream ss( s );
    std::string item;
    while ( std::getline( ss, item, sep ) ) out.push_back( item );
  }
  return out;
}

void send( LogicalKey key, bool down ) {
  Input in{};
  if ( down ) in.currKeysPressed.set( static_cast<unsigned>( key ) );
  in.lastPressed = key;
  Trigger::sendInput( in, down );
}
}  // namespace

auto ScriptedInput::fromEnv() -> ScriptedInput {
  ScriptedInput s;
  for ( const auto& item : items( "BICYCLE_SCRIPTED_INPUT", ',' ) ) {
    char k{};
    long from{}, to{}, every{ 1 };
    if ( std::sscanf( item.c_str(), "%c@%ld-%ld/%ld", &k, &from, &to, &every ) < 3 || every <= 0 ) continue;
    const char c = static_cast<char>( std::tolower( static_cast<unsigned char>( k ) ) );
    const LogicalKey key = c == ' ' ? LogicalKey::Space : c >= 'a' && c <= 'z' ? static_cast<LogicalKey>( c - 'a' + 1 ) : LogicalKey::NOTHING;
    if ( key != LogicalKey::NOTHING ) s._keys.push_back( { key, from, to, every, false } );
  }
  for ( const auto& item : items( "BICYCLE_SCRIPTED_MOUSE", ';' ) ) {
    Mouse m{};
    char b{};
    if ( std::sscanf( item.c_str(), "%f:%f@%ld-%ld", &m.dx, &m.dy, &m.from, &m.to ) == 4 ) {
      m.button = -1;
      s._mouse.push_back( m );
    }
    else if ( std::sscanf( item.c_str(), "%c@%ld-%ld", &b, &m.from, &m.to ) == 3 ) {
      m.button = b == 'L' ? 0 : b == 'R' ? 1 : 2;
      s._mouse.push_back( m );
    }
  }
  return s;
}

void ScriptedInput::play( long frame ) {
  auto& held = inputState();
  for ( auto& k : _keys ) {
    const bool down = frame >= k.from && frame <= k.to && ( frame - k.from ) % k.every == 0;
    if ( down != k.down ) {
      send( k.key, down );
      if ( down ) held.pressed.set( static_cast<unsigned>( k.key ) );
      k.down = down;
    }
    if ( down ) held.held.set( static_cast<unsigned>( k.key ) );
  }
  unsigned buttons{};
  float lookDx{}, lookDy{};
  for ( const auto& m : _mouse ) {
    if ( frame < m.from || frame > m.to ) continue;
    if ( m.button < 0 ) {
      held.mouseDx += m.dx;
      held.mouseDy += m.dy;
      lookDx += m.dx;
      lookDy += m.dy;
    }
    else buttons |= 1u << m.button;
  }
  if ( lookDx != 0 || lookDy != 0 ) Trigger::sendLook( lookDx, lookDy );
  for ( unsigned b = 0; b < 3; ++b ) {  // scripted buttons are events too (MouseLeft/Right/Middle)
    const bool now = buttons & ( 1u << b ), before = _buttons & ( 1u << b );
    if ( now == before ) continue;
    Input in{};
    in.lastPressed = static_cast<LogicalKey>( static_cast<int>( LogicalKey::MouseLeft ) + static_cast<int>( b ) );
    Trigger::sendInput( in, now );
  }
  held.mouseButtons = ( held.mouseButtons & ~_buttons ) | buttons;
  held.mousePressed |= buttons & ~_buttons;
  _buttons = buttons;
}
