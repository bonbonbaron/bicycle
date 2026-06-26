#include "v/Window.h"
#include "v/Menu.h"

class TextMenu : Menu {
  public:
    TextMenu( const std::string& menuName );
    void render() override;
    void onInput() override;
  private:
    void onCursorMovement() override;
    int _firstDispIdx{};
};

