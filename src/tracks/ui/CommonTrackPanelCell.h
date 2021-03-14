/**********************************************************************

Audacity: A Digital Audio Editor

CommonTrackPanelCell.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_COMMON_TRACK_PANEL_CELL__
#define __AUDACITY_COMMON_TRACK_PANEL_CELL__


#include "../../TrackPanelCell.h"
#include "../../TrackAttachment.h" // to inherit

#include <stdlib.h>
#include <memory>
#include <functional>

class Track;
class XMLWriter;

class AUDACITY_DLL_API CommonTrackPanelCell /* not final */
   : public TrackPanelCell
{
public:
   // Type of function to dispatch mouse wheel events
   using Hook = std::function<
      unsigned(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
   >;
   // Install a dispatcher function, returning the previous function
   static Hook InstallMouseWheelHook( const Hook &hook );

   CommonTrackPanelCell()
   {}

   virtual ~CommonTrackPanelCell() = 0;

   // Default to the arrow cursor
   HitTestPreview DefaultPreview
      (const TrackPanelMouseState &, const AudacityProject *) override;

   std::shared_ptr<Track> FindTrack() { return DoFindTrack(); }
   std::shared_ptr<const Track> FindTrack() const
      { return const_cast<CommonTrackPanelCell*>(this)->DoFindTrack(); }

protected:
   virtual std::shared_ptr<Track> DoFindTrack() = 0;

   unsigned HandleWheelRotation
      (const TrackPanelMouseEvent &event,
      AudacityProject *pProject) override;

};

class AUDACITY_DLL_API CommonTrackCell /* not final */
   : public CommonTrackPanelCell, public TrackAttachment
{
public:
   explicit CommonTrackCell( const std::shared_ptr<Track> &pTrack );

  ~CommonTrackCell();

   std::shared_ptr<Track> DoFindTrack() override;
   void Reparent( const std::shared_ptr<Track> &parent ) override;

private:
   std::weak_ptr< Track > mwTrack;
};

// fix this
#include "AttachedVirtualFunction.h"
class TrackView;
class AUDACITY_DLL_API TrackAffordanceControls
   : public CommonTrackCell
   , public std::enable_shared_from_this<TrackAffordanceControls>
{
public:
   using CommonTrackCell::CommonTrackCell;
   static TrackAffordanceControls &Get( TrackView &trackView );
};

struct DoGetAffordanceControlsTag;

using DoGetAffordanceControls =
AttachedVirtualFunction<
DoGetAffordanceControlsTag,
   std::shared_ptr< TrackAffordanceControls >,
   TrackView
>;
DECLARE_EXPORTED_ATTACHED_VIRTUAL(AUDACITY_DLL_API, DoGetAffordanceControls);

#endif
