/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectFileIORegistry.cpp

  Paul Licameli

**********************************************************************/

#include "ProjectFileIORegistry.h"

#include <wx/string.h>
#include <unordered_map>


namespace ProjectFileIORegistry {

namespace {
   using AttributeTable = std::unordered_map< std::wstring, AttributeHandler >;
   static AttributeTable &sAttributeTable()
   {
      static AttributeTable theTable;
      return theTable;
   }
}

AttributeEntry::AttributeEntry(
   const wchar_t *attr, const AttributeHandler &fn )
{
   sAttributeTable()[ attr ] = fn;
}

AttributeEntry::Init::Init() { (void)sAttributeTable(); }

AttributeHandler LookupAttribute( const wchar_t *attr )
{
   const auto &table = sAttributeTable();
   auto iter = table.find( attr );
   if ( iter == table.end() )
      return {};
   return iter->second;
}

namespace {
   using TagTable = std::unordered_map< wxString, TagHandlerFactory >;
   static TagTable &sTagTable()
   {
      static TagTable theTable;
      return theTable;
   }
}

Entry::Entry(
   const wxString &tag, const TagHandlerFactory &factory )
{
   sTagTable()[ tag ] = factory;
}

Entry::Init::Init(){ (void) sTagTable(); }

TagHandlerFactory Lookup( const wxString &tag )
{
   const auto &table = sTagTable();
   auto iter = table.find( tag );
   if ( iter == table.end() )
      return {};
   return iter->second;
}


namespace {
WriterTable &sWriterTable()
{
   static WriterTable theTable;
   return theTable;
}
}

WriterEntry::WriterEntry( const Writer &writer )
{
   sWriterTable().emplace_back(writer);
}

WriterEntry::Init::Init(){ (void)sWriterTable(); }

const WriterTable &GetWriters() { return sWriterTable(); }

}
