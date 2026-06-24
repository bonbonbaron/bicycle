#pragma once

#include <vector>
#include <functional>
#include <memory>

#include "m/Position.h"

/* If Menu is a superclass.l.. what i need is my notes on how i was going to resolve this, or at least to try to recreate them here.
 * (I)t was along the lines of having children; menus *can* have children.
 *   There should be a choice.
 *   The choice of each menu should be passed back to the actor menu.
 *   The actor menu is not always the root level. Item menus act on the character you choose and don't kick you all the way back to the root.
 *   Battle menus completely exit out of all menus when you make your selection.
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

struct Cursor {
  Image img;
  Position offset;
  int currItemIdx{};
};

struct Selection {
  std::string menuid;
  unsigned id{};
};

class Menu {
  public:
    Menu( const std::string& menuName );
    Menu() = delete;  // ensure they pass in items.
    const MenuItem& getCurrMenuItem() const;
    void onInput( );
    void moveCursor( int amt );
    auto findChild( const std::string& childId ) -> std::shared_ptr<Menu>;

  private:
    virtual void onCursorChange();
    std::string id;
    Cursor _cursor;
    std::vector< Entity > _items{};
    Selection _selection;
    int _firstDispIdx{};
    void addItem( const MenuItem& entity );  // upon menu pop-up
    std::vector<std::shared_ptr<Menu>> _children{};
};
