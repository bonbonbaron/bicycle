#include "c/SshInput.h"
#include "c/InputData.h"

using namespace std;

auto SshInput::getInstance() -> SshInput& {
  static SshInput kbListener;
  return kbListener;
}

SshInput::SshInput() {
  tcgetattr( 0, &origTerm );
  struct termios raw = origTerm;
  raw.c_lflag &= ~( ICANON | ECHO );
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr( 0, TCSANOW, &raw );
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

// TODO
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
}

int SshInput::listen() {
  int key;
  char buffer[8] = {};
  
  while ( ( key = read( 0, buffer, 1 ) ) != 1 ) {
    cout << "you pressed " << key << ".\n";
  }
  if ( key == 'q' ) {
    return -1;
  }
  return 0;
}

