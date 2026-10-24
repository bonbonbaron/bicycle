#include "c/InputListener.h"
#include "c/Trigger.h"
#include <iostream>

auto InputListener::getInstance() -> InputListener& {
  static InputListener kbListener;
  return kbListener;
}

#if !defined(_WIN32)
void InputListener::discoverPotentialKeyboards() {
  auto dir = opendir("/dev/input");
  if (!dir) return;

  struct dirent* ent;
  while ((ent = readdir(dir))) {
    if (std::string(ent->d_name).find("event") == std::string::npos)
      continue;

    std::string path = "/dev/input/" + std::string(ent->d_name);
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) continue;

    struct libevdev* dev = nullptr;
    if (libevdev_new_from_fd(fd, &dev) != 0) {
      close(fd);
      continue;
    }

    bool isKeyboard = libevdev_has_event_code(dev, EV_KEY, KEY_A) ||
                      libevdev_has_event_code(dev, EV_KEY, KEY_ENTER) ||
                      libevdev_has_event_type(dev, EV_KEY);

    if (isKeyboard) {
      _devices.push_back({fd, dev, path});
      std::cout << "Keyboard listener attached: " << libevdev_get_name(dev) << " → " << path << "\n";
    } else {
      libevdev_free(dev);
      close(fd);
    }
  }
  closedir(dir);
}
#endif

InputListener::InputListener() {
#if !defined(_WIN32)
  discoverPotentialKeyboards();
#endif
}

void InputListener::listen() {
  // constexpr unsigned VAL_KEY_UP    = 0;
  // constexpr unsigned VAL_KEY_DOWN  = 1;
  // constexpr unsigned VAL_KEY_REPEAT = 2;

#if defined(_WIN32)
  // Poll Windows virtual keys (simple cross-platform stand-in for the Linux event loop)
  static const struct { int vk; LogicalKey lk; } map[] = {
    {'A', LogicalKey::A}, {'B', LogicalKey::B}, {'C', LogicalKey::C}, {'D', LogicalKey::D},
    {'E', LogicalKey::E}, {'F', LogicalKey::F}, {'G', LogicalKey::G}, {'H', LogicalKey::H},
    {'I', LogicalKey::I}, {'J', LogicalKey::J}, {'K', LogicalKey::K}, {'L', LogicalKey::L},
    {'M', LogicalKey::M}, {'N', LogicalKey::N}, {'O', LogicalKey::O}, {'P', LogicalKey::P},
    {'Q', LogicalKey::Q}, {'R', LogicalKey::R}, {'S', LogicalKey::S}, {'T', LogicalKey::T},
    {'U', LogicalKey::U}, {'V', LogicalKey::V}, {'W', LogicalKey::W}, {'X', LogicalKey::X},
    {'Y', LogicalKey::Y}, {'Z', LogicalKey::Z},
    {'0', LogicalKey::Key0}, {'1', LogicalKey::Key1}, {'2', LogicalKey::Key2},
    {'3', LogicalKey::Key3}, {'4', LogicalKey::Key4}, {'5', LogicalKey::Key5},
    {'6', LogicalKey::Key6}, {'7', LogicalKey::Key7}, {'8', LogicalKey::Key8},
    {'9', LogicalKey::Key9},
    {VK_RETURN, LogicalKey::Enter}, {VK_ESCAPE, LogicalKey::Escape},
    {VK_TAB, LogicalKey::Tab}, {VK_SPACE, LogicalKey::Space},
    {VK_LCONTROL, LogicalKey::LeftCtrl}, {VK_RCONTROL, LogicalKey::RightCtrl},
    {VK_LSHIFT, LogicalKey::LeftShift}, {VK_RSHIFT, LogicalKey::RightShift},
    {VK_LMENU, LogicalKey::LeftAlt}, {VK_RMENU, LogicalKey::RightAlt},
    {VK_LWIN, LogicalKey::LeftMeta}, {VK_RWIN, LogicalKey::RightMeta}
  };

  for (const auto& m : map) {
    bool down = (GetAsyncKeyState(m.vk) & 0x8000) != 0;
    _state.currKeysPressed.set(static_cast<std::size_t>(m.lk), down);
  }
  // Compute delta and fire if needed (adjust field names to match your _state)
  _state.deltaKeysPressed = _state.currKeysPressed ^ _state.prevKeysPressed;
  _state.prevKeysPressed = _state.currKeysPressed;
  if (_state.deltaKeysPressed.any()) {
    Trigger::sendInput(_state);
  }

#else
  if (_devices.empty()) {
    return;
  }

  for (unsigned i = 0; i < _devices.size(); ++i) {
    input_event ev{};
    while (read(_devices.at(i).fd, &ev, sizeof(ev)) == sizeof(ev)) {
      if (ev.type != EV_KEY || ev.value == VAL_KEY_REPEAT)
        continue;

      auto lk = convertCodeToLogicalInt(ev.code);
      if (lk == LogicalKey::COUNT)
        continue;

      if (ev.value == VAL_KEY_DOWN) {
        _state.currKeysPressed.set(static_cast<std::size_t>(lk), true);
        std::cout << "down on " << ev.code << '\n';
      } else if (ev.value == VAL_KEY_UP) {
        _state.currKeysPressed.set(static_cast<std::size_t>(lk), false);
        std::cout << "up on " << ev.code << '\n';
      }
    }
  }

  // if (_state.deltaKeysPressed.any())
  //   Trigger::sendInput(_state);
#endif
}

