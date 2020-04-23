/**********************************************************************

  Shuttle.cpp

  James Crook
  (C) Audacity Developers, 2007

  wxWidgets license. See Licensing.txt

*******************************************************************//**

\file Shuttle.cpp
\brief Implements Shuttle, ShuttleCli and Enums.

*//****************************************************************//**

\class Shuttle
\brief Moves data from one place to another, converting it as required.

  Shuttle provides a base class for transferring parameter data into and
  out of classes into some other structure.  This is a common
  requirement and is needed for:
    - Prefs data
    - Command line parameter data
    - Project data in XML

  The 'Master' is the string side of the shuttle transfer, the 'Client'
  is the binary data side of the transfer.

  \see ShuttleBase
  \see ShuttleGui

*//****************************************************************//**

\class ShuttleCli
\brief Derived from Shuttle, this class exchanges string parameters
with a binary representation.

  \see ShuttleBase
  \see ShuttleGui

*//****************************************************************//**

\class Enums
\brief Enums is a helper class for Shuttle.  It defines enumerations
which are used in effects dialogs, in the effects themselves and in
preferences.

(If it grows big, we will move it out of shuttle.h).

*//*******************************************************************/


#include "Shuttle.h"

#include <wx/defs.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>

#include "WrappedType.h"
//#include "effects/Effect.h"


Shuttle::Shuttle()
{
}

bool Shuttle::TransferBool( const wxString & Name, bool & bValue, const bool & bDefault )
{
   if( mbStoreInClient )
   {
      bValue = bDefault;
      if( ExchangeWithMaster( Name ))
      {
         if( !mValueString.empty() )
            bValue = mValueString.GetChar(0) == L'y';
      }
   }
   else
   {
      mValueString = (bValue==0) ? L"no":L"yes";
      return ExchangeWithMaster( Name );
   }
   return true;
}

bool Shuttle::TransferFloat( const wxString & Name, float & fValue, const float &fDefault )
{
   if( mbStoreInClient )
   {
      fValue = fDefault;
      if( ExchangeWithMaster( Name ))
      {
         if( !mValueString.empty() )
            fValue = wxAtof( mValueString );
      }
   }
   else
   {
      mValueString = wxString::Format(L"%f",fValue);
      return ExchangeWithMaster( Name );
   }
   return true;
}

bool Shuttle::TransferDouble( const wxString & Name, double & dValue, const double &dDefault )
{
   if( mbStoreInClient )
   {
      dValue = dDefault;
      if( ExchangeWithMaster( Name ))
      {
         if( !mValueString.empty() )
            dValue = wxAtof( mValueString );
      }
   }
   else
   {
      //%f is format string for double
      mValueString = wxString::Format(L"%f",dValue);
      return ExchangeWithMaster( Name );
   }
   return true;
}

bool Shuttle::TransferInt( const wxString & Name, int & iValue, const int & iDefault )
{
   if( mbStoreInClient )
   {
      iValue = iDefault;
      if( ExchangeWithMaster( Name ))
      {
         iValue = wxAtoi( mValueString );
      }
   }
   else
   {
      mValueString = wxString::Format(L"%i",iValue);
      return ExchangeWithMaster( Name );
   }
   return true;
}


bool Shuttle::TransferInt( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t & iDefault )
{
   return TransferLongLong(Name, iValue, iDefault);
}

bool Shuttle::TransferLongLong( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t & iDefault )
{
   if( mbStoreInClient )
   {
      iValue = iDefault;
      if( ExchangeWithMaster( Name ))
      {
         iValue = wxAtoi( mValueString );
      }
   }
   else
   {
      /// \todo Fix for long long values.
      mValueString = wxString::Format(L"%lld", (long long) iValue);
      return ExchangeWithMaster( Name );
   }
   return true;
}


bool Shuttle::TransferEnum( const wxString & Name, int & iValue,
      const int nChoices, const wxString * pFirstStr)
{
   if( mbStoreInClient )
   {
      iValue = 0;// default index if none other selected.
      if( ExchangeWithMaster( Name ))
      {
         wxString str = mValueString;
         if( str.Left( 1 ) == L'"' && str.Right( 1 ) == L'"' )
         {
            str = str.Mid( 2, str.length() - 2 );
         }

         for( int i = 0; i < nChoices; i++ )
         {
            if( str == pFirstStr[i] )
            {
               iValue = i;
               break;
            }
         }
      }
   }
   else
   {
      //TIDY-ME: Out of range configuration values are silently discarded...
      if( iValue > nChoices )
         iValue = 0;
      if( iValue < 0 )
         iValue = 0;
      mValueString = pFirstStr[iValue];
      if( mValueString.Find( L' ' ) != wxNOT_FOUND )
      {
         mValueString = L'"' + pFirstStr[iValue] + L'"';  //strings have quotes around them
      }
      return ExchangeWithMaster( Name );
   }
   return true;
}

bool Shuttle::TransferString( const wxString & Name, wxString & strValue, const wxString & WXUNUSED(strDefault) )
{
   if( mbStoreInClient )
   {
      if( ExchangeWithMaster( Name ))
      {
         strValue = mValueString;
      }
      else
         return false;
   }
   else
   {
      mValueString = L'"' + strValue + L'"';  //strings have quotes around them
      return ExchangeWithMaster( Name );
   }
   return true;
}

bool Shuttle::TransferWrappedType( const wxString & Name, WrappedType & W )
{
   if( mbStoreInClient )
   {
      if( ExchangeWithMaster( Name ))
      {
         W.WriteToAsString( mValueString );
      }
   }
   else
   {
      mValueString = W.ReadAsString();
      return ExchangeWithMaster( Name );
   }
   return true;
}


