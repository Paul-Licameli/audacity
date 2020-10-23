/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackViewConstants.h

Paul Licameli split from class WaveTrack

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_VIEW_CONSTANTS__
#define __AUDACITY_WAVE_TRACK_VIEW_CONSTANTS__

#include "Registry.h"
#include "ComponentInterfaceSymbol.h" // for EnumValueSymbol

namespace WaveTrackViewConstants
{
   // Only two types of sample display for now, but
   // others (eg sinc interpolation) may be added later.
   enum SampleDisplay {
      LinearInterpolate = 0,
      StemPlot
   };

   enum ZoomActions {
      // Note that these can be with or without spectrum view which
      // adds a constant.
      kZoom1to1 = 1,
      kZoomTimes2,
      kZoomDiv2,
      kZoomHalfWave,
      kZoomInByDrag,
      kZoomIn,
      kZoomOut,
      kZoomReset
   };

   //! String identifier for a preference for one of each type of view
   extern AUDACITY_DLL_API const EnumValueSymbol MultiViewSymbol;
}

#include <vector>

struct AUDACITY_DLL_API WaveTrackSubViewType {

   struct TypeItem;

   // The translation is suitable for the track control panel drop-down,
   // and it may contain a menu accelerator
   EnumValueSymbol name;

   // Types are extrinsically ordered by registration order
   bool operator < ( const WaveTrackSubViewType &other ) const;

   bool operator == ( const WaveTrackSubViewType &other ) const
   { return name == other.name; }

   // Typically a file scope statically constructed object
   struct AUDACITY_DLL_API Registration final
      : public Registry::RegisteredItem<TypeItem>
   {
      Registration( WaveTrackSubViewType type,
         const Registry::Placement &placement = { wxEmptyString, {} });
      ~Registration();

      struct AUDACITY_DLL_API Init{ Init(); };
   };

   //! Discover all registered types
   static const std::vector<WaveTrackSubViewType> &All();

   //! Return a preferred type
   static Identifier Default();
};

struct AUDACITY_DLL_API WaveTrackSubViewType::TypeItem final
   : ::Registry::SingleItem {
   static ::Registry::GroupItem &Registry();
   TypeItem( WaveTrackSubViewType type );
   ~TypeItem();
   struct Visitor;
   WaveTrackSubViewType type;
};

static WaveTrackSubViewType::Registration::Init sInitWaveTrackSubViewTypes;

#endif
