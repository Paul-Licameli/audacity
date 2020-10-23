/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackViewConstants.cpp

Paul Licameli split from class WaveTrack

**********************************************************************/

#include "Internat.h"
#include "WaveTrackViewConstants.h"
#include <mutex>

static const auto PathStart = wxT("WaveTrackViewTypes");
static std::vector< WaveTrackSubViewType > types;

struct WaveTrackSubViewType::TypeItem::Visitor : ::Registry::Visitor {

   void Visit( SingleItem &item, const Path & ) override
   {
      types.emplace_back(static_cast<TypeItem&>(item).type);
   }

   static std::vector< WaveTrackSubViewType > &GetRegistry()
   {
      static std::once_flag flag;
      std::call_once( flag, []{
         static Registry::OrderingPreferenceInitializer init{
            PathStart,
            { {wxT(""), wxT("Waveform,Spectrogram") } }
         };
         Registry::TransparentGroupItem<> top{ PathStart };
         Visitor visitor;
         Registry::Visit( visitor, &top, &WaveTrackSubViewType::TypeItem::Registry() );
      } );
      return types;
   }
};

::Registry::GroupItem &WaveTrackSubViewType::TypeItem::Registry()
{
   static ::Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}

WaveTrackSubViewType::Registration::Registration(
   WaveTrackSubViewType type, const ::Registry::Placement &placement )
   : RegisteredItem{
      std::make_unique<TypeItem>( type ), placement }
{
}

WaveTrackSubViewType::Registration::~Registration() = default;

// static
auto WaveTrackSubViewType::All()
   -> const std::vector<WaveTrackSubViewType> &
{
   return TypeItem::Visitor::GetRegistry();
}

bool
WaveTrackSubViewType::operator < ( const WaveTrackSubViewType &other ) const
{
   auto types = All();
   auto begin = types.begin(), end = types.end();
   auto iter1 = std::find( begin, end, *this );
   auto iter2 = std::find( begin, end, other );
   return iter1 - iter2 < 0;
}

// static
auto WaveTrackSubViewType::Default() -> Identifier
{
   auto &all = All();
   if (all.empty())
      return {};
   return all[0].name.Internal();
}

const EnumValueSymbol WaveTrackViewConstants::MultiViewSymbol{
   wxT("Multiview"), XXO("&Multi-view")
};

WaveTrackSubViewType::TypeItem::TypeItem( WaveTrackSubViewType type )
   : SingleItem{ type.name.Internal() }
   , type{ type }
{}

WaveTrackSubViewType::TypeItem::~TypeItem() = default;

WaveTrackSubViewType::Registration::Init::Init() { TypeItem::Registry(); }
