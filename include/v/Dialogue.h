#pragma once
#include <string>
#include <vector>
#include "Window.h"

// Variables
struct LineLimits {
  int start{}, len{};  // i just KNOW the len is gonna be mistaken for end at some point
};

class Dialogue : public Window {
  public:
    Dialogue( const std::string& s );
    Dialogue( const std::string&& s );
    Dialogue( const std::string& s, const int x, const int y );
    Dialogue( const std::string& s, const int x, const int y, const int w, const int h );
    Dialogue( const std::string&& s, const int x, const int y, const int w, const int h );
    // Public functions
    void update() override;
    void react( const int input ) override;
  private:
    // Private members
    std::string _content;
    std::vector<LineLimits> lineLimits{};
    unsigned initLineNum{};
    // Private functions
    void init();
    void delimitLines();
};
