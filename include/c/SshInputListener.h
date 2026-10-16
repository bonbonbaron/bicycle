#ifdef _WIN32
#include <conio.h>  // _kbhit(), _getch(): unbuffered, unechoed console keys
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <iostream>
#include "c/Input.h"

using namespace std;

class SshInput {
  public:
    static auto getInstance() -> SshInput&;
    static void listen();   // this is called by the SshInput instance itself
    void _listen();   // this is called by the SshInput instance itself
    auto getState() const -> const Input&;
  private:
    SshInput();
    SshInput(const SshInput&) = delete;
    SshInput operator=(const SshInput&) = delete;
    SshInput(const SshInput&&) = delete;
    SshInput operator=(const SshInput&&) = delete;
    // Most importantly...
    Input _inputState{};
#ifndef _WIN32
    struct termios origTerm;  // let's us restore terminal to its original state before starting the game in dtor
#endif
    auto convertCodeToLogicalInt(int code) -> LogicalKey;
};

