#include "v/Menu.h"
#include "c/InputData.h"

class TextMenu : public Menu {
  public:
    TextMenu(  
      const std::string& menuName, 
      const std::vector<Menu::MenuItem>& items,
      const int x,
      const int y,
      const int w,
      const int h);
    void render() override;
    void onInput( const InputState& input ) override;
  private:
    void onCursorMovement() override;
    int _firstDispIdx{};
};