bool Shuttle::ExchangeWithMaster(const wxString & WXUNUSED(Name))
{
   // ExchangeWithMaster() will usually be over-ridden
   // in derived classes.  We could have made it an
   // abstract function.
   wxASSERT( false );
   return true;
}

// This variant uses values of the form
// param1=value1 param2=value2
bool ShuttleCli::ExchangeWithMaster(const wxString & Name)
{
   if( !mbStoreInClient )
   {
      mParams += L" ";
      mParams +=Name;
      mParams += L"=";
      mParams +=mValueString;
   }
   else
   {
      int i;
      mParams = L" "+mParams;
      i=mParams.Find( L" "+Name+L"=" );
      if( i>=0 )
      {
         int j=i+2+Name.length();
         wxString terminator = L' ';
         if(mParams.GetChar(j) == L'"') //Strings are surrounded by quotes
         {
            terminator = L'"';
            j++;
         }
         else if(mParams.GetChar(j) == L'\'') // or by single quotes.
         {
            terminator = L'\'';
            j++;
         }         
         i=j;
         while( j<(int)mParams.length() && mParams.GetChar(j) != terminator )
            j++;
         mValueString = mParams.Mid(i,j-i);
         return true;
      }
      return false;
   }
   return true;
}


#ifdef _MSC_VER
// If this is compiled with MSVC (Visual Studio)
#pragma warning( push )
#pragma warning( disable: 4100 ) // unused parameters.
#endif //_MSC_VER


// The ShouldSet and CouldGet functions have an important side effect
// on the pOptionalFlag.  They 'use it up' and clear it down for the next parameter.


// Tests for parameter being optional.
// Prepares for next parameter by clearing the pointer.
// Reports on whether the parameter should be set, i.e. should set 
// if it was chosen to be set, or was not optional.
bool ShuttleParams::ShouldSet(){
   if( !pOptionalFlag )
      return true;
   bool result = *pOptionalFlag;
   pOptionalFlag = NULL;
   return result;
}
// These are functions to override.  They do nothing.
void ShuttleParams::Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl){;};
void ShuttleParams::Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ){;};
void ShuttleParams::Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ){;};
void ShuttleParams::Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ){;};
void ShuttleParams::Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ){;};
void ShuttleParams::Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ){;};
void ShuttleParams::Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ){;};
void ShuttleParams::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings ){;};



/*
void ShuttleParams::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings )
{
}
*/

// ShuttleGetAutomation gets from the shuttle into typically a string.
ShuttleParams & ShuttleGetAutomation::Optional( bool & var ){ 
   pOptionalFlag = &var;
   return *this;
};

void ShuttleGetAutomation::Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, var);
}

void ShuttleGetAutomation::Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, var);
}

void ShuttleGetAutomation::Define( size_t & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, (int) var);
}

void ShuttleGetAutomation::Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl )
{
   if( !ShouldSet() ) return;
   mpEap->WriteFloat(key, var);
}

void ShuttleGetAutomation::Define( float & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl )
{
   if( !ShouldSet() ) return;
   mpEap->WriteFloat(key, var);
}

void ShuttleGetAutomation::Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, var);
}


void ShuttleGetAutomation::Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, var);
}


void ShuttleGetAutomation::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings )
{
   if( !ShouldSet() ) return;
   mpEap->Write(key, strings[var].Internal());
}



ShuttleParams & ShuttleSetAutomation::Optional( bool & var ){ 
   pOptionalFlag = &var;
   return *this;
};

// Tests for parameter being optional.
// Prepares for next parameter by clearing the pointer.
// If the parameter is optional, finds out if it was actually provided.
// i.e. could it be got from automation?
// The result goes into the flag variable, so we typically ignore the result.
bool ShuttleSetAutomation::CouldGet( const wxString &key ){
   // Not optional?  Can get as we will get the default, at worst.
   if( !pOptionalFlag )
      return true;
   bool result = mpEap->HasEntry( key );
   *pOptionalFlag = result;
   pOptionalFlag = NULL;
   return result;
}

void ShuttleSetAutomation::Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   // Use of temp in this and related functions is to handle the case of 
   // only committing values if all values pass verification.
   bool temp =var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault);
   if( bWrite && bOK)
      var = temp;
}

void ShuttleSetAutomation::Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   int temp =var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, vmin, vmax);
   if( bWrite && bOK)
      var = temp;
}

void ShuttleSetAutomation::Define( size_t & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   int temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, vmin, vmax);
   if( bWrite && bOK )
      var = temp;
}

void ShuttleSetAutomation::Define( float & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   float temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, vmin, vmax);
   if( bWrite && bOK )
      var = temp;
}


void ShuttleSetAutomation::Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   double temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, vmin, vmax);
   if( bWrite && bOK)
      var = temp;
}

void ShuttleSetAutomation::Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   double temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, vmin, vmax);
   if( bWrite && bOK)
      var = temp;
}


void ShuttleSetAutomation::Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl )
{
   CouldGet( key );
   if( !bOK )
      return;
   wxString temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault);
   if( bWrite && bOK )
      var = temp;
}


void ShuttleSetAutomation::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings )
{
   CouldGet( key );
   if( !bOK )
      return;
   int temp = var;
   bOK = mpEap->ReadAndVerify(key, &temp, vdefault, strings, nStrings);
   if( bWrite && bOK)
      var = temp;
}


#ifdef _MSC_VER
// If this is compiled with MSVC (Visual Studio)
#pragma warning( pop )
#endif //_MSC_VER





