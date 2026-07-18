#pragma once
#include <string>
#include <vector>
#include "Window.h"
#include "LineLimits.h"

class Dialogue : public Window {
  public:
    Dialogue( const std::string& s );
    Dialogue( const std::string&& s );
    Dialogue( const std::string& s, const int x, const int y );
    Dialogue( const std::string& s, const int x, const int y, const int w, const int h );
    Dialogue( const std::string&& s, const int x, const int y, const int w, const int h );
    // Public functions
    void render() override;
    void onInput( Input& input ) override;
  private:
    // Private members
    std::string _content;
    std::vector<LineLimits> lineLimits{};
    unsigned initLineNum{};
    // Private functions
    void init();
    void delimitLines();
};
