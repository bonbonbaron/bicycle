#include <termios.h>
#include <unistd.h>
#include <iostream>
#include "c/InputData.h"

using namespace std;

class SshInput {
  public:
    static auto getInstance() -> SshInput&;
    int listen();   // this is called by the SshInput instance itself
    ~SshInput();
  private:
    SshInput();
    SshInput(const SshInput&) = delete;
    SshInput operator=(const SshInput&) = delete;
    SshInput(const SshInput&&) = delete;
    SshInput operator=(const SshInput&&) = delete;

    auto convertCodeToLogicalInt(int code) -> LogicalKey;

    // Most importantly...
    InputState _inputState{};
    struct termios origTerm;
};

