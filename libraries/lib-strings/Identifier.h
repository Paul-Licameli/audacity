/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 Identifier.h
 
 Paul Licameli split from Types.h
 
 **********************************************************************/

#ifndef __AUDACITY_IDENTIFIER__
#define __AUDACITY_IDENTIFIER__

#include <vector>
#include <wx/string.h>

//! An explicitly nonlocalized string, not meant for the user to see.
/*! String manipulations are discouraged, other than splitting and joining on separator characters.
Wherever GET is used to fetch the underlying wxString, there should be a comment explaining the need for it.
 */
class STRINGS_API Identifier
{
public:
   using value_type = std::wstring;
   using char_type = value_type::value_type;

   Identifier() = default;

   //! Allow implicit conversion to this class, but not from
   Identifier(const value_type &str);

   //! Allow implicit conversion to this class, but not from
   Identifier(const wxString &str);

   // Allow implicit conversion to this class, but not from
   Identifier(const char_type *str);

   // Allow implicit conversion to this class, but not from
   Identifier(const char *str);

   // Copy construction and assignment
   Identifier( const Identifier & ) = default;
   Identifier &operator = ( const Identifier& ) = default;

   // Move construction and assignment
   Identifier( wxString && str );
   Identifier( Identifier && id )
      { swap( id ); }
   Identifier &operator= ( Identifier&& id )
   {
      if ( this != &id )
         value.clear(), swap( id );
      return *this;
   }

   // Implements moves
   void swap( Identifier &id ) { value.swap( id.value ); }

   //! Convenience for building concatenated identifiers.
   /*! The list must have at least two members (so you don't easily circumvent the restrictions on
    interconversions intended in TaggedIdentifier below) */
   explicit Identifier(
      std::initializer_list<Identifier> components, char_type separator);

   bool empty() const { return value.empty(); }
   size_t size() const { return value.size(); }
   size_t length() const { return value.length(); }

   //! Explicit conversion to wxString, meant to be ugly-looking and demanding of a comment why it's correct
   wxString GET() const;

   std::vector< Identifier > split( char_type separator ) const;

private:
   value_type value;
};

//! Comparisons of Identifiers are case-sensitive
bool operator == ( const Identifier &x, const Identifier &y );

inline bool operator != ( const Identifier &x, const Identifier &y )
{ return !(x == y); }

bool operator < ( const Identifier &x, const Identifier &y );

inline bool operator > ( const Identifier &x, const Identifier &y )
{ return y < x; }

inline bool operator <= ( const Identifier &x, const Identifier &y )
{ return !(y < x); }

inline bool operator >= ( const Identifier &x, const Identifier &y )
{ return !(x < y); }

namespace std
{
   template<> struct hash< Identifier > {
      size_t operator () ( const Identifier &id ) const // noexcept
      ;
   };
}

//! This lets you pass Identifier into wxFileConfig::Read
inline bool wxFromString(const wxString& str, Identifier *id)
   { if (id) { *id = str; return true; } else return false; }

//! This lets you pass Identifier into wxFileConfig::Write
wxString wxToString( const Identifier& str );

//! Template generates different TaggedIdentifier classes that don't interconvert implicitly
/*! The second template parameter determines whether comparisons are case
sensitive; default is case sensitive */
template<typename Tag, bool CaseSensitive = true >
class TaggedIdentifier : public Identifier
{
public:

   using TagType = Tag;

   using Identifier::Identifier;
   TaggedIdentifier() = default;

   //! Copy allowed only for the same Tag class and case sensitivity
   TaggedIdentifier( const TaggedIdentifier& ) = default;
   //! Move allowed only for the same Tag class and case sensitivity
   TaggedIdentifier( TaggedIdentifier&& ) = default;
   //! Copy allowed only for the same Tag class and case sensitivity
   TaggedIdentifier& operator= ( const TaggedIdentifier& ) = default;
   //! Move allowed only for the same Tag class and case sensitivity
   TaggedIdentifier& operator= ( TaggedIdentifier&& ) = default;

   //! Copy prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier( const TaggedIdentifier<Tag2, b>& ) PROHIBITED;
   //! Move prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier( TaggedIdentifier<Tag2, b>&& ) PROHIBITED;
   //! Copy prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier& operator= ( const TaggedIdentifier<Tag2, b>& ) PROHIBITED;
   //! Move prohibited for other Tag classes or case sensitivity
   template< typename Tag2, bool b >
   TaggedIdentifier& operator= ( TaggedIdentifier<Tag2, b>&& ) PROHIBITED;

   /*! Allow implicit conversion to this class from un-tagged Identifier,
      but not from; resolution will use other overloads above if argument has a tag */
   TaggedIdentifier(const Identifier &str) : Identifier{ str } {}

   //! Explicit conversion to another kind of TaggedIdentifier
   template<typename String, typename = typename String::TagType>
      String CONVERT() const
         { return String{ this->GET() }; }
};

/*! Comparison of a TaggedIdentifier with an Identifier is allowed, resolving
to one of the operators on Identifiers, but always case sensitive.
Comparison operators for two TaggedIdentifiers require the same tags and case sensitivity. */
template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator == (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{
   static_assert( std::is_same< Tag1, Tag2 >::value && b1 == b2,
      "TaggedIdentifiers with different tags or sensitivity are not comparable" );
   // This test should be eliminated at compile time:
   if ( b1 )
      return x.GET(). Cmp ( y.GET() ) == 0;
   else
      return x.GET(). CmpNoCase ( y.GET() ) == 0;
}

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator != (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(x == y); }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator < (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{
   static_assert( std::is_same< Tag1, Tag2 >::value && b1 == b2,
      "TaggedIdentifiers with different tags or sensitivity are not comparable" );
   // This test should be eliminated at compile time:
   if ( b1 )
      return x.GET(). Cmp ( y.GET() ) < 0;
   else
      return x.GET(). CmpNoCase ( y.GET() ) < 0;
}

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator > (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return y < x; }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator <= (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(y < x); }

template< typename Tag1, typename Tag2, bool b1, bool b2 >
inline bool operator >= (
   const TaggedIdentifier< Tag1, b1 > &x, const TaggedIdentifier< Tag2, b2 > &y )
{ return !(x < y); }

namespace std
{
   template<typename Tag, bool b > struct hash< TaggedIdentifier<Tag, b> >
      : hash< Identifier > {};
}

/**************************************************************************//**

\brief type alias for identifying a Plugin supplied by a module, each module
defining its own interpretation of the strings, which may or may not be as a
file system path
********************************************************************************/
using PluginPath = wxString;
using PluginPaths = std::vector< PluginPath >;

// A key to be passed to wxConfigBase
using RegistryPath = wxString;
using RegistryPaths = std::vector< RegistryPath >;

class wxArrayStringEx;

//! File extension, not including any leading dot
using FileExtension = wxString;
using FileExtensions = wxArrayStringEx;

using FilePath = wxString;
using FilePaths = wxArrayStringEx;

struct CommandIdTag;
//! Identifies a menu command or macro. Case-insensitive comparison
using CommandID = TaggedIdentifier< CommandIdTag, false >;
using CommandIDs = std::vector<CommandID>;

#endif

