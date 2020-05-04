/**********************************************************************

  Audacity: A Digital Audio Editor

  FileHistory.h

  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_WIDGETS_FILEHISTORY__
#define __AUDACITY_WIDGETS_FILEHISTORY__

#include <functional>
#include <vector>
#include <algorithm>
#include <wx/defs.h>
#include <wx/weakref.h> // member variable

#include "Identifier.h"
#include "wxArrayStringEx.h"

class wxConfigBase;

namespace Widgets {
   class MenuHandle;
}

class AUDACITY_DLL_API FileHistory
{
 public:
   FileHistory(size_t maxfiles = 12);
   virtual ~FileHistory();
   FileHistory( const FileHistory& ) = delete;
   FileHistory &operator =( const FileHistory & ) = delete;

   static FileHistory &Global();

   void Append( const FilePath &file )
   { AddFileToHistory( file, true ); }
   void Remove( size_t i );
   void Clear();

   // Causes this menu to reflect the contents of this FileHistory, now and
   // also whenever the history changes.
   void UseMenu(Widgets::MenuHandle menu);

   // Type of function that receives notification of selection from the list
   // Return value is true if the item should remain in the list
   using Callback = std::function< bool( const FilePath& ) >;

   // Set the callback (returning the previous)
   Callback SetCallback( Callback callback );

   // Load and save history contents to configuration file
   void Load(wxConfigBase& config, const wxString & group = wxEmptyString);
   void Save(wxConfigBase& config);

   // stl-style accessors
   using const_iterator = FilePaths::const_iterator;
   const_iterator begin() const;
   const_iterator end() const;
   const FilePath &operator[] ( size_t ii ) const;
   bool empty() const;

 private:
   void AddFileToHistory(const FilePath & file, bool update);
   void NotifyMenus();
   void NotifyMenu(Widgets::MenuHandle menu);

   void Compress();

   size_t mMaxFiles;

   std::vector< Widgets::MenuHandle > mMenus;
   FilePaths mHistory;

   wxString mGroup;
   Callback mCallback;
};

#endif
