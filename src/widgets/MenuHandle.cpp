/**********************************************************************

Audacity: A Digital Audio Editor

widgets/MenuHandle.cpp

Paul Licameli

**********************************************************************/

#include "MenuHandle.h"

#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/windowptr.h>

namespace Widgets {

TranslatableString MenuItemLabel::FullLabel() const
{
   return accel.empty()
      ? main
      : TranslatableString{ main }
         .Join( Verbatim( accel.GET() ), L"\t" );
}

class MenuHandle::Menu : public wxMenu
{
public:
   using wxMenu::wxMenu;
   ~Menu() override = default;

   // Remember the unstranslated strings
   TranslatableStrings mLabels;
};

MenuHandle::MenuHandle( Menu *pMenu )
   : mwMenu{ pMenu }
{}

MenuHandle::MenuHandle()
   : muMenu{ std::make_unique< Menu >() }
   , mwMenu{ muMenu.get() }
{}

MenuHandle::MenuHandle( nullptr_t )
{}

MenuHandle::MenuHandle( const MenuHandle &other )
   : muMenu{ nullptr }
   , mwMenu{ other.mwMenu }
{}

MenuHandle::MenuHandle( MenuHandle &&other )
   : muMenu{ std::move( other.muMenu ) }
   , mwMenu{ other.mwMenu } // copy, don't move, the weak reference
{}

MenuHandle &MenuHandle::operator =( MenuHandle &&other )
{
   muMenu = std::move( other.muMenu );
   mwMenu = other.mwMenu; // copy, don't move, the weak reference
   return *this;
}

MenuHandle::~MenuHandle() = default;

namespace {
   void ApplyState(
      wxMenuItem &item, const MenuItemState &state, unsigned mask = ~0u )
   {
      if ( (mask & MenuItemState::Enable) )
         item.Enable( state.enabled );
      if ( (mask & MenuItemState::Check) &&
            item.IsCheckable() && !item.IsSeparator() )
         item.Check( state.checked );
   }

   inline wxItemKind toItemKind( MenuItemType type )
   {
      switch ( type ) {
      case MenuItemType::Separator:
         return wxITEM_SEPARATOR;
      case MenuItemType::Normal:
         return wxITEM_NORMAL;
      case MenuItemType::Check:
         return wxITEM_CHECK;
      case MenuItemType::Radio:
         return wxITEM_RADIO;
      case MenuItemType::SubMenu:
         return wxITEM_DROPDOWN;
      default:
         return static_cast< wxItemKind >( type );
      }
   }

   inline MenuItemType toItemType( wxItemKind kind )
   {
      switch ( kind ) {
      case wxITEM_SEPARATOR:
         return MenuItemType::Separator;
      case wxITEM_NORMAL:
         return MenuItemType::Normal;
      case wxITEM_CHECK:
         return MenuItemType::Check;
      case wxITEM_RADIO:
         return MenuItemType::Radio;
      case wxITEM_DROPDOWN:
         return MenuItemType::SubMenu;
      default:
         return static_cast< MenuItemType >( kind );
      }
   }
}

void MenuHandle::Append( MenuItemType type,
   const MenuItemText &text,
   MenuItemAction action,
   const MenuItemState &state,
   MenuItemID itemid )
{
   auto kind = toItemKind( type );
   if ( type == MenuItemType::Separator )
      itemid = wxID_SEPARATOR, kind = wxITEM_NORMAL;
   auto result = mwMenu->Append( itemid,
      text.label.FullLabel().Translation(), text.help.Translation(), kind );
   if (action)
      mwMenu->Bind( wxEVT_MENU,
         [action](wxCommandEvent&){ action(); },
         result->GetId() );
   ApplyState( *result, state );
   mwMenu->mLabels.push_back( text.label.main );
}

void MenuHandle::AppendSubMenu(MenuHandle &&submenu,
   const MenuItemText &text,
   const MenuItemState &state )
{
   // Beware the release!  Exception-safety...
   auto rawText = text.label.FullLabel().Translation();
   auto rawHelp = text.help.Translation();
   auto result = mwMenu->Append( 0, rawText,
      submenu.muMenu.release(), rawHelp );
   ApplyState( *result, state );
   mwMenu->mLabels.push_back( text.label.main );
}

void MenuHandle::Clear()
{
   wxMenuItemList items = mwMenu->GetMenuItems();
   for (auto end = items.end(), iter = items.begin(); iter != end;)
      mwMenu->Destroy(*iter++);
}

void MenuHandle::Popup( wxWindow &window )
{
   Popup( window, {} );
}

void MenuHandle::Popup( wxWindow &window, const wxPoint &pos )
{
   window.PopupMenu(mwMenu, pos);
}

auto MenuHandle::begin() const -> iterator
{
   return { this, true };
}

auto MenuHandle::end() const -> iterator
{
   return { this, false };
}

MenuItemState MenuHandle::GetState( MenuItemID itemid )
{
   return { mwMenu->IsEnabled( itemid ), mwMenu->IsChecked( itemid ) };
}

bool MenuHandle::SetState(
   MenuItemID itemid, const MenuItemState &state, unsigned mask )
{
   const auto pItem = mwMenu->FindItem( itemid );
   if ( pItem ) {
      ApplyState( *pItem, state, mask );
      return true;
   }
   else
      return false;
}

bool MenuHandle::SetLabel( MenuItemID itemid, const MenuItemLabel& label)
{
   const auto pItem = mwMenu->FindItem( itemid );
   if ( pItem ) {
      pItem->SetItemLabel( label.FullLabel().Translation() );
      return true;
   }
   else
      return false;
}

struct MenuHandle::iterator::State {
   using iterator = wxMenuItemList::const_iterator;
   iterator mIterator;
   size_t mIndex = 0;
   State( iterator iter ) : mIterator{ iter } {}
};

MenuHandle::iterator::iterator( const MenuHandle *p, bool begin )
: mpMenu{ p->mwMenu }
, mpState{ std::make_unique<State>( begin
    ? mpMenu->GetMenuItems().begin()
    : mpMenu->GetMenuItems().end() ) }
{}

MenuHandle::iterator::iterator( iterator && ) = default;

MenuHandle::iterator::~iterator() = default;

auto MenuHandle::iterator::operator++() -> iterator &
{
   ++mpState->mIterator;
   ++mpState->mIndex;
   return *this;
}

MenuItem MenuHandle::iterator::operator*()
{
   auto &item = **mpState->mIterator;
   MenuItemID itemid = item.GetId();
   MenuItemType type = toItemType( item.GetKind() );
//   auto label = item.GetItemLabelText();
   auto label = mpMenu->mLabels[ mpState->mIndex ];
   auto accel = item.GetItemLabel();
   if( accel.Contains("\t") )
      accel = accel.AfterLast('\t');
   else
      accel = "";
   MenuItemState state{ item.IsEnabled(), item.IsChecked() };

   return { itemid, type, label, accel, state,
      { static_cast< Menu* >( item.GetSubMenu() ) } };
}

bool operator == (
   const MenuHandle::iterator &x, const MenuHandle::iterator & y )
{ return &x.mpMenu == &y.mpMenu && x.mpState->mIterator == y.mpState->mIterator; }


class MenuBarHandle::MenuBar : public wxMenuBar
{
public:
   using wxMenuBar::wxMenuBar;
   ~MenuBar() override = default;

