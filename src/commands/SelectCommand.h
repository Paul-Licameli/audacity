/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2018 Audacity Team
   License: GPL v2 - see LICENSE.txt

   Dan Horgan
   James Crook

******************************************************************//**

\file SelectCommand.h
\brief Declarations for SelectCommand and SelectCommandType classes

*//*******************************************************************/

#ifndef __SELECT_COMMAND__
#define __SELECT_COMMAND__



#include "CommandType.h"
#include "Command.h"

//#include "../commands/AudacityCommand.h"


#define SELECT_TIME_PLUGIN_SYMBOL XO("Select Time")
#define SELECT_FREQUENCY_PLUGIN_SYMBOL XO("Select Frequency")
#define SELECT_TRACKS_PLUGIN_SYMBOL XO("Select Tracks")
#define SELECT_PLUGIN_SYMBOL XO("Select")

class SelectTimeCommand : public AudacityCommand
{
public:
   // CommandDefinitionInterface overrides
   wxString GetSymbol() override {return SELECT_TIME_PLUGIN_SYMBOL;};
   wxString GetDescription() override {return _("Selects a time range.");};
   bool DefineParams( ShuttleParams & S ) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool Apply(const CommandContext & context) override;

   // AudacityCommand overrides
   wxString ManualPage() override {return wxT("Audio_Selection");};

   bool bHasT0;
   bool bHasT1;

   double mT0;
   double mT1;
   bool mFromEnd;
};

class SelectFrequenciesCommand : public AudacityCommand
{
public:
   // CommandDefinitionInterface overrides
   wxString GetSymbol() override {return SELECT_FREQUENCY_PLUGIN_SYMBOL;};
   wxString GetDescription() override {return _("Selects a frequency range.");};
   bool DefineParams( ShuttleParams & S ) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool Apply(const CommandContext & context) override;

   // AudacityCommand overrides
   wxString ManualPage() override {return wxT("Spectral_Selection");};

   bool bHasBottom;
   bool bHasTop;

   double mBottom;
   double mTop;
};


class SelectTracksCommand : public AudacityCommand
{
public:
   // CommandDefinitionInterface overrides
   wxString GetSymbol() override {return SELECT_TRACKS_PLUGIN_SYMBOL;};
   wxString GetDescription() override {return _("Selects a range of tracks.");};
   bool DefineParams( ShuttleParams & S ) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool Apply(const CommandContext & context) override;
   // AudacityCommand overrides
   wxString ManualPage() override {return wxT("Audio_Selection");};

   bool bHasFirstTrack;
   bool bHasLastTrack;

   int mFirstTrack;
   int mLastTrack;
   int mMode;
};


class SelectCommand : public AudacityCommand
{
public:
   // CommandDefinitionInterface overrides
   wxString GetSymbol() override {return SELECT_PLUGIN_SYMBOL;};
   wxString GetDescription() override {return _("Selects Audio.");};
   bool DefineParams( ShuttleParams & S ) override { 
      return 
         mSelTime.DefineParams(S) &&  
         mSelFreq.DefineParams(S) &&
         mSelTracks.DefineParams(S);
   };
   void PopulateOrExchange(ShuttleGui & S) override {
      mSelTime.PopulateOrExchange(S);
      mSelFreq.PopulateOrExchange(S);
      mSelTracks.PopulateOrExchange(S);
   };
   bool Apply(const CommandContext & context) override {
      return 
         mSelTime.Apply(context) &&  
         mSelFreq.Apply( context )&&
         mSelTracks.Apply(context);
   }
   // AudacityCommand overrides
   wxString ManualPage() override {return wxT("Audio_Selection");};
private:
   SelectTimeCommand mSelTime;
   SelectFrequenciesCommand mSelFreq;
   SelectTracksCommand mSelTracks;


};

#endif /* End of include guard: __SELECT_COMMAND__ */
