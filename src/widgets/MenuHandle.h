/**********************************************************************

Audacity: A Digital Audio Editor

widgets/MenuHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY__WX_MENU_WRAPPER__
#define __AUDACITY__WX_MENU_WRAPPER__

#include <functional>
#include <MemoryX.h>
#include "Internat.h"
#include "../commands/Keyboard.h"
#include <wx/weakref.h>

class wxCommandEvent;
class wxMenuItem;
class wxMenuItemList;
class wxFrame;
class wxPoint;
class wxWindow;

namespace Widgets {

// Identifies menu items
using MenuItemID = wxWindowID;
constexpr MenuItemID InvalidMenuItemID = -1;

// Describes actual or requested state of a menu item
struct MenuItemState {
   // Mask bits to specify a subset of state
   enum : unsigned {
      Enable = 0x01,
      Check = 0x02,
   };

   MenuItemState( bool enable = true, bool check = false )
      : enabled{ enable }, checked{ check }
   {}

   bool enabled;
   bool checked;
};

// Types of menu items
enum class MenuItemType {
    Separator = -1,
    Normal,
    Check,
    Radio,
    SubMenu,
};
   
// Defined below
struct MenuItem;

// Determines user-visible text on the menu button
struct MenuItemLabel {
   TranslatableLabel main;
   NormalizedKeyString accel;

   MenuItemLabel() = default;
   MenuItemLabel(
      const TranslatableLabel &main, const NormalizedKeyString &accel = {} )
      : main{ main }, accel{ accel }
   {}

   // Computes the full label text
   TranslatableLabel FullLabel() const;
};

// Full menu texts including optional help
struct MenuItemText {
   MenuItemLabel label;
   TranslatableString help;

   MenuItemText() = default;

   MenuItemText( const TranslatableLabel &label)
      // Unspecified help defaults to the same as the label
      : MenuItemText{ label, label.Stripped() }
   {}

   MenuItemText( const TranslatableLabel &label,
      const TranslatableString &help )
      : label{ label }, help{ help }
   {}

   MenuItemText( const MenuItemLabel &label )
      // Unspecified help defaults to the same as the label
      : MenuItemText{ label, label.main.Stripped() }
   {}

   MenuItemText( const MenuItemLabel &label,
      const TranslatableString &help )
      : label{ label }, help{ help }
   {}
};

// Callback to associate with a menu item
using MenuItemAction = std::function< void() >;

/**************************************************************************//**

\brief Wraps a menu so that you must supply translatable strings as you build
it; also acts as a weak reference to a menu already inserted into the user
interface, which may be rebuilt or visited with iterators.
This is a cheaply copied or moved handle to a shared menu structure.
********************************************************************************/
class MenuHandle
{
   class Menu;

   MenuHandle( Menu *pMenu );

public:

   // Default constructed with a fresh unshared menu object
   MenuHandle();

   // Constructor for a null handle
   MenuHandle( nullptr_t );

   // Copy construction produces a non-owning weak reference
   MenuHandle( const MenuHandle &other );

   // There is no copy assignment
//   MenuHandle &operator =( const MenuHandle &other ) = delete;
   MenuHandle &operator =( const MenuHandle &other ) { return *this; }

   // Move transfers the unshared ownership, if other had such, else makes
   // a weak reference only; leaves weak reference in other
   MenuHandle( MenuHandle &&other );
   MenuHandle &operator =( MenuHandle &&other );

   virtual ~MenuHandle();

   // Returns false if it doesn't (any more) point to a menu
   explicit operator bool() const { return mwMenu; }

   // Comparison
   friend inline bool operator == (
      const MenuHandle &x, const MenuHandle &y )
   { return x.mwMenu == y.mwMenu; }

   friend inline bool operator != (
      const MenuHandle &x, const MenuHandle &y )
   { return !( x == y ); }

   // Building
   // Constructs any kind of menu item except for sub-menus
   void Append( MenuItemType type,
      const MenuItemText &text = {},
      MenuItemAction action = {},
      const MenuItemState &state = {},
      MenuItemID itemid = InvalidMenuItemID );

   // Constructs an ordinary menu item
   void Append(
      const MenuItemText &text = {},
      MenuItemAction action = {},
      const MenuItemState &state = {},
      MenuItemID itemid = InvalidMenuItemID )
   { Append( MenuItemType::Normal, text, std::move( action ), state, itemid ); }

   // Constructs a menu item presenting an exclusive choice among neighboring
   // items of the same type
   void AppendRadioItem(
      const MenuItemText &text = {},
      MenuItemAction action = {},
      const MenuItemState &state = {},
      MenuItemID itemid = InvalidMenuItemID )
   { Append( MenuItemType::Radio, text, std::move( action ), state, itemid ); }

