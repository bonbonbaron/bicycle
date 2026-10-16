#pragma once
#include <string>
#include <vector>
#include "v/common/Window.h"
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
    void onTermResize() override;  // a bottom-anchored dialogue stays full width at the bottom; text rewraps
    void onInput( Input& input ) override;
    auto isDone() const -> bool;  // E or Space was pressed on the last page
  private:
    // Private members
    std::string _content;
    std::vector<LineLimits> lineLimits{};
    unsigned initLineNum{};
    bool _done{};
    bool _bottomAnchored{};
    int _wantH{};  // the height it was created with (a short terminal may squeeze it for a while)  // spanned the bottom of the screen when created (Dialogue.new's default)
    // Private functions
    void init();
    void delimitLines();
    auto getTextRows() const -> unsigned;
    auto lastPageStart() const -> unsigned;
    auto isOnLastPage() const -> bool;
};