#if !defined(_WIN32)
auto InputListener::convertCodeToLogicalInt(int code) -> LogicalKey {
  switch (code) {
    case KEY_A: return LogicalKey::A;
    case KEY_B: return LogicalKey::B;
    // ... (rest of your existing switch)
    default: return LogicalKey::COUNT;
  }
}
#endif
#include "c/InputListener.h"
#include "c/Trigger.h"
#include "c/SshInputListener.h"
#include <cstdlib>

// ────────────────────────────────────────────────
//  Logical keys – our unified, cross-platform key namespace
//  Based on USB HID Usage IDs (Keyboard page 0x07)
// ────────────────────────────────────────────────

// Get singleton
auto InputListener::getInstance() -> InputListener& {
  static InputListener kbListener;
  return kbListener;
}

#if !defined(_WIN32)
void InputListener::discoverPotentialKeyboards()
{
  // close_all();  // TODO is this needed?

  auto dir = opendir("/dev/input");
  if (!dir) {
    return;
  }

  struct dirent* ent;
  while ((ent = readdir(dir))) {
    if ( std::string( ent->d_name ).find( "event" ) == std::string::npos ) {
      continue;
    }

    std::string path = "/dev/input/" + std::string(ent->d_name);

    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      continue;
    }

    struct libevdev* dev = nullptr;
    if (libevdev_new_from_fd(fd, &dev) != 0) {
      close(fd);
      continue;
    }

    // This is the test that reliably identifies keyboards (even through KVM switches)
    bool isKeyboard = libevdev_has_event_code(dev, EV_KEY, KEY_A) ||
      libevdev_has_event_code(dev, EV_KEY, KEY_ENTER) ||
      libevdev_has_event_type(dev, EV_KEY);

    if (isKeyboard) {
      _devices.push_back({fd, dev, path});
      std::cout << "Keyboard listener attached: " << libevdev_get_name(dev) << " → " << path << "\n";
    } 
    else {
      libevdev_free(dev);
      close(fd);
    }
  }
  closedir(dir);
}
#endif

// Constructor
InputListener::InputListener() {
#if defined(_WIN32)
  // Windows console: read real key-down/key-up records (no evdev here). Under mintty/MSYS2's default terminal stdin
  // is a pipe, not a console, so fall back to the terminal reader (SshInput) there.
  _console = GetStdHandle( STD_INPUT_HANDLE );
  DWORD mode{};
  if ( _console != INVALID_HANDLE_VALUE && GetConsoleMode( _console, &mode ) ) {
    SetConsoleMode( _console, ( mode | ENABLE_WINDOW_INPUT ) & ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE ) | ENABLE_EXTENDED_FLAGS );
  }
  else {
    _useTerminal = true;
  }
#else
  // Over SSH the keys arrive on stdin, not on this machine's /dev/input keyboards.
  if ( !std::getenv( "SSH_CONNECTION" ) && !std::getenv( "SSH_TTY" ) ) {
    discoverPotentialKeyboards();
  }
  _useTerminal = _devices.empty();
#endif
  if ( _useTerminal ) {
    SshInput::getInstance();  // raw terminal mode now, not on the first key
  }
}

void InputListener::press( LogicalKey lk, bool down ) {
  if ( lk == LogicalKey::COUNT || lk == LogicalKey::NOTHING ) {
    return;
  }
  _state.currKeysPressed.set( static_cast<std::size_t>( lk ), down );
}

// One Trigger::sendInput per key that went down or up since the last frame, as Lua's onInput( self, key, pressed ).
void InputListener::flush() {
  _state.deltaKeysPressed = _state.currKeysPressed ^ _state.prevKeysPressed;
  if ( _state.deltaKeysPressed.any() ) {
    // We don't have to know who has focus. Knowing focus is Lua-side's responsibility.
    for ( std::size_t i = 0; i < _state.deltaKeysPressed.size(); ++i ) {
      if ( !_state.deltaKeysPressed.test( i ) ) {
        continue;
      }
      Input ev = _state;
      ev.lastPressed = static_cast<LogicalKey>( i );
      ev.pressed = _state.currKeysPressed.test( i );
      Trigger::sendInput( ev );
    }
  }
  _state.prevKeysPressed = _state.currKeysPressed;
}

