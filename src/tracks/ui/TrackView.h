/**********************************************************************

Audacity: A Digital Audio Editor

TrackView.h

Paul Licameli split from class Track

**********************************************************************/

#ifndef __AUDACITY_TRACK_VIEW__
#define __AUDACITY_TRACK_VIEW__

#include <memory>
#include "CommonTrackPanelCell.h" // to inherit
#include "ClientData.h" // to inherit

class Track;
class TrackList;
class TrackPanelResizerCell;

class TrackView;
using AttachedTrackViewCells = ClientData::Site<
   TrackView, TrackPanelCell, ClientData::SkipCopying,
   std::shared_ptr
>;

class AUDACITY_DLL_API TrackView /* not final */ : public CommonTrackCell
   , public std::enable_shared_from_this<TrackView>
   , public AttachedTrackViewCells
{
   TrackView( const TrackView& ) = delete;
   TrackView &operator=( const TrackView& ) = delete;

public:
   enum : unsigned { DefaultHeight = 150 };

   explicit
   TrackView( const std::shared_ptr<Track> &pTrack );
   virtual ~TrackView();

   // some static conveniences, useful for summation over track iterator
   // ranges
   static int GetTrackHeight( const Track *pTrack );
   static int GetChannelGroupHeight( const Track *pTrack );
   // Total height of the given track and all previous ones (constant time!)
   static int GetCumulativeHeight( const Track *pTrack );
   static int GetTotalHeight( const TrackList &list );

   // Copy view state, for undo/redo purposes
   void CopyTo( Track &track ) const override;

   static TrackView &Get( Track & );
   static const TrackView &Get( const Track & );
   static TrackView *Find( Track * );
   static const TrackView *Find( const Track * );

   bool GetMinimized() const { return mMinimized; }
   void SetMinimized( bool minimized );

   int GetY() const { return mY; }
   int GetActualHeight() const { return mHeight; }
   virtual int GetMinimizedHeight() const;
   int GetHeight() const;

   void SetY(int y) { DoSetY( y ); }
   void SetHeight(int height);

   void WriteXMLAttributes( XMLWriter & ) const override;
   bool HandleXMLAttribute( const wxChar *attr, const wxChar *value ) override;

   // New virtual function.  The default just returns a one-element array
   // containing this.  Overrides might refine the Y axis.
   using Refinement = std::vector< std::pair<
      wxCoord, std::shared_ptr< TrackView >
   > >;
   virtual Refinement GetSubViews( const wxRect &rect );

   virtual void DoSetMinimized( bool isMinimized );

   //! Returns no hits
   std::vector<UIHandlePtr> HitTest
      (const TrackPanelMouseState &state,
       const AudacityProject *) override;

   //! Paints a blank rectangle
   void Draw(
      TrackPanelDrawingContext &context, const wxRect &rect, unsigned iPass )
      override;

protected:

   // No need yet to make this virtual
   void DoSetY(int y);

   void DoSetHeight(int h);

private:
   bool           mMinimized{ false };
   int            mY{ 0 };
   int            mHeight{ DefaultHeight };
};

#include "AttachedVirtualFunction.h"

struct DoGetViewTag;

using DoGetView =
AttachedVirtualFunction<
   DoGetViewTag,
   std::shared_ptr< TrackView >,
   Track
>;
DECLARE_EXPORTED_ATTACHED_VIRTUAL(AUDACITY_DLL_API, DoGetView);

struct GetDefaultTrackHeightTag;

using GetDefaultTrackHeight =
AttachedVirtualFunction<
   GetDefaultTrackHeightTag,
   int,
   Track
>;
DECLARE_EXPORTED_ATTACHED_VIRTUAL(AUDACITY_DLL_API, GetDefaultTrackHeight);

#endif
