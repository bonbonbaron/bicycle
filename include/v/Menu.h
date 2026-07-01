#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <variant>

#include "v/Image.h"
#include "m/Position.h"
#include "m/Entity.h"
#include "v/Window.h"

/* 
 *
 * Menu -> selection -> action? -yes-> callback upon selected (T)arget
 *                              -no--> pass selection up to parent
 *         cancel  ------------------> pass null/cancel up to parent
 *
 * (\T)>what should target data comprise of?
 *     Entity is easy
 *     Text might be an enum, but that grows unwieldy quickly.
 *     Go with Selection struct for now.
 * 
 */

class Menu : public Window {
  public:
    using MenuItemBody = std::variant<Entity, std::string>;

    struct MenuItemValue {
      std::string id;
      int value;  // this informs caller of things like Entity # or... idk
    };

    struct MenuItem {
      MenuItemValue value{};
      MenuItemBody body{};
      std::function<void()> cb{};  // could be empty if just returning
    };

    struct Selection {
      std::string menuid;  // tells us what menu this selection pertains to
      MenuItemValue itemValue{};  
    };

    auto getItem() const -> MenuItem;

    Menu( const std::string& menuName, 
      const std::vector<MenuItem>& items,
      const int x,
      const int y,
      const int w,
      const int h);

  protected:
    struct Cursor {
      Image img{};
      Position offset{};
      int currItemIdx{};
    };

    virtual void onCursorMovement() = 0;
    const MenuItem& getCurrMenuItem() const;
    void moveCursor( int amt );
    void cancel();
    std::vector< MenuItem > _items{};
    Selection _selection{};
    Cursor _cursor{ { " > ", Color::WHITE }, {0, 0}, 0 };
    std::shared_ptr<Menu> _parent{};
    void onCursorChange();
    std::string _id;
    bool _stopHere{};  // selecting an item in a descendant menu pops all parents until it reaches here if true
    void addItem( const MenuItem& entity );  // upon menu pop-up
};
