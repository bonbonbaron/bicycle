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