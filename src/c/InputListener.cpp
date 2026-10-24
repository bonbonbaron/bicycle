#include "c/InputListener.h"
#include "c/Trigger.h"
#include <cstdlib>
#include <iostream>
#if defined(BICYCLE_RENDER_ASCII)
#  include "c/SshInputListener.h"
#endif
#if !defined(_WIN32)
#  include <dirent.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <linux/input.h>
#  if defined(BICYCLE_RENDER_ASCII)
#    include <libevdev/libevdev.h>
#  endif
#endif

auto InputListener::getInstance() -> InputListener& {
  static InputListener listener;
  return listener;
}

// Pick the backend. SDL builds get their keys from the window, so there's nothing to open.
InputListener::InputListener() {
#if defined(BICYCLE_RENDER_ASCII)
#  if defined(_WIN32)
  // CONIN$ is the console even when stdin is redirected. A pipe on stdin means mintty without ConPTY: its keys
  // never reach the console, so read the keyboard state instead.
  DWORD mode{};
  const bool stdinIsConsole = GetConsoleMode( GetStdHandle( STD_INPUT_HANDLE ), &mode );
  _conin = CreateFileW( L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        OPEN_EXISTING, 0, nullptr );
  if ( stdinIsConsole && _conin != INVALID_HANDLE_VALUE && GetConsoleMode( _conin, &mode ) ) {
    mode &= ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT );
    SetConsoleMode( _conin, mode | ENABLE_EXTENDED_FLAGS );
    _backend = Backend::CONSOLE;
  }
  else {
    _backend = Backend::ASYNC_KEYS;
  }
#  else
  // Over SSH the keys arrive on stdin, not on this machine's keyboards.
  const bool ssh = std::getenv( "SSH_CONNECTION" ) || std::getenv( "SSH_TTY" );
  if ( !ssh && !std::getenv( "BICYCLE_NO_EVDEV" ) ) openEvdevKeyboards();
  _backend = _evdevFds.empty() ? Backend::TERMINAL : Backend::EVDEV;
  if ( _backend == Backend::TERMINAL ) SshInput::getInstance();  // raw terminal mode now, not on the first key
#  endif
#endif
}

void InputListener::press( LogicalKey key, bool down ) {
  if ( key == LogicalKey::NOTHING || key >= LogicalKey::COUNT ) return;
  auto& n = _sourcesDown[static_cast<std::size_t>( key )];
  if ( down ? n++ != 0 : ( n == 0 || --n != 0 ) ) return;  // not the first source down / last source up
  _edges.push_back( { key, down } );
}

void InputListener::listen() {
  switch ( _backend ) {
    case Backend::WINDOW: break;  // PixelRenderer already pumped SDL's events into press()
#if defined(BICYCLE_RENDER_ASCII)
#  if defined(_WIN32)
    case Backend::CONSOLE: pollConsole(); break;
    case Backend::ASYNC_KEYS: pollAsyncKeys(); break;
#  else
    case Backend::EVDEV: pollEvdev(); break;
    case Backend::TERMINAL: SshInput::listen(); break;
#  endif
#endif
    default: break;
  }
  // We don't have to know who has focus. Knowing focus is Lua-side's responsibility.
  Bitset held{};
  for ( std::size_t k = 0; k < _sourcesDown.size(); ++k ) held.set( k, _sourcesDown[k] != 0 );
  for ( const auto& e : _edges ) {
    Input in{};
    in.lastPressed = e.key;
    in.pressed = e.down;
    in.currKeysPressed = held;
    Trigger::sendInput( in );
  }
  _edges.clear();
}

#if defined(_WIN32)
#  include <tlhelp32.h>

// Our ancestor processes (shell, terminal, VS Code ...), found once. The terminal's window belongs to one of them.
static auto ancestorPids() -> std::vector<DWORD> {
  std::vector<std::pair<DWORD, DWORD>> parentOf;  // { pid, parent pid }
  HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if ( snap != INVALID_HANDLE_VALUE ) {
    PROCESSENTRY32W e{ sizeof e };
    for ( BOOL ok = Process32FirstW( snap, &e ); ok; ok = Process32NextW( snap, &e ) )
      parentOf.emplace_back( e.th32ProcessID, e.th32ParentProcessID );
    CloseHandle( snap );
  }
  std::vector<DWORD> chain{ GetCurrentProcessId() };
  for ( bool found = true; found && chain.size() < 64; ) {
    found = false;
    for ( const auto& [pid, ppid] : parentOf ) {
      if ( pid != chain.back() || ppid == 0 ) continue;
      for ( DWORD c : chain ) if ( c == ppid ) return chain;  // pid reuse loop
      chain.push_back( ppid );
      found = true;
      break;
    }
  }
  return chain;
}

// Does the foreground window belong to our terminal? A classic console window is GetConsoleWindow(). Under ConPTY
// (Windows Terminal, VS Code) that's a hidden pseudo window, so accept a window owned by an ancestor, or the window
// that was in front when our console last got a key (a terminal that isn't our ancestor, e.g. Windows Terminal
// opened by default-terminal handoff).
auto InputListener::terminalHasFocus() -> bool {
  HWND fg = GetForegroundWindow();
  if ( !fg ) return false;
  if ( fg == GetConsoleWindow() || fg == _typedWindow ) return true;
  DWORD pid{};
  GetWindowThreadProcessId( fg, &pid );
  for ( DWORD a : _ancestors ) if ( a == pid ) return true;
  return false;
}

