/**********************************************************************

  Audacity: A Digital Audio Editor

  ComboBox.cpp

**********************************************************************/

#include "ComboBox.h"

#include "../AllThemeResources.h"
#include "../Theme.h"

#if wxUSE_ACCESSIBILITY
#include "WindowAccessible.h"
#endif

#include <wx/wx.h>
#include <wx/odcombo.h>
#include <wx/tooltip.h>

class ComboBoxPopup : public wxVListBoxComboPopup
{
public:
   bool AcceptsFocus() const override
   {
      return true;
   }

   void SetFocus() override
   {
      wxVListBox::SetFocus();
   }
};

ComboBox::ComboBox(wxWindow *parent,
                   wxWindowID id,
                   const wxString &value,
                   const wxPoint &pos,
                   const wxSize &size,
                   int n,
                   const wxString choices[],
                   long style)
:  wxOwnerDrawnComboBox(parent, id, value, pos, size, n, choices, style)
{
   // wxWidgets will guarantee destruction of the ComboBoxPopup
   SetPopupControl(safenew ComboBoxPopup());
}

ComboBox::ComboBox(wxWindow *parent,
                   wxWindowID id,
                   const wxString &value,
                   const wxPoint &pos,
                   const wxSize &size,
                   const wxArrayString &choices,
                   long style)
:  ComboBox(parent, id, value, pos, size, 0, nullptr, style)
{
   for (auto choice : choices)
   {
      Append(choice);
   }
}

void ComboBox::OnDrawItem(wxDC &dc,
                          const wxRect &rect,
                          int item,
                          int WXUNUSED(flags)) const
{
   if (item == wxNOT_FOUND)
   {
      return;
   }

//      dc.SetBackground(theTheme.Colour(clrTrackInfo));
//      dc.Clear();
   dc.SetTextForeground(theTheme.Colour(clrTrackPanelText));

   dc.DrawText(GetString(item),
               rect.x + 3,
               rect.y + ((rect.height - dc.GetCharHeight())/2)
               );
}

void ComboBox::OnDrawBackground(wxDC &dc,
                                const wxRect &rect,
                                int item,
                                int flags) const
{

   // If item is selected or even, or we are painting the
   // combo control itself, use the default rendering.
   if (flags & wxODCB_PAINTING_SELECTED)
   {
      wxOwnerDrawnComboBox::OnDrawBackground(dc, rect, item, flags);
      return;
   }

   auto color = theTheme.Colour(clrTrackInfo);
   dc.SetBrush(color);
   dc.SetPen(color);
   dc.DrawRectangle(rect);
}

wxCoord ComboBox::OnMeasureItem(size_t WXUNUSED(item)) const
{
   return -1;
}

wxCoord ComboBox::OnMeasureItemWidth(size_t WXUNUSED(item)) const
{
   return -1;
}