// Check for keyboard events; called once per frame by the ASCII renderer's pollInput().
void InputListener::listen() {
  if ( _useTerminal ) {
    SshInput::listen();
    return;
  }
#if defined(_WIN32)
  DWORD n{};
  while ( GetNumberOfConsoleInputEvents( _console, &n ) && n > 0 ) {
    INPUT_RECORD rec[32];
    DWORD got{};
    if ( !ReadConsoleInputW( _console, rec, 32, &got ) || got == 0 ) {
      break;
    }
    for ( DWORD i = 0; i < got; ++i ) {
      if ( rec[i].EventType != KEY_EVENT ) {
        continue;
      }
      const auto& k = rec[i].Event.KeyEvent;
      press( convertCodeToLogicalInt( k.wVirtualKeyCode ), k.bKeyDown != 0 );  // repeats don't change the bitset
    }
  }
#else
  // Constants
  constexpr int VAL_KEY_UP{0};
  constexpr int VAL_KEY_DOWN{1};
  constexpr int VAL_KEY_REPEAT{2};

  // Check for key-up-or-down events.
  for ( unsigned i = 0; i < _devices.size(); ++i ) {
    input_event ev{};
    // power through all the events currently queued on this device
    while ( read( _devices.at( i ).fd, &ev, sizeof( ev ) ) == sizeof( ev ) ) {
      // If it's not a key event, we're not interested.
      if ( ev.type != EV_KEY || ev.value == VAL_KEY_REPEAT ) {
        continue;
      }
      if ( ev.value == VAL_KEY_DOWN || ev.value == VAL_KEY_UP ) {
        press( convertCodeToLogicalInt( ev.code ), ev.value == VAL_KEY_DOWN );
      }
    }
  }
#endif
  flush();
}  // InputListener::listen()

// ────────────────────────────────────────────────
// Platform implementations (mapping native → LogicalKey)
// ────────────────────────────────────────────────


#if defined(_WIN32)
// Windows virtual-key codes. Arrows drive the same keys as WASD, as in the SDL renderers.
auto InputListener::convertCodeToLogicalInt(int code) -> LogicalKey {
  if ( code >= 'A' && code <= 'Z' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + ( code - 'A' ) );
  if ( code >= '1' && code <= '9' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::Key1 ) + ( code - '1' ) );
  switch ( code ) {
    case '0':         return LogicalKey::Key0;
    case VK_RETURN:   return LogicalKey::Enter;
    case VK_ESCAPE:   return LogicalKey::Escape;
    case VK_TAB:      return LogicalKey::Tab;
    case VK_SPACE:    return LogicalKey::Space;
    case VK_LCONTROL: case VK_CONTROL: return LogicalKey::LeftCtrl;
    case VK_RCONTROL: return LogicalKey::RightCtrl;
    case VK_LSHIFT:   case VK_SHIFT: return LogicalKey::LeftShift;
    case VK_RSHIFT:   return LogicalKey::RightShift;
    case VK_LMENU:    case VK_MENU: return LogicalKey::LeftAlt;
    case VK_RMENU:    return LogicalKey::RightAlt;
    case VK_UP:       return LogicalKey::W;
    case VK_LEFT:     return LogicalKey::A;
    case VK_DOWN:     return LogicalKey::S;
    case VK_RIGHT:    return LogicalKey::D;
    default:          return LogicalKey::COUNT;
  }
}
#else
// Linux evdev / input-event-codes.h uses its own set (often close to USB HID)
auto InputListener::convertCodeToLogicalInt(int code) -> LogicalKey {
  switch (code)
  {
    case KEY_A: return LogicalKey::A;
    case KEY_B: return LogicalKey::B;
    case KEY_C: return LogicalKey::C;
    case KEY_D: return LogicalKey::D;
    case KEY_E: return LogicalKey::E;
    case KEY_F: return LogicalKey::F;
    case KEY_G: return LogicalKey::G;
    case KEY_H: return LogicalKey::H;
    case KEY_I: return LogicalKey::I;
    case KEY_J: return LogicalKey::J;
    case KEY_K: return LogicalKey::K;
    case KEY_L: return LogicalKey::L;
    case KEY_M: return LogicalKey::M;
    case KEY_N: return LogicalKey::N;
    case KEY_O: return LogicalKey::O;
    case KEY_P: return LogicalKey::P;
    case KEY_Q: return LogicalKey::Q;
    case KEY_R: return LogicalKey::R;
    case KEY_S: return LogicalKey::S;
    case KEY_T: return LogicalKey::T;
    case KEY_U: return LogicalKey::U;
    case KEY_V: return LogicalKey::V;
    case KEY_W: return LogicalKey::W;
    case KEY_X: return LogicalKey::X;
    case KEY_Y: return LogicalKey::Y;
    case KEY_Z: return LogicalKey::Z;

    case KEY_0: return LogicalKey::Key0;
    case KEY_1: return LogicalKey::Key1;
    case KEY_2: return LogicalKey::Key2;
    case KEY_3: return LogicalKey::Key3;
    case KEY_4: return LogicalKey::Key4;
    case KEY_5: return LogicalKey::Key5;
    case KEY_6: return LogicalKey::Key6;
    case KEY_7: return LogicalKey::Key7;
    case KEY_8: return LogicalKey::Key8;
    case KEY_9: return LogicalKey::Key9;

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
    case KEY_UP:        return LogicalKey::W;
    case KEY_LEFT:      return LogicalKey::A;
    case KEY_DOWN:      return LogicalKey::S;
    case KEY_RIGHT:     return LogicalKey::D;

    default: return LogicalKey::COUNT;
  }
}  // Native key to LogicalKey converter
#endif
