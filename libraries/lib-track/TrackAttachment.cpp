/**********************************************************************

Audacity: A Digital Audio Editor

TrackAttachment.cpp
@brief implements TrackAttachment

Paul Licameli split from CommonTrackPanelCell.cpp

**********************************************************************/

#include "TrackAttachment.h"

TrackAttachment::~TrackAttachment() = default;

void TrackAttachment::CopyTo( Track& ) const
{
}

void TrackAttachment::Reparent( const std::shared_ptr<Track> & )
{
}

void TrackAttachment::WriteXMLAttributes( XMLWriter & ) const
{
}

bool TrackAttachment::HandleXMLAttribute( const wxChar *, const wxChar * )
{
   return false;
}
