/**********************************************************************

  Audacity: A Digital Audio Editor

  Keyboard.cpp

  Dominic Mazzoni
  Brian Gunlogson

**********************************************************************/


#include "Keyboard.h"

#include <wx/event.h>

NormalizedKeyString::NormalizedKeyString( const wxString & key )
   : NormalizedKeyStringBase( key )
{
#if defined(__WXMAC__)
   wxString newkey;
   wxString temp = key;

   // PRL:  This is needed to parse older preference files.
   temp.Replace(L"XCtrl+", L"Control+");

   // PRL:  RawCtrl is the proper replacement for Control, when formatting
   // wxMenuItem, so that wxWidgets shows ^ in the menu.  It is written into
   // NEW preference files (2.2.0 and later).
   temp.Replace(L"RawCtrl+", L"Control+");
   temp.Replace(L"Ctrl+", L"Command+");

   if (temp.Contains(L"Control+")) {
      newkey += L"RawCtrl+";
   }

   if (temp.Contains(L"Alt+") || temp.Contains(L"Option+")) {
      newkey += L"Alt+";
   }

   if (temp.Contains(L"Shift+")) {
      newkey += L"Shift+";
   }

   if (temp.Contains(L"Command+")) {
      newkey += L"Ctrl+";
   }

   (NormalizedKeyStringBase&)*this =
      newkey + temp.AfterLast(L'+');
#else
   (NormalizedKeyStringBase&)*this = key;
#endif
}

DisplayKeyString NormalizedKeyString::Display(bool usesSpecialChars) const
{
   (void)usesSpecialChars;//compiler food
   // using GET to manipulate key string as needed for macOS differences
   // in displaying of it
   auto newkey = this->GET();
#if defined(__WXMAC__)

   if (!usesSpecialChars) {
      // Compose user-visible keystroke names, all ASCII
      newkey.Replace(L"RawCtrl+", L"Control+");
      newkey.Replace(L"Alt+", L"Option+");
      newkey.Replace(L"Ctrl+", L"Command+");
   }
   else {
      // Compose user-visible keystroke names, with special characters
      newkey.Replace(L"Shift+", L"\u21e7");
      newkey.Replace(L"RawCtrl+", '^');
      newkey.Replace(L"Alt+", L"\u2325");
      newkey.Replace(L"Ctrl+", L"\u2318");
   }

#endif

   return newkey;
}

NormalizedKeyString::NormalizedKeyString(
   const DisplayKeyString &str, bool usesSpecialChars)
{
   // Computes the inverse of the previous function

   (void)usesSpecialChars;//compiler food
   auto newkey = str.GET();
#if defined(__WXMAC__)

   if (!usesSpecialChars) {
      newkey.Replace(L"Control+", L"RawCtrl+");
      newkey.Replace(L"Option+", L"Alt+");
      newkey.Replace(L"Command+", L"Ctrl+");
   }
   else {
      newkey.Replace(L"\u21e7", L"Shift+");
      newkey.Replace('^', L"RawCtrl+");
      newkey.Replace(L"\u2325", L"Alt+");
      newkey.Replace(L"\u2318", L"Ctrl+");
   }

#endif

   Identifier::operator=( newkey );
}

