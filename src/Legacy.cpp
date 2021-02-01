/**********************************************************************

  Audacity: A Digital Audio Editor

  Legacy.cpp

  Dominic Mazzoni

*******************************************************************//*!

\file Legacy.cpp
\brief Converts old Audacity file types.  Implements
AutoRollbackRenamer.

  These routines convert Audacity project files from the
  0.98...1.0 format into an XML format that's compatible with
  Audacity 1.2.0 and newer.

*//****************************************************************//**

\class AutoRollbackRenamer
\brief AutoRollbackRenamer handles the renaming of files
which is needed when producing a NEW version of a file which may fail.
On failure the old version is put back in place.

*//*******************************************************************/



#include "Legacy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wx/defs.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/textfile.h>

#include "widgets/AudacityMessageBox.h"
#include "xml/XMLWriter.h"

static bool ConvertLegacyTrack(wxTextFile *f, XMLFileWriter &xmlFile)
// may throw
{
   wxString line;
   wxString kind;

   kind = (*f)[f->GetCurrentLine()];

   if (kind == L"WaveTrack") {
      xmlFile.StartTag(L"wavetrack");
      xmlFile.WriteAttr(L"name", f->GetNextLine());

      wxString channel = f->GetNextLine();
      if (channel == L"left") {
         xmlFile.WriteAttr(L"channel", 0);
         line = f->GetNextLine();
      }
      else if (channel == L"right") {
         xmlFile.WriteAttr(L"channel", 1);
         line = f->GetNextLine();
      }
      else if (channel == L"mono") {
         xmlFile.WriteAttr(L"channel", 2);
         line = f->GetNextLine();
      }
      else {
         xmlFile.WriteAttr(L"channel", 2);
         line = channel;
      }

      if (line == L"linked") {
         xmlFile.WriteAttr(L"linked", 1);
         line = f->GetNextLine();
      }

      if (line != L"offset")
         return false;
      xmlFile.WriteAttr(L"offset", f->GetNextLine());

      long envLen;

      if (f->GetNextLine() != L"EnvNumPoints")
         return false;
      line = f->GetNextLine();
      line.ToLong(&envLen);
      if (envLen < 0 || envLen > 10000)
         return false;

      size_t envStart = f->GetCurrentLine();
      if (f->GetLineCount() < envStart+(2*envLen)+1)
         return false;

      f->GoToLine(envStart+(2*envLen));
      if (f->GetNextLine() != L"EnvEnd")
         return false;
      if (f->GetNextLine() != L"numSamples")
         return false;

      wxString numSamples = f->GetNextLine();

      if (f->GetNextLine() != L"rate")
         return false;

      xmlFile.WriteAttr(L"rate", f->GetNextLine());

      if (envLen > 0) {
         xmlFile.StartTag(L"envelope");
         xmlFile.WriteAttr(L"numpoints", envLen);

         long i;
         for(i=0; i<envLen; i++) {
            xmlFile.StartTag(L"controlpoint");
            xmlFile.WriteAttr(L"t", f->GetLine(envStart + 2*i + 1));
            xmlFile.WriteAttr(L"val", f->GetLine(envStart + 2*i + 2));
            xmlFile.EndTag(L"controlpoint");
         }

         xmlFile.EndTag(L"envelope");
      }

      if (f->GetNextLine() != L"numBlocks")
         return false;
      long numBlocks;
      line = f->GetNextLine();
      line.ToLong(&numBlocks);

      if (numBlocks < 0 || numBlocks > 131072)
         return false;

      xmlFile.StartTag(L"sequence");
      xmlFile.WriteAttr(L"maxsamples", 524288);
      xmlFile.WriteAttr(L"sampleformat", 131073);
      xmlFile.WriteAttr(L"numsamples", numSamples);

      long b;
      for(b=0; b<numBlocks; b++) {
         wxString start;
         wxString len;
         wxString name;

         if (f->GetNextLine() != L"Block start")
            return false;
         start = f->GetNextLine();
         if (f->GetNextLine() != L"Block len")
            return false;
         len = f->GetNextLine();
         if (f->GetNextLine() != L"Block info")
            return false;
         name = f->GetNextLine();

         xmlFile.StartTag(L"waveblock");
         xmlFile.WriteAttr(L"start", start);

         xmlFile.StartTag(L"legacyblockfile");
         if (name == L"Alias") {
            wxString aliasPath = f->GetNextLine();
            wxString localLen = f->GetNextLine();
            wxString aliasStart = f->GetNextLine();
            wxString aliasLen = f->GetNextLine();
            wxString aliasChannel = f->GetNextLine();
            wxString localName = f->GetNextLine();

            xmlFile.WriteAttr(L"name", localName);
            xmlFile.WriteAttr(L"alias", 1);
            xmlFile.WriteAttr(L"aliaspath", aliasPath);

            // This was written but not read again?
            xmlFile.WriteAttr(L"aliasstart", aliasStart);

            xmlFile.WriteAttr(L"aliaslen", aliasLen);
            xmlFile.WriteAttr(L"aliaschannel", aliasChannel);
            xmlFile.WriteAttr(L"summarylen", localLen);
            xmlFile.WriteAttr(L"norms", 1);
         }
         else {
            xmlFile.WriteAttr(L"name", name);
            xmlFile.WriteAttr(L"len", len);
            xmlFile.WriteAttr(L"summarylen", 8244);
            xmlFile.WriteAttr(L"norms", 1);
         }
         xmlFile.EndTag(L"legacyblockfile");

         xmlFile.EndTag(L"waveblock");
      }

      xmlFile.EndTag(L"sequence");
      xmlFile.EndTag(L"wavetrack");

      return true;
   }
   else if (kind == L"LabelTrack") {
      line = f->GetNextLine();
      if (line != L"NumMLabels")
         return false;

      long numLabels, l;

      line = f->GetNextLine();
      line.ToLong(&numLabels);
      if (numLabels < 0 || numLabels > 1000000)
         return false;

      xmlFile.StartTag(L"labeltrack");
      xmlFile.WriteAttr(L"name", L"Labels");
      xmlFile.WriteAttr(L"numlabels", numLabels);

      for(l=0; l<numLabels; l++) {
         wxString t, title;

         t = f->GetNextLine();
         title = f->GetNextLine();

         xmlFile.StartTag(L"label");
         xmlFile.WriteAttr(L"t", t);
         xmlFile.WriteAttr(L"title", title);
         xmlFile.EndTag(L"label");
      }

      xmlFile.EndTag(L"labeltrack");

      line = f->GetNextLine();
      if (line != L"MLabelsEnd")
         return false;

      return true;
   }
   else if (kind == L"NoteTrack") {
      // Just skip over it - they didn't even work in version 1.0!

      do {
         line = f->GetNextLine();
         if (line == L"WaveTrack" ||
             line == L"NoteTrack" ||
             line == L"LabelTrack" ||
             line == L"EndTracks") {
            f->GoToLine(f->GetCurrentLine()-1);
            return true;
         }
      } while (f->GetCurrentLine() < f->GetLineCount());

      return false;
   }
   else
      return false;
}

