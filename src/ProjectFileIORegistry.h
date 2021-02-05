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
struct AUDACITY_DLL_API Entry{
   Entry( const wxString &tag, const TagHandlerFactory &factory );
};

TagHandlerFactory Lookup( const wxString &tag );

//! Type of function that writes extra data directly contained in the top project tag
using Writer = std::function< void(const AudacityProject &, XMLWriter &) >;
using WriterTable = std::vector< Writer >;

//! Typically statically constructed
struct AUDACITY_DLL_API WriterEntry{
   WriterEntry( const Writer &writer );
   struct AUDACITY_DLL_API Init{ Init(); };
};

const WriterTable &GetWriters();

}

#endif
