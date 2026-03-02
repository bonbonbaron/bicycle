#include "c/Input.h"

// ────────────────────────────────────────────────
//  Logical keys – our unified, cross-platform key namespace
//  Based on USB HID Usage IDs (Keyboard page 0x07)
// ────────────────────────────────────────────────

// Get singleton
auto Input::getInstance() -> Input& {
  static Input kbListener;
  return kbListener;
}
void Input::discoverPotentialKeyboards()
{
  // close_all();

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

// Constructor
Input::Input() {
  discoverPotentialKeyboards();
}

void Input::listen() {
  auto& kbl = Input::getInstance();
  kbl._listen();
}

// Check dev file for keyboard events
void Input::_listen() {
  // Constants
  constexpr unsigned VAL_KEY_UP{0};
  constexpr unsigned VAL_KEY_DOWN{1};
  constexpr unsigned VAL_KEY_REPEAT{2};

  if (_devices.empty()) {
    std::cout << "devices is empty\n";
    // Optional: re-scan every few seconds if nothing is connected yet
    static int counter = 0;
    if (++counter % 300 == 0) {
      //reopenIfNeededi(;  // ~5s at 60fps
    }
    return;
  }

  // Check for key-up-or-down events.
  auto oldKeystate = _keyState;
  input_event ev{};
  for ( unsigned i = 0; i < _devices.size(); ++i ) {
    input_event ev{};
    // power through all the events currently queued on this device
    while (read(_devices.at(i).fd, &ev, sizeof(ev)) == sizeof(ev)) {
      // If it's not a key event, we're not interested.
      if (ev.type != EV_KEY || ev.value == VAL_KEY_REPEAT ) {  // TODO what about mouse movements? Is there one for that?
        continue;
      }
      // Get key's index in bitset.
      auto lk = convertCodeToLogicalInt(ev.code);
      // Skip if we don't support this key.
      if (lk == LogicalKey::COUNT) {
        continue;
      }
      if ( ev.value == VAL_KEY_DOWN ) {
        _keyState.set( static_cast<size_t>(lk), true );
        std::cout << "down on " << ev.code << '\n';
      }
      if ( ev.value == VAL_KEY_UP ) {
        _keyState.set( static_cast<size_t>(lk), false );
        std::cout << "up on " << ev.code << '\n';
      }
    }
  }
  // Send new events, bundled up in a single bitset, to activity system.
  if ( _keyState != oldKeystate ) {
    // We don't have to know who has focus. Activity should know.
    // TODO activity.onInput( _keyState );  
  }
}

// ────────────────────────────────────────────────
// Platform implementations (mapping native → LogicalKey)
// ────────────────────────────────────────────────


// Linux evdev / input-event-codes.h uses its own set (often close to USB HID)
auto Input::convertCodeToLogicalInt(int code) -> LogicalKey {
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
    case KEY_7: return LogicalKey::Key2;
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

    default: return LogicalKey::COUNT;
  }
}  // Native key to LogicalKey converter
