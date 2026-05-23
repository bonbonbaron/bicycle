#include <termios.h>
#include <unistd.h>
#include <iostream>
#include "c/InputData.h"

using namespace std;

class SshInput {
  public:
    static auto getInstance() -> SshInput&;
    static void listen();   // this is called by the SshInput instance itself
    void _listen();   // this is called by the SshInput instance itself
    ~SshInput();
  private:
    SshInput();
    SshInput(const SshInput&) = delete;
    SshInput operator=(const SshInput&) = delete;
    SshInput(const SshInput&&) = delete;
    SshInput operator=(const SshInput&&) = delete;
    // Most importantly...
    InputState _inputState{};
    struct termios origTerm;  // let's us restore terminal to its original state before starting the game in dtor
    auto convertCodeToLogicalInt(int code) -> LogicalKey;
};

