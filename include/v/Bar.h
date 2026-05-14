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
    unsigned int _x{};
    unsigned int _y{};
    unsigned int _len{};
    // constants
    const unsigned int ONE_THIRD_MAX_BAR_LENGTH{ _len / 3};  // red threshold
    const unsigned int TWO_THIRDS_BAR_LENGTH{ 2 * ONE_THIRD_MAX_BAR_LENGTH }; // yellow threshold
};

