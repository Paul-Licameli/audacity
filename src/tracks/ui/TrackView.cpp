/**********************************************************************

Audacity: A Digital Audio Editor

TrackView.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "TrackView.h"
#include "../../Track.h"
#include "TrackVRulerControls.h"
#include "../../AColor.h"
#include "../../TrackPanelDrawingContext.h"
#include <wx/dc.h>

#include "../../ClientData.h"
#include "../../Project.h"
#include "../../xml/XMLTagHandler.h"
#include "../../xml/XMLWriter.h"

TrackView::TrackView( const std::shared_ptr<Track> &pTrack )
   : CommonTrackCell{ pTrack }
{
   DoSetHeight( GetDefaultTrackHeight::Call( *pTrack ) );
}

TrackView::~TrackView()
{
}

int TrackView::GetTrackHeight( const Track *pTrack )
{
   return pTrack ? Get( *pTrack ).GetHeight() : 0;
}

int TrackView::GetChannelGroupHeight( const Track *pTrack )
{
   return pTrack ? TrackList::Channels( pTrack ).sum( GetTrackHeight ) : 0;
}

int TrackView::GetCumulativeHeight( const Track *pTrack )
{
   if ( !pTrack )
      return 0;
   auto &view = Get( *pTrack );
   return view.GetY() + view.GetHeight();
}

int TrackView::GetTotalHeight( const TrackList &list )
{
   return GetCumulativeHeight( *list.Any().rbegin() );
}

void TrackView::CopyTo( Track &track ) const
{
   auto &other = Get( track );

   other.mMinimized = mMinimized;

   // Let mY remain 0 -- TrackPositioner corrects it later
   other.mY = 0;
   other.mHeight = mHeight;
}

TrackView &TrackView::Get( Track &track )
{
   auto pView = std::static_pointer_cast<TrackView>( track.GetTrackView() );
   if (!pView)
      // create on demand
      track.SetTrackView( pView = DoGetView::Call( track ) );
   return *pView;
}

const TrackView &TrackView::Get( const Track &track )
{
   return Get( const_cast< Track& >( track ) );
}

TrackView *TrackView::Find( Track *pTrack )
{
   if (!pTrack)
      return nullptr;
   auto &track = *pTrack;

   auto pView = std::static_pointer_cast<TrackView>( track.GetTrackView() );
   // do not create on demand if it is null

   return pView.get();
}

const TrackView *TrackView::Find( const Track *pTrack )
{
   return Find( const_cast< Track* >( pTrack ) );
}

void TrackView::SetMinimized(bool isMinimized)
{
   // Do special changes appropriate to subclass
   DoSetMinimized(isMinimized);

   // Update positions and heights starting from the first track in the group
   auto leader = *TrackList::Channels( FindTrack().get() ).begin();
   if ( leader )
      leader->AdjustPositions();
}

void TrackView::WriteXMLAttributes( XMLWriter &xmlFile ) const
{
   xmlFile.WriteAttr(wxT("height"), GetActualHeight());
   xmlFile.WriteAttr(wxT("minimized"), GetMinimized());
}

bool TrackView::HandleXMLAttribute( const wxChar *attr, const wxChar *value )
{
   wxString strValue( value );
   long nValue;
   if (!wxStrcmp(attr, wxT("height")) &&
         XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      SetHeight(nValue);
      return true;
   }
   else if (!wxStrcmp(attr, wxT("minimized")) &&
         XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      SetMinimized(nValue != 0);
      return true;
   }
   else
      return false;
}

auto TrackView::GetSubViews( const wxRect &rect ) -> Refinement
{
   return { { rect.GetTop(), shared_from_this() } };
}

void TrackView::DoSetMinimized(bool isMinimized)
{
   mMinimized = isMinimized;
}

std::shared_ptr<TrackVRulerControls> TrackView::GetVRulerControls()
{
   if (!mpVRulerControls)
      // create on demand
      mpVRulerControls = DoGetVRulerControls();
   return mpVRulerControls;
}

std::shared_ptr<const TrackVRulerControls> TrackView::GetVRulerControls() const
{
   return const_cast< TrackView* >( this )->GetVRulerControls();
}

void TrackView::DoSetY(int y)
{
   mY = y;
}

int TrackView::GetHeight() const
{
   if ( GetMinimized() )
      return GetMinimizedHeight();

   return mHeight;
}

void TrackView::SetHeight(int h)
{
   DoSetHeight(h);
   FindTrack()->AdjustPositions();
}

void TrackView::DoSetHeight(int h)
{
   mHeight = h;
}

std::vector<UIHandlePtr> TrackView::HitTest(
   const TrackPanelMouseState &state, const AudacityProject *)
{
   return {};
}

void TrackView::Draw(
   TrackPanelDrawingContext &context, const wxRect &rect, unsigned iPass )
{
   if (iPass == 0) {
      auto &dc = context.dc;
      auto pTrack = FindTrack();
      AColor::MediumTrackInfo(&dc, pTrack && pTrack->GetSelected() );
      dc.DrawRectangle(rect);
   }
}

std::shared_ptr<TrackVRulerControls> TrackView::DoGetVRulerControls()
{
   return std::make_shared<TrackVRulerControls>(shared_from_this());
}

int TrackView::GetMinimizedHeight() const
{
   return 100;
}

namespace {

// Attach an object to each project.  It receives track list events and updates
// track Y coordinates
struct TrackPositioner final : ClientData::Base, wxEvtHandler
{
   AudacityProject &mProject;

   explicit TrackPositioner( AudacityProject &project )
      : mProject{ project }
   {
      TrackList::Get( project ).Bind(
         EVT_TRACKLIST_ADDITION, &TrackPositioner::OnUpdate, this );
      TrackList::Get( project ).Bind(
         EVT_TRACKLIST_DELETION, &TrackPositioner::OnUpdate, this );
      TrackList::Get( project ).Bind(
         EVT_TRACKLIST_PERMUTED, &TrackPositioner::OnUpdate, this );
      TrackList::Get( project ).Bind(
         EVT_TRACKLIST_RESIZING, &TrackPositioner::OnUpdate, this );
   }
   TrackPositioner( const TrackPositioner & ) PROHIBITED;
   TrackPositioner &operator=( const TrackPositioner & ) PROHIBITED;

   void OnUpdate( TrackListEvent & e )
   {
      e.Skip();

      auto iter =
         TrackList::Get( mProject ).Find( e.mpTrack.lock().get() );
      if ( !*iter )
         return;

      auto prev = iter;
      auto yy = TrackView::GetCumulativeHeight( *--prev );

      while( auto pTrack = *iter ) {
         auto &view = TrackView::Get( *pTrack );
         view.SetY( yy );
         yy += view.GetHeight();
         ++iter;
      }
   }
};

static const AudacityProject::AttachedObjects::RegisteredFactory key{
  []( AudacityProject &project ){
     return std::make_shared< TrackPositioner >( project );
   }
};

}

DEFINE_ATTACHED_VIRTUAL(DoGetView) {
   return [](Track &track){
      return std::make_shared<TrackView>(track.shared_from_this());
   };
}

DEFINE_ATTACHED_VIRTUAL(GetDefaultTrackHeight) {
   return [](auto &track){
      return 100;
   };
}
