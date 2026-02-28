#pragma once
#include <memory>

class Bar {
  public:
    Bar( 
        std::shared_ptr<unsigned> val, 
        const unsigned max, 
        const int x, 
        const int y, 
        const int len );
    void draw() const;
    void setPos( const int x, const int y);
  private:
    std::shared_ptr<unsigned> _curr{};
    unsigned _max{};
    int _x{};
    int _y{};
    int _len{};
    // constants
    const int ONE_THIRD_MAX_BAR_LENGTH{ _len / 3};  // red threshold
    const int TWO_THIRDS_BAR_LENGTH{ static_cast<int>(2 * ONE_THIRD_MAX_BAR_LENGTH) }; // yellow threshold
};

