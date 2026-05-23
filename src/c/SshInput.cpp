#include "c/SshInput.h"
#include "c/InputData.h"
#include <fcntl.h>

using namespace std;

auto SshInput::getInstance() -> SshInput& {
  static SshInput kbListener;
  return kbListener;
}

SshInput::SshInput() {
  tcgetattr( 0, &origTerm );
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
  // tcsetattr( STDIN_FILENO, TCSANOW, &raw );  // makes changes take effect immediately w/o flushing current content
  tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw );
  
  int flags = fcntl( STDIN_FILENO, F_GETFL, 0 );
  fcntl( STDIN_FILENO, F_SETFL, flags | O_NONBLOCK );
}

SshInput::~SshInput() {
  tcsetattr( 0, TCSANOW, &origTerm );
}

auto SshInput::convertCodeToLogicalInt(int code) -> LogicalKey {
  switch (code)
  {
    case 'a': case 'A': return LogicalKey::A;
    case 'b': case 'B': return LogicalKey::B;
    case 'c': case 'C': return LogicalKey::C;
    case 'd': case 'D': return LogicalKey::D;
    case 'e': case 'E': return LogicalKey::E;
    case 'f': case 'F': return LogicalKey::F;
    case 'g': case 'G': return LogicalKey::G;
    case 'h': case 'H': return LogicalKey::H;
    case 'i': case 'I': return LogicalKey::I;
    case 'j': case 'J': return LogicalKey::J;
    case 'k': case 'K': return LogicalKey::K;
    case 'l': case 'L': return LogicalKey::L;
    case 'm': case 'M': return LogicalKey::M;
    case 'n': case 'N': return LogicalKey::N;
    case 'o': case 'O': return LogicalKey::O;
    case 'p': case 'P': return LogicalKey::P;
    case 'q': case 'Q': return LogicalKey::Q;
    case 'r': case 'R': return LogicalKey::R;
    case 's': case 'S': return LogicalKey::S;
    case 't': case 'T': return LogicalKey::T;
    case 'u': case 'U': return LogicalKey::U;
    case 'v': case 'V': return LogicalKey::V;
    case 'w': case 'W': return LogicalKey::W;
    case 'x': case 'X': return LogicalKey::X;
    case 'y': case 'Y': return LogicalKey::Y;
    case 'z': case 'Z': return LogicalKey::Z;

    case '0': return LogicalKey::Key0;
    case '1': return LogicalKey::Key1;
    case '2': return LogicalKey::Key2;
    case '3': return LogicalKey::Key3;
    case '4': return LogicalKey::Key4;
    case '5': return LogicalKey::Key5;
    case '6': return LogicalKey::Key6;
    case '7': return LogicalKey::Key2;
    case '8': return LogicalKey::Key8;
    case '9': return LogicalKey::Key9;

// TODO: support these keys as well once you figure out their ASCII codes.
#if 0
    case KEY_ENTER:     return LogicalKey::Enter;
    case KEY_ESC:       return LogicalKey::Escape;
    case KEY_TAB:       return LogicalKey::Tab;
    case KEY_SPACE:     return LogicalKey::Space;

    case KEY_LEFTCTRL:  return LogicalKey::LeftCtrl;
    case KEY_RIGHTCTRL: return LogicalKey::RightCtrl;
    case KEY_LEFTSHIFT: return LogicalKey::LeftShift;
    case KEY_RIGHTSHIFT:return LogicalKey::RightShift;
    case KEY_LEFTALT:   return LogicalKey::LeftAlt;
    case KEY_RIGHTALT:  return LogicalKey::RightAlt;
    case KEY_LEFTMETA:  return LogicalKey::LeftMeta;
    case KEY_RIGHTMETA: return LogicalKey::RightMeta;
#endif

    default: return LogicalKey::COUNT;
  }
} // SshInput::convertCodeToLogicalInt()

void SshInput::_listen() {
  char buffer[static_cast<unsigned>(LogicalKey::COUNT)] = {};  
  // TODO i put "8" so i can support multiple button presses per frame later... 
  // for now, i'm happy with actually only working wiht one byte
  // Reads 1 byte from fd 0 into a buffer.
  read( STDIN_FILENO, buffer, 1 );
  if ( buffer[0] != '\0' ) {
    auto lkey = convertCodeToLogicalInt( buffer[0] );
    _inputState.currKeysPressed.reset(); // SSH mode doesn't support key press/release distinctions. Press-detections only.
    _inputState.currKeysPressed.set( static_cast<unsigned>( lkey ) );
    cout << _inputState.currKeysPressed << '\n';
    // TODO iter thru all high bits to determine whether we process "KEY DOWN" events for them or not.
    //      We're fortunate to know that an entire frame's timespan has passed since last processing this.
    //      So we can check unconditionally without regard for time in Input's perspective.
  }
}

void SshInput::listen() {
  auto& input = getInstance();
  input._listen();
}

