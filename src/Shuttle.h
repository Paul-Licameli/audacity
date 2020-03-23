/**********************************************************************

  Audacity: A Digital Audio Editor

  Shuttle.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE__
#define __AUDACITY_SHUTTLE__

#include "../include/audacity/ComponentInterface.h"

class ComponentInterfaceSymbol;
class WrappedType;

template<typename Type> struct EffectParameter {
   const wxChar *const key;
   const Type def;
   const Type min;
   const Type max;
   const Type scale;
   mutable Type cache;
   inline operator const Type& () const { return cache; }
};

template<typename Type>
inline const EffectParameter<Type> Parameter(
   const wxChar *key,
   const Type &def, const Type &min, const Type &max, const Type &scale )
{ return { key, def, min, max, scale, {} }; }

struct EnumEffectParameter : EffectParameter<int>
{
   EnumEffectParameter(
      const wxChar *key, int def, int min, int max, int scale,
      const EnumValueSymbol *symbols_, size_t nSymbols_ )
      : EffectParameter<int>{ key, def, min, max, scale, {} }
      , symbols{ symbols_ }
      , nSymbols{ nSymbols_ }
   {}

   const EnumValueSymbol *const symbols;
   const size_t nSymbols;
};

using EnumParameter = EnumEffectParameter;

class Shuttle /* not final */ {
 public:
   // constructors and destructors
   Shuttle();
   virtual ~Shuttle() {}

 public:
   bool mbStoreInClient;
   wxString mValueString;
   // Even though virtual, mostly the transfer functions won't change
   // for special kinds of archive.
   virtual bool TransferBool( const wxString & Name, bool & bValue, const bool & bDefault );
   virtual bool TransferFloat( const wxString & Name, float & fValue, const float &fDefault );
   virtual bool TransferDouble( const wxString & Name, double & dValue, const double &dDefault );
   virtual bool TransferInt( const wxString & Name, int & iValue, const int &iDefault );
   virtual bool TransferInt( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferLongLong( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferString( const wxString & Name, wxString & strValue, const wxString &strDefault );
   virtual bool TransferEnum( const wxString & Name, int & iValue,
      const int nChoices, const wxString * pFirstStr);
   virtual bool TransferWrappedType( const wxString & Name, WrappedType & W );
   // We expect the ExchangeWithMaster function to change from one type of
   // archive to another.
   virtual bool ExchangeWithMaster(const wxString & Name);
};

class ShuttleCli final : public Shuttle
{
public:
   wxString mParams;
   ShuttleCli() {}
   virtual ~ShuttleCli() {}
   bool ExchangeWithMaster(const wxString & Name) override;
};

class CommandParameters;
/**************************************************************************//**
\brief Shuttle that deals with parameters.  This is a base class with lots of
virtual functions that do nothing by default.
Unrelated to class Shuttle.
********************************************************************************/
class AUDACITY_DLL_API ShuttleParams /* not final */
{
public:
   wxString mParams;
   bool *pOptionalFlag;
   CommandParameters * mpEap;
   ShuttleParams() { mpEap = NULL; pOptionalFlag = NULL; }
   virtual ~ShuttleParams() {}
   bool ShouldSet();
   virtual ShuttleParams & Optional( bool & WXUNUSED(var) ){ pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var ){ return Optional( var );};
   virtual ShuttleParams & OptionalN( bool & var ){ return Optional( var );};
   virtual void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin=false, const bool vmax=false, const bool vscl=false );
   virtual void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( int & var,      const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl=1.0f );
   virtual void Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin = {}, const wxString vmax = {}, const wxString vscl = {} );
   virtual void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings );

   template< typename Var, typename Type >
   void SHUTTLE_PARAM( Var &var, const EffectParameter< Type > &name )
   { Define( var, name.key, name.def, name.min, name.max, name.scale ); }

   void SHUTTLE_ENUM_PARAM(
      int &var, const EffectParameter< int > &name,
      const EnumValueSymbol strings[], size_t nStrings )
   { DefineEnum( var, name.key, name.def, strings, nStrings ); }
};

/**************************************************************************//**
\brief Shuttle that gets parameter values into a string.
********************************************************************************/
class AUDACITY_DLL_API ShuttleGetAutomation final : public ShuttleParams
{
public:
   ShuttleParams & Optional( bool & var ) override;
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};

/**************************************************************************//**
\brief Shuttle that sets parameters to a value (from a string)
********************************************************************************/
class AUDACITY_DLL_API ShuttleSetAutomation final : public ShuttleParams
{
public:
   ShuttleSetAutomation(){ bWrite = false; bOK = false;};
   bool bOK;
   bool bWrite;
   ShuttleParams & Optional( bool & var ) override;
   bool CouldGet(const wxString &key);
   void SetForValidating( CommandParameters * pEap){ mpEap=pEap; bOK=true;bWrite=false;};
   void SetForWriting(CommandParameters * pEap){ mpEap=pEap;bOK=true;bWrite=true;};
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};


/**************************************************************************//**
\brief Shuttle that sets parameters to their default values.
********************************************************************************/
class ShuttleDefaults final : public ShuttleParams
{
public:
   wxString Result;
   virtual ShuttleParams & Optional( bool & var )override{  var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var )override{ var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalN( bool & var )override{ var = false;pOptionalFlag = NULL;return *this;};

   void Define( bool & var,          const wxChar * WXUNUSED(key),  const bool     vdefault, 
      const bool     WXUNUSED(vmin), const bool     WXUNUSED(vmax), const bool     WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( int & var,           const wxChar * WXUNUSED(key),  const int      vdefault, 
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( size_t & var,        const wxChar * WXUNUSED(key),  const int      vdefault, 
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) ) 
      override{ var = vdefault;};
   void Define( float & var,         const wxChar * WXUNUSED(key),  const float    vdefault, 
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const float    vdefault, 
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const double   vdefault, 
      const double   WXUNUSED(vmin), const double   WXUNUSED(vmax), const double   WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( wxString &var,       const wxChar * WXUNUSED(key),  const wxString vdefault, 
      const wxString WXUNUSED(vmin), const wxString WXUNUSED(vmax), const wxString WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void DefineEnum( int &var,        const wxChar * WXUNUSED(key),  const int vdefault,
      const EnumValueSymbol WXUNUSED(strings) [], size_t WXUNUSED( nStrings ) )
      override { var = vdefault;};
};






#endif
