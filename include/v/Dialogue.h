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
    auto isDone() const -> bool;  // E or Space was pressed on the last page
  private:
    // Private members
    std::string _content;
    std::vector<LineLimits> lineLimits{};
    unsigned initLineNum{};
    bool _done{};
    // Private functions
    void init();
    void delimitLines();
    auto getTextRows() const -> unsigned;
    auto lastPageStart() const -> unsigned;
    auto isOnLastPage() const -> bool;
};
