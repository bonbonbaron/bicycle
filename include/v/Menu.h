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
 * (G) Is there a cleaner way to chain together selections? If every menu composes of a map, that bloats quickly.
 * (D)o we want actions triggered by cursor movement too, such as showing a new character image when you iterate across a character selection menu?
 *      if we do that, trigger will forward the input back to the menu since it's the top window.
 *      In that case, menu would have to be an entity with behaviors.
 *      Might be easier to just have a onCursorMovement().
 *      That suggests there can be many custom descendants of Menu. CharacterMenu could be one.
 *
 *
 * (\T)>what should target data comprise of?
 *     Entity is easy
 *     Text might be an enum, but that grows unwieldy quickly.
 *     Go with Selection struct for now.
 * 
 * (S)hould this be a template?
 * (I) Should MenuItem give way to entity/string?
 */

class Menu : Window {
  public:
    Menu() = delete;  // ensure they pass in items.

    using MenuItemBody = std::variant<Entity, std::string>;

    struct MenuItem {
      std::string id;
      MenuItemBody body{};
      std::function<void()> cb;
    };

  protected:
    struct Cursor {
      Image img;
      Position offset;
      int currItemIdx{};
    };

    struct Selection {
      std::string menuid;
      MenuItem& item;  
    };

    virtual void onCursorMovement() = 0;
    auto findChild( const std::string& childId ) -> std::shared_ptr<Menu>;
    const MenuItem& getCurrMenuItem() const;
    void moveCursor( int amt );
    void cancel();

  private:
    std::string id;
    Cursor _cursor;
    bool _stopHere{};  // selecting an item in a descendant menu pops all parents until it reaches here if true
    std::vector< MenuItem > _items{};
    Selection _selection;
    void addItem( const MenuItem& entity );  // upon menu pop-up
    std::vector<std::shared_ptr<Window>> _children{}; // Window inclues Dialogue as potential child
    unsigned _currMenuItemIdx{};
};
