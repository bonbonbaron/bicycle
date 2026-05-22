#include "c/SshInput.h"

using namespace std;

struct termios origTerm;

public: 
SshInput() {
  tcgetattr( 0, &origTerm );
  struct termios raw = origTerm;
  raw.c_lflag &= ~( ICANON | ECHO );
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr( 0, TCSANOW, &raw );
}

~SshInput() {
  tcsetattr( 0, TCSANOW, &origTerm );
}

auto getKey() -> int {
  char buffer[8] = {};
  if ( read( 0, buffer, 1 ) != 1 ) {
    return -1;
  }
  return buffer[0];
}
};

int listen() {
SshInput kf;
while ( true ) {
  auto key = kf.getKey();
  cout << "you pressed " << key << ".\n";
  if ( key == 'q' ) {
    break;
  }
}
return 0;
}


#include <termios.h>
#include <unistd.h>
#include <iostream>
#include "c/LogicalKey.h"

using namespace std;

class SshInput {
struct termios origTerm;

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

auto SshInput::getKey() -> int {
  char buffer[8] = {};
  if ( read( 0, buffer, 1 ) != 1 ) {
    return -1;
  }
  return buffer[0];
}
};

int SshInput::listen() {
  SshInput kf;
  while ( true ) {
    auto key = kf.getKey();
    cout << "you pressed " << key << ".\n";
    if ( key == 'q' ) {
      break;
    }
  }
}

