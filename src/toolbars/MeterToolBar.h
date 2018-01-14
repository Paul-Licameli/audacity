/**********************************************************************

  Audacity: A Digital Audio Editor

  MeterToolbar.h

  Dominic Mazzoni
  Leland Lucius

  ToolBar to hold the VU Meter

**********************************************************************/

#ifndef __AUDACITY_METER_TOOLBAR__
#define __AUDACITY_METER_TOOLBAR__

#include "ToolBar.h"
#include "../Project.h"

class wxDC;
class wxGridBagSizer;
class wxSizeEvent;
class wxWindow;

class MeterPanel;


// Constants used as bit pattern
const int kWithRecordMeter = 1;
const int kWithPlayMeter = 2;

class MeterToolBar final : public ToolBar {

 public:

   MeterToolBar(AudacityProject *project, int type);
   virtual ~MeterToolBar();

   void Create(wxWindow *parent) override;

   void Populate() override;
   void ReCreateButtons() override;
   void Repaint(wxDC * WXUNUSED(dc)) override {};
   void EnableDisableButtons() override {};
   void UpdatePrefs() override;

   void OnSize(wxSizeEvent & event);
   bool Expose(bool show) override;

   int GetInitialWidth() override {return (mWhichMeters ==
      (kWithRecordMeter + kWithPlayMeter)) ? 338 : 460;} // Separate bars used to be smaller.
   int GetMinToolbarWidth()  override { return 50; }
   wxSize GetDockedSize() override;

 private:
   void RegenerateTooltips() override;

   AudacityProject *mProject;
   int mWhichMeters;
   wxGridBagSizer *mSizer;
   MeterPanel *mPlayMeter;
   MeterPanel *mRecordMeter;

 public:

   DECLARE_CLASS(MeterToolBar)
   DECLARE_EVENT_TABLE()

};

#endif

