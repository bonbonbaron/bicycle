#if defined(__linux__)

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

class KeyboardListener {
  int fd_ = -1;
  public:
  void platform_open_evdev(const char* device = "/dev/input/event0") // or find the right one
  {
    fd_ = open(device, O_RDONLY | O_NONBLOCK);
    // usually needs to be in input group or run as root
  }

  void platform_pump() {
    if (fd_ < 0) {
      return;
    }

    struct pollfd pfd = {fd_, POLLIN, 0};
    input_event ev{};
    while (poll(&pfd, 1, 0) > 0) {
      if (read(fd_, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
          LogicalKey lk = linux::evdev_to_logical(ev.code);
          if (lk != LogicalKey::COUNT) {
            push_event(lk, ev.value != 0);   // 1=down, 0=up, 2=repeat
          }
        }
      }
    }
  }
};

#endif
