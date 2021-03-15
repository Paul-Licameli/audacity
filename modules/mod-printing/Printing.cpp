/**********************************************************************

  Audacity: A Digital Audio Editor

  Printing.cpp

  Dominic Mazzoni

*******************************************************************//*!

\class AudacityPrintout
\brief Derived from wxPrintout, this class helps with printing.

*//*******************************************************************/


#include <wx/defs.h>
#include <wx/dc.h>
#include <wx/print.h>
#include <wx/printdlg.h>

#include "AColor.h"
#include "TrackArtist.h"
#include "ViewInfo.h"
#include "Track.h"
#include "widgets/Ruler.h"
#include "widgets/AudacityMessageBox.h"

#include "TrackPanelDrawingContext.h"

#include "TrackView.h"

#include "CommandContext.h"
#include "CommandManager.h"
#include "CommonCommandFlags.h"
#include "Project.h"
#include "ProjectWindows.h"

namespace {
// Globals, so that we remember settings from session to session
wxPrintData &gPrintData()
{
   static wxPrintData theData;
   return theData;
}

class AudacityPrintout final : public wxPrintout
{
 public:
   AudacityPrintout(wxString title,
                    TrackList *tracks,
                    AudacityProject &project, wxWindow &panel):
      wxPrintout(title),
      mTracks(tracks)
      , mProject{ project }
      , mPanel(panel)
   {
   }
   bool OnPrintPage(int page);
   bool HasPage(int page);
   bool OnBeginDocument(int startPage, int endPage);
   void GetPageInfo(int *minPage, int *maxPage,
                    int *selPageFrom, int *selPageTo);

 private:
   AudacityProject &mProject;
   wxWindow &mPanel;
   TrackList *mTracks;
};

bool AudacityPrintout::OnPrintPage(int WXUNUSED(page))
{
   wxDC *dc = GetDC();
   if (!dc)
      return false;

   int width, height;
   dc->GetSize(&width, &height);

   int rulerScreenHeight = 40;
   int screenTotalHeight =
      TrackView::GetTotalHeight( *mTracks ) + rulerScreenHeight;

   double scale = height / (double)screenTotalHeight;

   int rulerPageHeight = (int)(rulerScreenHeight * scale);
   Ruler ruler;
   ruler.SetBounds(0, 0, width, rulerPageHeight);
   ruler.SetOrientation(wxHORIZONTAL);
   ruler.SetRange(0.0, mTracks->GetEndTime());
   ruler.SetFormat(Ruler::TimeFormat);
   ruler.SetLabelEdges(true);
   ruler.Draw(*dc);

   const double screenDuration = mTracks->GetEndTime();
   ZoomInfo zoomInfo(0.0, width / screenDuration);
   SelectedRegion region{};
   TrackArtist artist( mProject, mPanel, zoomInfo, region );
   artist.SetBackgroundBrushes(*wxWHITE_BRUSH, *wxWHITE_BRUSH,
                               *wxWHITE_PEN, *wxWHITE_PEN);
   int y = rulerPageHeight;

   for (auto n : mTracks->Any()) {
      auto &view = TrackView::Get( *n );
      wxRect r;
      r.x = 0;
      r.y = 0;
      r.width = width;
      auto trackHeight = (int)(view.GetHeight() * scale);
      r.height = trackHeight;

      const auto subViews = view.GetSubViews( r );
      if (subViews.empty())
         continue;
   
      auto iter = subViews.begin(), end = subViews.end(), next = iter;
      auto yy = iter->first;
      for ( ; iter != end; iter = next ) {
         ++next;
         auto nextY = ( next == end )
            ? trackHeight
            : next->first;
         r.y = y + yy;
         r.SetHeight( nextY - yy );
         yy = nextY;

         TrackPanelDrawingContext context{
            *dc, {}, {}, &artist
         };
         iter->second->Draw( context, r, TrackArtist::PassTracks );
      }

      dc->SetPen(*wxBLACK_PEN);
      AColor::Line(*dc, 0, y, width, y);

      y += trackHeight;
   };

   return true;
}

bool AudacityPrintout::HasPage(int page)
{
   return (page==1);
}

bool AudacityPrintout::OnBeginDocument(int startPage, int endPage)
{
   return wxPrintout::OnBeginDocument(startPage, endPage);
}

void AudacityPrintout::GetPageInfo(int *minPage, int *maxPage,
                                   int *selPageFrom, int *selPageTo)
{
   *minPage = 1;
   *maxPage = 1;
   *selPageFrom = 1;
   *selPageTo = 1;
}

void HandlePageSetup(wxWindow *parent)
{
   wxPageSetupData pageSetupData;

   wxPageSetupDialog pageSetupDialog(parent, &pageSetupData);
   pageSetupDialog.ShowModal();

   gPrintData() = pageSetupDialog.GetPageSetupData().GetPrintData();
}

void HandlePrint(
   wxWindow *parent, const wxString &name, TrackList *tracks,
   AudacityProject &project, wxWindow &panel)
{
   wxPrintDialogData printDialogData(gPrintData());

   wxPrinter printer(&printDialogData);
   AudacityPrintout printout(name, tracks, project, panel);
   if (!printer.Print(parent, &printout, true)) {
      if (wxPrinter::GetLastError() == wxPRINTER_ERROR) {
         AudacityMessageBox(
            XO("There was a problem printing."),
            XO("Print"),
            wxOK);
      }
      else {
         // Do nothing, the user cancelled...
      }
   }
   else {
      gPrintData() = printer.GetPrintDialogData().GetPrintData();
   }
}

struct Handler : CommandHandlerObject {
void OnPageSetup(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = GetProjectFrame( project );
   HandlePageSetup(&window);
}

void OnPrint(const CommandContext &context)
{
   auto &project = context.project;
   auto name = project.GetProjectName();
   auto &tracks = TrackList::Get( project );
   auto &window = GetProjectFrame( project );
   HandlePrint(&window, name, &tracks, project, GetProjectPanel( project ));
}
};

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static Handler instance;
   return instance;
};

#define FN(X) (& Handler :: X)
using namespace MenuTable;
BaseItemSharedPtr PrintingItems()
{
   static BaseItemSharedPtr items{
   ( FinderScope{ findCommandHandler },
   Items( "",
      Command( wxT("PageSetup"), XXO("Pa&ge Setup..."), FN(OnPageSetup),
         AudioIONotBusyFlag() | TracksExistFlag() ),
      /* i18n-hint: (verb) It's item on a menu. */
      Command( wxT("Print"), XXO("&Print..."), FN(OnPrint),
         AudioIONotBusyFlag() | TracksExistFlag() )
   ) ) };
   return items;
}
#undef FN

AttachedItem sAttachment{ L"File/Print",
   Shared( PrintingItems() )
};

}

#include "ModuleConstants.h"
DEFINE_MODULE_ENTRIES
