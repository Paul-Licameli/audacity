/**********************************************************************

  Audacity: A Digital Audio Editor

  ComboBox.h

**********************************************************************/

#ifndef __AUDACITY_COMBOBOX__
#define __AUDACITY_COMBOBOX__

#include <wx/odcombo.h>

class ComboBox : public wxOwnerDrawnComboBox
{
public:
   ComboBox(wxWindow *parent,
            wxWindowID id,
            const wxString &value = wxEmptyString,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            int n = 0,
            const wxString choices[] = nullptr,
            long style = 0);

   ComboBox(wxWindow *parent,
            wxWindowID id,
            const wxString &value = wxEmptyString,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            const wxArrayString &choices = {},
            long style = 0);

   virtual void OnDrawItem(wxDC &dc,
                           const wxRect &rect,
                           int item,
                           int flags) const override;

   virtual void OnDrawBackground(wxDC& dc,
                                 const wxRect& rect,
                                 int item,
                                 int flags) const override;

   virtual wxCoord OnMeasureItem(size_t item) const override;

   virtual wxCoord OnMeasureItemWidth(size_t item) const override;
};

#endif

