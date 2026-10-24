#ifndef _WIN32
#include <fcntl.h>
#endif
#include "c/SshInputListener.h"
#include "c/InputState.h"
#include "c/InputListener.h"
#include <chrono>
#include <cstdlib>
#include <cstring>

using namespace std;

auto SshInput::getInstance() -> SshInput& {
  static SshInput kbListener;
  return kbListener;
}

namespace {
  #ifndef _WIN32
  auto envOff( const char* name ) -> bool {
    const char* v = std::getenv( name );
    return v && ( *v == '0' || *v == 'n' || *v == 'N' );
  }
  #endif
  auto nowMs() -> double {
    using namespace std::chrono;
    return duration<double, std::milli>( steady_clock::now().time_since_epoch() ).count();
  }
#ifndef _WIN32
  struct termios savedTerm{};
  bool modesOn{};
  void writeOut( const char* s ) {
    const ssize_t n = ::write( STDOUT_FILENO, s, std::strlen( s ) );
    (void) n;
  }
  // Undo what the constructor asked of the terminal (kitty keys, SGR mouse), and the raw mode.
  void restoreTerminal() {
    if ( modesOn ) writeOut( "\x1b[<u\x1b[?1006l\x1b[?1000l" );
    modesOn = false;
    tcsetattr( STDIN_FILENO, TCSANOW, &savedTerm );
  }
#endif
}

#ifdef _WIN32
// The Windows console hands _getch() keys unbuffered and unechoed already.
SshInput::SshInput() { _keys.configure( std::getenv( "BICYCLE_TTY_KEYUP" ) ); }
#else
SshInput::SshInput() {
  tcgetattr( 0, &origTerm );
  savedTerm = origTerm;
  struct termios raw = origTerm;
  // Canonical mode = line-buffering; we don't want players to wait till they press Enter to process input.
  // Echo = printing input; we don't want that either!
  // ISIG *would* prevent signals like ^C from interrupting your program, but I think we should allow those.
  raw.c_lflag &= ~( ICANON | ECHO | IEXTEN );
  // IXON would allow pausing and unpausing program with ^S and ^Q respectively; we don't want that.
  // ICRNL translates carriage return (CR) to newline (NL); we don't want that.
  // BRKINT would prevent break interrupts; we want to keep those.
  // ISTRIP would strip the 8th bit off characters; we want to preserve them instead.
  raw.c_iflag &= ~( IXON | ICRNL | INPCK | ISTRIP );
  raw.c_cflag |= CS8;  // sets the character size to 8 bits to prevent truncation of larger char sizes
  raw.c_cc[VMIN] = 0;  // minimum number of bytes that must be read before read() returns
  raw.c_cc[VTIME] = 0;  // specifies timeout; 0 disables it (not sure that matters given the above)
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw );

  int flags = fcntl( STDIN_FILENO, F_GETFL, 0 );
  fcntl( STDIN_FILENO, F_SETFL, flags | O_NONBLOCK );

  _keys.configure( std::getenv( "BICYCLE_TTY_KEYUP" ) );
  if ( isatty( STDOUT_FILENO ) ) {
    // Real key releases where the terminal has them: push kitty keyboard flags 1|2|8 and ask whether it took
    // (terminals without the protocol ignore both; the reply CSI ? flags u turns real releases on).
    if ( !envOff( "BICYCLE_TTY_KITTY" ) ) writeOut( "\x1b[>11u\x1b[?u" );
    // SGR mouse: button presses and releases (CSI < b;x;y M / m).
    if ( !envOff( "BICYCLE_TTY_MOUSE" ) ) writeOut( "\x1b[?1000h\x1b[?1006h" );
    modesOn = true;
  }
  std::atexit( restoreTerminal );
}
#endif

void SshInput::send( LogicalKey k, bool pressed ) {
  InputListener::getInstance().press( k, pressed );  // InputListener::listen() sends it
  if ( pressed ) inputState().pressed.set( static_cast<unsigned>( k ) );
}

void SshInput::onEvent( const term::Event& ev, double t ) {
  using term::Kind;
  switch ( ev.kind ) {
    case Kind::BYTE:  // legacy byte: a press, or an auto-repeat of a held key
      if ( _keys.onByte( ev.key, t ) ) send( ev.key, true );
      break;
    case Kind::PRESS:
    case Kind::REPEAT:
      if ( _keys.press( ev.key, t ) ) send( ev.key, true );
      break;
    case Kind::RELEASE:
      if ( _keys.release( ev.key ) ) send( ev.key, false );
      break;
  }
}

void SshInput::_listen() {
  // Drain everything typed since last frame and send every event, in order. Reading only one byte per frame
  // let key bursts queue up and land frames late; keeping only the newest key dropped presses.
  const double t = nowMs();
  auto out = [&]( const term::Event& ev ) { onEvent( ev, t ); };
#ifdef _WIN32
  while ( _kbhit() ) {
    int c = _getch();
    if ( c == 0 || c == 0xE0 ) {  // arrow/function keys arrive as two codes
      const int a = _getch();
      const LogicalKey k = a == 72 ? LogicalKey::W : a == 80 ? LogicalKey::S : a == 75 ? LogicalKey::A : a == 77 ? LogicalKey::D : LogicalKey::COUNT;
      if ( k != LogicalKey::COUNT ) out( term::Event{ k, term::Kind::BYTE } );
      continue;
    }
    _decoder.feed( static_cast<unsigned char>( c ), out );
  }
#else
  unsigned char buf[256];
  ssize_t n{};
  while ( ( n = read( STDIN_FILENO, buf, sizeof buf ) ) > 0 ) {
    for ( ssize_t i = 0; i < n; ++i ) _decoder.feed( buf[i], out );
  }
#endif
  _decoder.endFrame( out );
  // Keys whose auto-repeat stopped are released (kitty keys and mouse buttons wait for their real release).
  _keys.expire( t, [&]( LogicalKey k ) { send( k, false ); } );
  // The polled state (scripts/Input3D.lua) sees what's down, like SDL's keyboard state.
  for ( unsigned k = 1; k < static_cast<unsigned>( LogicalKey::COUNT ); ++k ) {
    if ( _keys.down( static_cast<LogicalKey>( k ) ) ) inputState().held.set( k );
  }
}

void SshInput::listen() {
  auto& input = getInstance();
  input._listen();
}

auto SshInput::getState() const -> const Input& {
  return _inputState;
}
