#include <termios.h>
#include <unistd.h>
#include <iostream>
#include "c/LogicalKey.h"

using namespace std;

class SshInput {
  struct termios origTerm;

  public: 
  SshInput();
  ~SshInput();
  auto getKey() -> int;
  auto listen() -> int;
};