// Console key records only say a key was typed: ConPTY sends each one as a down+up pair and auto-repeats only the
// last key held. So drain them (the buffer must not fill; a key there also tells us which window is our terminal)
// and take the held keys from the keyboard itself.
void InputListener::pollConsole() {
  DWORD n{};
  while ( GetNumberOfConsoleInputEvents( _conin, &n ) && n > 0 ) {
    INPUT_RECORD rec[32];
    DWORD got{};
    if ( !ReadConsoleInputW( _conin, rec, 32, &got ) || got == 0 ) break;
    for ( DWORD i = 0; i < got; ++i )
      if ( rec[i].EventType == KEY_EVENT && rec[i].Event.KeyEvent.bKeyDown ) _typedWindow = GetForegroundWindow();
  }
  pollAsyncKeys();
}

// The real keyboard: true multi-key state and exact releases. Only while our terminal has focus, so typing in
// another app doesn't drive the game; losing focus releases everything.
void InputListener::pollAsyncKeys() {
  if ( _ancestors.empty() ) _ancestors = ancestorPids();
  const bool focus = terminalHasFocus();
  for ( int vk = 1; vk < 256; ++vk ) {
    const LogicalKey lk = nativeToLogical( vk );
    if ( lk == LogicalKey::COUNT ) continue;
    const bool down = focus && ( GetAsyncKeyState( vk ) & 0x8000 ) != 0;
    if ( down == _vkDown[vk] ) continue;
    _vkDown[vk] = down;
    press( lk, down );
  }
}

// Windows virtual-key codes. Arrows drive the same keys as WASD, as in SDL mode.
auto InputListener::nativeToLogical( int code ) -> LogicalKey {
  if ( code >= 'A' && code <= 'Z' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::A ) + ( code - 'A' ) );
  if ( code >= '1' && code <= '9' ) return static_cast<LogicalKey>( static_cast<int>( LogicalKey::Key1 ) + ( code - '1' ) );
  switch ( code ) {
    case '0':         return LogicalKey::Key0;
    case VK_RETURN:   return LogicalKey::Enter;
    case VK_ESCAPE:   return LogicalKey::Escape;
    case VK_TAB:      return LogicalKey::Tab;
    case VK_SPACE:    return LogicalKey::Space;
    case VK_LCONTROL: return LogicalKey::LeftCtrl;
    case VK_RCONTROL: return LogicalKey::RightCtrl;
    case VK_LSHIFT:   return LogicalKey::LeftShift;
    case VK_RSHIFT:   return LogicalKey::RightShift;
    case VK_LMENU:    return LogicalKey::LeftAlt;
    case VK_RMENU:    return LogicalKey::RightAlt;
    case VK_LWIN:     return LogicalKey::LeftMeta;
    case VK_RWIN:     return LogicalKey::RightMeta;
    case VK_UP:       return LogicalKey::W;
    case VK_LEFT:     return LogicalKey::A;
    case VK_DOWN:     return LogicalKey::S;
    case VK_RIGHT:    return LogicalKey::D;
    default:          return LogicalKey::COUNT;
  }
}

#else  // Linux

// Keyboards are the event devices that have letter or Enter keys (this holds even through KVM switches).
void InputListener::openEvdevKeyboards() {
#if defined(BICYCLE_RENDER_ASCII)
  DIR* dir = opendir( "/dev/input" );
  if ( !dir ) return;
  while ( dirent* ent = readdir( dir ) ) {
    if ( std::string( ent->d_name ).rfind( "event", 0 ) != 0 ) continue;
    const std::string path = "/dev/input/" + std::string( ent->d_name );
    const int fd = open( path.c_str(), O_RDONLY | O_NONBLOCK );
    if ( fd < 0 ) continue;  // no permission: add yourself to the "input" group
    libevdev* dev = nullptr;
    if ( libevdev_new_from_fd( fd, &dev ) == 0 &&
         ( libevdev_has_event_code( dev, EV_KEY, KEY_A ) || libevdev_has_event_code( dev, EV_KEY, KEY_ENTER ) ) ) {
      std::cerr << "Keyboard listener attached: " << libevdev_get_name( dev ) << " -> " << path << "\n";
      _evdevFds.push_back( fd );
    }
    else close( fd );
    if ( dev ) libevdev_free( dev );
  }
  closedir( dir );
#endif
}

void InputListener::pollEvdev() {
  constexpr int VAL_KEY_UP{ 0 }, VAL_KEY_DOWN{ 1 };  // 2 is auto-repeat
  for ( const int fd : _evdevFds ) {
    input_event ev{};
    while ( read( fd, &ev, sizeof ev ) == sizeof ev ) {
      if ( ev.type == EV_KEY && ( ev.value == VAL_KEY_DOWN || ev.value == VAL_KEY_UP ) ) {
        press( nativeToLogical( ev.code ), ev.value == VAL_KEY_DOWN );
      }
    }
  }
}

// Linux evdev / input-event-codes.h uses its own set (often close to USB HID)
auto InputListener::nativeToLogical( int code ) -> LogicalKey {
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
