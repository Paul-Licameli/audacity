/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectFileIORegistry.h

  Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_PROJECT_FILE_IO_REGISTRY__
#define __AUDACITY_PROJECT_FILE_IO_REGISTRY__

#include <functional>
#include <vector>

class AudacityProject;
class XMLTagHandler;
class XMLWriter;
class wxString;

namespace ProjectFileIORegistry {

//! Type of functions returning objects that interpret a part of the saved XML
using TagHandlerFactory =
   std::function< XMLTagHandler *( AudacityProject & ) >;

//! Typically statically constructed
struct XML_API Entry{
   Entry( const wxString &tag, const TagHandlerFactory &factory );

   struct XML_API Init{ Init(); };
};

XML_API TagHandlerFactory Lookup( const wxString &tag );

//! Type of function that writes extra data directly contained in the top project tag
using Writer = std::function< void(const AudacityProject &, XMLWriter &) >;
using WriterTable = std::vector< Writer >;

//! Typically statically constructed
struct XML_API WriterEntry{
   WriterEntry( const Writer &writer );
   struct XML_API Init{ Init(); };
};

XML_API const WriterTable &GetWriters();

}

// Guarantees registry exists before attempts to use it
static ProjectFileIORegistry::Entry::Init sInitProjectFileIORegistryEntry;

#endif