NormalizedKeyString KeyEventToKeyString(const wxKeyEvent & event)
{
   wxString newStr;

   long key = event.GetKeyCode();

   if (event.ControlDown())
      newStr += L"Ctrl+";

   if (event.AltDown())
      newStr += L"Alt+";

   if (event.ShiftDown())
      newStr += L"Shift+";

#if defined(__WXMAC__)
   if (event.RawControlDown())
      newStr += L"RawCtrl+";
#endif

   if (event.RawControlDown() && key >= 1 && key <= 26)
      newStr += (wxChar)(64 + key);
   else if (key >= 33 && key <= 255 && key != 127)
      newStr += (wxChar)key;
   else
   {
      switch(key)
      {
      case WXK_BACK:
         newStr += L"Backspace";
         break;
      case WXK_DELETE:
         newStr += L"Delete";
         break;
      case WXK_SPACE:
         newStr += L"Space";
         break;
      case WXK_TAB:
         newStr += L"Tab";
         break;
      case WXK_RETURN:
         newStr += L"Return";
         break;
      case WXK_PAGEUP:
         newStr += L"PageUp";
         break;
      case WXK_PAGEDOWN:
         newStr += L"PageDown";
         break;
      case WXK_END:
         newStr += L"End";
         break;
      case WXK_HOME:
         newStr += L"Home";
         break;
      case WXK_LEFT:
         newStr += L"Left";
         break;
      case WXK_UP:
         newStr += L"Up";
         break;
      case WXK_RIGHT:
         newStr += L"Right";
         break;
      case WXK_DOWN:
         newStr += L"Down";
         break;
      case WXK_ESCAPE:
         newStr += L"Escape";
         break;
      case WXK_INSERT:
         newStr += L"Insert";
         break;
      case WXK_NUMPAD0:
         newStr += L"NUMPAD0";
         break;
      case WXK_NUMPAD1:
         newStr += L"NUMPAD1";
         break;
      case WXK_NUMPAD2:
         newStr += L"NUMPAD2";
         break;
      case WXK_NUMPAD3:
         newStr += L"NUMPAD3";
         break;
      case WXK_NUMPAD4:
         newStr += L"NUMPAD4";
         break;
      case WXK_NUMPAD5:
         newStr += L"NUMPAD5";
         break;
      case WXK_NUMPAD6:
         newStr += L"NUMPAD6";
         break;
      case WXK_NUMPAD7:
         newStr += L"NUMPAD7";
         break;
      case WXK_NUMPAD8:
         newStr += L"NUMPAD8";
         break;
      case WXK_NUMPAD9:
         newStr += L"NUMPAD9";
         break;
      case WXK_MULTIPLY:
         newStr += L"*";
         break;
      case WXK_ADD:
         newStr += L"+";
         break;
      case WXK_SUBTRACT:
         newStr += L"-";
         break;
      case WXK_DECIMAL:
         newStr += L".";
         break;
      case WXK_DIVIDE:
         newStr += L"/";
         break;
      case WXK_F1:
         newStr += L"F1";
         break;
      case WXK_F2:
         newStr += L"F2";
         break;
      case WXK_F3:
         newStr += L"F3";
         break;
      case WXK_F4:
         newStr += L"F4";
         break;
      case WXK_F5:
         newStr += L"F5";
         break;
      case WXK_F6:
         newStr += L"F6";
         break;
      case WXK_F7:
         newStr += L"F7";
         break;
      case WXK_F8:
         newStr += L"F8";
         break;
      case WXK_F9:
         newStr += L"F9";
         break;
      case WXK_F10:
         newStr += L"F10";
         break;
      case WXK_F11:
         newStr += L"F11";
         break;
      case WXK_F12:
         newStr += L"F12";
         break;
      case WXK_F13:
         newStr += L"F13";
         break;
      case WXK_F14:
         newStr += L"F14";
         break;
      case WXK_F15:
         newStr += L"F15";
         break;
      case WXK_F16:
         newStr += L"F16";
         break;
      case WXK_F17:
         newStr += L"F17";
         break;
      case WXK_F18:
         newStr += L"F18";
         break;
      case WXK_F19:
         newStr += L"F19";
         break;
      case WXK_F20:
         newStr += L"F20";
         break;
      case WXK_F21:
         newStr += L"F21";
         break;
      case WXK_F22:
         newStr += L"F22";
         break;
      case WXK_F23:
         newStr += L"F23";
         break;
      case WXK_F24:
         newStr += L"F24";
         break;
      case WXK_NUMPAD_ENTER:
         newStr += L"NUMPAD_ENTER";
         break;
      case WXK_NUMPAD_F1:
         newStr += L"NUMPAD_F1";
         break;
      case WXK_NUMPAD_F2:
         newStr += L"NUMPAD_F2";
         break;
      case WXK_NUMPAD_F3:
         newStr += L"NUMPAD_F3";
         break;
      case WXK_NUMPAD_F4:
         newStr += L"NUMPAD_F4";
         break;
      case WXK_NUMPAD_HOME:
         newStr += L"NUMPAD_HOME";
         break;
      case WXK_NUMPAD_LEFT:
         newStr += L"NUMPAD_LEFT";
         break;
      case WXK_NUMPAD_UP:
         newStr += L"NUMPAD_UP";
         break;
      case WXK_NUMPAD_RIGHT:
         newStr += L"NUMPAD_RIGHT";
         break;
      case WXK_NUMPAD_DOWN:
         newStr += L"NUMPAD_DOWN";
         break;
      case WXK_NUMPAD_PAGEUP:
         newStr += L"NUMPAD_PAGEUP";
         break;
      case WXK_NUMPAD_PAGEDOWN:
         newStr += L"NUMPAD_PAGEDOWN";
         break;
      case WXK_NUMPAD_END:
         newStr += L"NUMPAD_END";
         break;
      case WXK_NUMPAD_BEGIN:
         newStr += L"NUMPAD_HOME";
         break;
      case WXK_NUMPAD_INSERT:
         newStr += L"NUMPAD_INSERT";
         break;
      case WXK_NUMPAD_DELETE:
         newStr += L"NUMPAD_DELETE";
         break;
      case WXK_NUMPAD_EQUAL:
         newStr += L"NUMPAD_EQUAL";
         break;
      case WXK_NUMPAD_MULTIPLY:
         newStr += L"NUMPAD_MULTIPLY";
         break;
      case WXK_NUMPAD_ADD:
         newStr += L"NUMPAD_ADD";
         break;
      case WXK_NUMPAD_SUBTRACT:
         newStr += L"NUMPAD_SUBTRACT";
         break;
      case WXK_NUMPAD_DECIMAL:
         newStr += L"NUMPAD_DECIMAL";
         break;
      case WXK_NUMPAD_DIVIDE:
         newStr += L"NUMPAD_DIVIDE";
         break;
      default:
         return {}; // Don't do anything if we don't recognize the key
      }
   }

   return NormalizedKeyString{ newStr };
}
