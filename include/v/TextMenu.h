#include "v/Window.h"
#include "v/Menu.h"

class TextMenu : Menu {
  public:
    TextMenu( const std::string& menuName );
    void render() override;
  private:
    int _firstDispIdx{};
};

