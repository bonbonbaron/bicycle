#pragma once
#include <string>
#include "v/Window.h"

// A labeled meter (health, stamina, a progress bar...) drawn glued to a window, i.e. in screen cells:
//   HP [#######---] 7/10
// The filled part is curr/max of len segments, rounded up so any value above 0 shows at least one.
class Bar {
  public:
    Bar( const std::string& label, const int max, const int len, const int x, const int y );
    void setValue( const int val );
    auto getValue() const -> int;
    void setPos( const int x, const int y );
    auto filledLength() const -> int;
    void draw( const Window& tgt ) const;
  private:
    std::string _label{};
    int _curr{};
    int _max{1};
    int _len{};
    int _x{};
    int _y{};
};