bool ConvertLegacyProjectFile(const wxFileName &filename)
{
   wxTextFile f;

   const wxString name = filename.GetFullPath();
   f.Open( name );
   if (!f.IsOpened())
      return false;

   return GuardedCall< bool >( [&] {
      XMLFileWriter xmlFile{ name, XO("Error Converting Legacy Project File") };

      xmlFile.Write(L"<?xml version=\"1.0\"?>\n");

      wxString label;
      wxString value;

      if (f.GetFirstLine() != L"AudacityProject")
         return false;
      if (f.GetNextLine() != L"Version")
         return false;
      if (f.GetNextLine() != L"0.95")
         return false;
      if (f.GetNextLine() != L"projName")
         return false;

      xmlFile.StartTag(L"audacityproject");
      xmlFile.WriteAttr(L"projname", f.GetNextLine());
      xmlFile.WriteAttr(L"version", L"1.1.0");
      xmlFile.WriteAttr(L"audacityversion",AUDACITY_VERSION_STRING);

      label = f.GetNextLine();
      while (label != L"BeginTracks") {
         xmlFile.WriteAttr(label, f.GetNextLine());
         label = f.GetNextLine();
      }

      label = f.GetNextLine();
      while (label != L"EndTracks") {
         bool success = ConvertLegacyTrack(&f, xmlFile);
         if (!success)
            return false;
         label = f.GetNextLine();
      }

      // Close original before Commit() tries to overwrite it.
      f.Close();

      xmlFile.EndTag(L"audacityproject");
      xmlFile.Commit();

      ::AudacityMessageBox(
         XO(
"Converted a 1.0 project file to the new format.\nThe old file has been saved as '%s'")
            .Format( xmlFile.GetBackupName() ),
         XO("Opening Audacity Project"));

      return true;
   } );
}
