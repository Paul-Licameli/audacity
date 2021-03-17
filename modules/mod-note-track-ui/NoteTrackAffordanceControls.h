/*!********************************************************************
*
 Audacity: A Digital Audio Editor

 NoteTrackAffordanceControls.h

 Vitaly Sverchinsky

 **********************************************************************/

#pragma once

#include "CommonTrackPanelCell.h"

class AffordanceHandle;

class NOTE_TRACK_UI_API NoteTrackAffordanceControls
   : public TrackAffordanceControls
{
    std::weak_ptr<AffordanceHandle> mAffordanceHandle;
public:
    NoteTrackAffordanceControls(const std::shared_ptr<Track>& pTrack);

    std::vector<UIHandlePtr> HitTest(const TrackPanelMouseState& state, const AudacityProject* pProject) override;

    void Draw(TrackPanelDrawingContext& context, const wxRect& rect, unsigned iPass) override;

    bool IsSelected() const;
};
