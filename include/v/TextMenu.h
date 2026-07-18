#include "v/Menu.h"
#include "c/Input.h"

class TextMenu : public Menu {
  public:
    TextMenu(  
      const int x,
      const int y,
      const int w,
      const int h);
    void render() override;
    void onInput( Input& input ) override;
  private:
    int _firstDispIdx{};
};

