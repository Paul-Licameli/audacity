/**********************************************************************

  Audacity: A Digital Audio Editor

  Import.h

  Dominic Mazzoni

**********************************************************************/

#ifndef _IMPORT_
#define _IMPORT_

#include "ImportForwards.h"
#include "Identifier.h"
#include <vector>
#include <wx/tokenzr.h> // for enum wxStringTokenizerMode

#include "../widgets/wxPanelWrapper.h" // to inherit
#include "FileNames.h" // for FileType

#include "Registry.h"

class wxArrayString;
class wxListBox;
class AudacityProject;
class Tags;
class WaveTrackFactory;
class Track;
class ImportPlugin;
class ImportFileHandle;
class UnusableImportPlugin;
typedef bool (*progress_callback_t)( void *userData, float percent );

class ExtImportItem;
class WaveTrack;

using ExtImportItems = std::vector< std::unique_ptr<ExtImportItem> >;
using TrackHolders = std::vector< std::vector< std::shared_ptr<Track> > >;

class ExtImportItem
{
  public:
  /**
   * Unique string ID exists for each filter, it is not translatable
   * and can be stored in config. This ID is matched internally with a
   * translated name of a filter.
   * Unknown IDs will be presented and saved as-is.
   * Unused filters will not be used for matching files, unless forced.
   */
  wxArrayString filters;

  /**
   * The index of first unused filter in @filters array
   * 0 - divider is at the top of the list (in the list control
   * it will be the highest item), all filters are unused
   * -1 - divider is at the bottom of the list (in the list control
   * it will be the lowest item), all filters are used
   */
  int divider;

  /**
   * Array of pointers to import plugins (members of FormatList)
   */
  std::vector<ImportPlugin*> filter_objects;

  /**
   * File extensions. Each one is a string with simple wildcards,
   * i.e. "*.wav".
   */
  wxArrayString extensions;

  /**
   * Mime-types. Each one is a string in form of "foo/bar-baz" or
   * something like that.
   */
  wxArrayString mime_types;
};

class AUDACITY_DLL_API  Importer {
   struct ImporterItem;
public:

   // Objects of this type are statically constructed in files implementing
   // subclasses of ImportPlugin
   struct AUDACITY_DLL_API RegisteredImportPlugin final
      : public Registry::RegisteredItem<ImporterItem>
   {
      RegisteredImportPlugin(
         const Identifier &id, // an internal string naming the plug-in
         std::unique_ptr<ImportPlugin>,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct AUDACITY_DLL_API Init{ Init(); };
   };

   // Objects of this type are statically constructed in files, to identify
   // unsupported import formats; typically in a conditional compilation
   struct RegisteredUnusableImportPlugin{
      RegisteredUnusableImportPlugin( std::unique_ptr<UnusableImportPlugin> );
      ~RegisteredUnusableImportPlugin();

      struct AUDACITY_DLL_API Init{ Init(); };

   private:
      UnusableImportPlugin *mpPlugin = nullptr;
   };

   Importer();
   ~Importer();

   Importer( const Importer& ) PROHIBITED;
   Importer &operator=( Importer& ) PROHIBITED;

   /**
    * Return instance reference
    */
   static Importer & Get();

   /**
    * Initialization/Termination
    */
   bool Initialize();
   bool Terminate();

   /**
    * Reads extended import filters from gPrefs into internal
    * list mExtImportItems
    */
   void ReadImportItems();

   /**
    * Writes mExtImportItems into gPrefs
    */
   void WriteImportItems();

   /**
    * Helper function - uses wxStringTokenizer to tokenize
    * @str string and appends string-tokens to a list @list.
    * @mod defines tokenizer's behaviour.
    */
   void StringToList(wxString &str, wxString &delims, wxArrayString &list, wxStringTokenizerMode mod = wxTOKEN_RET_EMPTY_ALL);

   /**
    * Returns a pointer to internal items array.
    * External objects are allowed to change the array contents.
    */
   ExtImportItems &GetImportItems() { return mExtImportItems; }

   /**
    * Allocates NEW ExtImportItem, fills it with default data
    * and returns a pointer to it.
    */
    std::unique_ptr<ExtImportItem> CreateDefaultImportItem();

   static bool Import(AudacityProject &project, const FilePath &fileName,
               bool addToHistory = true);

   static void AddImportedTracks(AudacityProject &project,
      const FilePath &fileName,
      TrackHolders &&newTracks);

private:
   // if false, the import failed and errorMessage will be set.
   bool Import( AudacityProject &project,
              const FilePath &fName,
              WaveTrackFactory *trackFactory,
              TrackHolders &tracks,
              Tags *tags,
              TranslatableString &errorMessage);

   struct AUDACITY_DLL_API ImporterItem final : Registry::SingleItem {
      static Registry::GroupItem &Registry();

      ImporterItem( const Identifier &id, std::unique_ptr<ImportPlugin> pPlugin );
      ~ImporterItem();
      std::unique_ptr<ImportPlugin> mpPlugin;
   };

   static Importer mInstance;

   ExtImportItems mExtImportItems;
   static UnusableImportPluginList &sUnusableImportPluginList();
};

//----------------------------------------------------------------------------
// ImportStreamDialog
//----------------------------------------------------------------------------

class ImportStreamDialog final : public wxDialogWrapper
{
public:
   // constructors and destructors
   ImportStreamDialog( ImportFileHandle *_mFile,
      wxWindow *parent, wxWindowID id, const TranslatableString &title,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
   virtual ~ImportStreamDialog();

private:
   ImportFileHandle *mFile;
   wxInt32 scount;
   wxListBox *StreamList;

private:
   void OnOk( wxCommandEvent &event );
   void OnCancel( wxCommandEvent &event );

private:
   DECLARE_EVENT_TABLE()
};

// Guarantees registries exist before attempts to use them
static Importer::RegisteredImportPlugin::Init sInitRegisteredImportPlugin;
static Importer::RegisteredUnusableImportPlugin::Init
   sInitRegisteredUnusableImportPlugin;

#endif