   // Constructs a menu item that may have a checkmark independently of others
   void AppendCheckItem(
      const MenuItemText &text = {},
      MenuItemAction action = {},
      const MenuItemState &state = {},
      MenuItemID itemid = InvalidMenuItemID )
   { Append( MenuItemType::Check, text, std::move( action ), state, itemid ); }

   // Makes a line between other menu items
   void AppendSeparator()
   { Append( MenuItemType::Separator ); }

   // Constructs a menu item that can open as a cascading sub-menu
   // submenu gives up unique ownership but retains a weak reference
   void AppendSubMenu( MenuHandle &&submenu,
      const MenuItemText &text = {},
      const MenuItemState &state = {} );

   // Deletes all items
   void Clear();

   // Displays the menu, invoke at most one action, then hides it
   void Popup( wxWindow &window );
   void Popup( wxWindow &window, const wxPoint &pos );

   // Iterator over existing menu items
   struct iterator : ValueIterator< MenuItem >{
      ~iterator();
      iterator( iterator && );
      iterator &operator ++ ();
      MenuItem operator * ();
      friend bool operator == ( const iterator &x, const iterator & y );
      friend inline bool operator != ( const iterator &x, const iterator & y )
      { return !( x == y ); }
   private:

      friend MenuHandle;
      iterator( const MenuHandle *p, bool begin );

      struct State;
   
      Menu *const mpMenu;
      std::unique_ptr< State > mpState;
   };

   // Support for range-for syntax
   iterator begin() const;
   iterator end() const;

   // Other item-level accessors and mutators
   MenuItemState GetState( MenuItemID itemid );
   // Mask can be a bitwise or of enum values defined in MenuItemState
   // Returns true of the item was found
   bool SetState(
      MenuItemID itemid, const MenuItemState &state, unsigned mask = ~0u );

   // Returns true of the item was found
   bool SetLabel( MenuItemID itemid, const MenuItemLabel& label);

private:
   friend class MenuBarHandle;
   std::unique_ptr< Menu > muMenu;
   wxWeakRef< Menu > mwMenu;
};

// Information accessed by iterator over menu items
struct MenuItem {
   MenuItemID id;
   MenuItemType type;
   TranslatableLabel label;
   wxString accel;
   MenuItemState state;
   MenuHandle pSubMenu;
};

// Information accessed by iterator over menu bar items
struct MenuBarItem {
   TranslatableLabel title;
   MenuHandle pSubMenu;
};

/**************************************************************************//**

\brief Wraps a menu bar so that you must supply translatable strings as you
build it; also acts as a weak reference to a menu bar already inserted into the
user interface.
This is a cheaply copied or moved handle to a shared menu bar structure.
********************************************************************************/
class MenuBarHandle
{
   class MenuBar;

public:
   // Default constructed with a fresh unshared menu bar object
   MenuBarHandle();

   // Constructor for a null
   MenuBarHandle( nullptr_t );

   // Retrive a weak reference from a frame that a menu bar was previously
   // attached to
   MenuBarHandle( wxFrame &frame );

   // Copy construction produces a non-owning weak reference
   MenuBarHandle( const MenuBarHandle &other );

   // There is no copy assignment
   MenuBarHandle &operator =( const MenuBarHandle &other ) = delete;

   // Move transfers the unshared ownership, if other had such, else makes
   // a weak reference only; leaves weak reference in other
   MenuBarHandle( MenuBarHandle &&other );
   MenuBarHandle &operator =( MenuBarHandle &&other );

   virtual ~MenuBarHandle();

   // Returns false if it doesn't (any more) point to a menu bar
   explicit operator bool() const { return mwMenuBar; }

   // menu gives up ownership but retains a weak reference
   void Append( MenuHandle &&menu, const TranslatableLabel &title );

   // macOS only
   static void MacSetCommonMenuBar( MenuBarHandle &&pMenuBar );

   // this gives up ownership but retains a weak reference
   void AttachTo( wxFrame &frame ) &&;

   // Iterator over existing drop-down menus
   struct iterator : ValueIterator< MenuBarItem >{
      ~iterator();
      iterator( iterator && );
      iterator &operator ++ ();
      MenuBarItem operator * ();
      friend bool operator == ( const iterator &x, const iterator & y );
      friend inline bool operator != ( const iterator &x, const iterator & y )
      { return !( x == y ); }
   private:

      friend MenuBarHandle;
      iterator( const MenuBarHandle *p, bool begin );

      struct State;
   
      MenuBar *const mpMenuBar;
      std::unique_ptr< State > mpState;
   };

   // Support for ragne-for syntax
   iterator begin() const;
   iterator end() const;

private:
   std::unique_ptr< MenuBar > muMenuBar;
   wxWeakRef< MenuBar > mwMenuBar;
};

} // namespace Widgets

#endif