   // Remember the unstranslated strings
   TranslatableStrings mTitles;
};

MenuBarHandle::MenuBarHandle()
   : muMenuBar{ std::make_unique< MenuBar >() }
   , mwMenuBar{ muMenuBar.get() }
{}

MenuBarHandle::MenuBarHandle( nullptr_t )
{}

MenuBarHandle::MenuBarHandle( wxFrame &frame )
   : mwMenuBar{ dynamic_cast<MenuBar*>( frame.GetMenuBar() ) }
{}

MenuBarHandle::MenuBarHandle( const MenuBarHandle &other )
   : muMenuBar{ nullptr }
   , mwMenuBar{ other.mwMenuBar }
{}

MenuBarHandle::MenuBarHandle( MenuBarHandle &&other )
   : muMenuBar{ std::move( other.muMenuBar ) }
   , mwMenuBar{ other.mwMenuBar } // copy, don't move, the weak reference
{}

MenuBarHandle &MenuBarHandle::operator =( MenuBarHandle &&other )
{
   muMenuBar = std::move( other.muMenuBar );
   mwMenuBar = other.mwMenuBar; // copy, don't move, the weak reference
   return *this;
}

MenuBarHandle::~MenuBarHandle() = default;

void MenuBarHandle::Append(
   MenuHandle &&menu, const TranslatableString &title )
{
   mwMenuBar->Append(
      std::move( menu.muMenu.release() ), title.Translation() );
   mwMenuBar->mTitles.push_back( title );
}

#ifdef __WXMAC__
void MenuBarHandle::MacSetCommonMenuBar(
   MenuBarHandle &&pMenuBar )
{
   wxMenuBar::MacSetCommonMenuBar( pMenuBar.muMenuBar.release() );
}
#endif

namespace {
// Get hackcess to a protected method
class wxFrameEx : public wxFrame
{
public:
   using wxFrame::DetachMenuBar;
};
}

void MenuBarHandle::AttachTo( wxFrame &frame ) &&
{
   // Delete the menus, since we will soon recreate them.
   // Rather oddly, the menus don't vanish as a result of doing this.
   auto &window = static_cast<wxFrameEx&>( frame );
   auto oldMenuBar = window.GetMenuBar();
   if ( oldMenuBar != muMenuBar.get() ) {
      window.DetachMenuBar();
      wxWindowPtr<wxMenuBar>{ oldMenuBar };
      window.SetMenuBar( muMenuBar.release() );
      // oldMenuBar gets deleted here
   }
}

auto MenuBarHandle::begin() const -> iterator
{
   return { this, true };
}

auto MenuBarHandle::end() const -> iterator
{
   return { this, false };
}

struct MenuBarHandle::iterator::State {
   size_t mIndex;
   State( size_t ii ) : mIndex{ ii } {}
};

MenuBarHandle::iterator::iterator( const MenuBarHandle *p, bool begin )
: mpMenuBar{ p->mwMenuBar }
, mpState{ std::make_unique<State>(
   begin ? size_t(0) : mpMenuBar->GetMenuCount() ) }
{}

MenuBarHandle::iterator::iterator( iterator && ) = default;

MenuBarHandle::iterator::~iterator() = default;

auto MenuBarHandle::iterator::operator++() -> iterator &
{
   ++mpState->mIndex;
   return *this;
}

MenuBarItem MenuBarHandle::iterator::operator*()
{
   auto title = mpMenuBar->mTitles[ mpState->mIndex ];
   return { title,
      { static_cast< MenuHandle::Menu* >(
         mpMenuBar->GetMenu( mpState->mIndex ) ) } };
}

bool operator == (
   const MenuBarHandle::iterator &x, const MenuBarHandle::iterator & y )
{ return &x.mpMenuBar == &y.mpMenuBar &&
   x.mpState->mIndex == y.mpState->mIndex; }

} // namespace Widgets
