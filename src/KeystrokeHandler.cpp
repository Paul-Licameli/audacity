/**********************************************************************

  Audacity: A Digital Audio Editor

  @file KeystrokeHandler.cpp

  Paul Licameli split from CommandManager.cpp

**********************************************************************/
#include "KeystrokeHandler.h"

#include "ActiveProject.h"
#include "KeyboardCapture.h"
#include "Menus.h"
#include "Project.h"
#include "ProjectCommandManager.h"
#include "ProjectWindows.h"
#include "CommandManagerWindowClasses.h"
#include "Keyboard.h"

#include <wx/evtloop.h>
#include <wx/frame.h>

NormalizedKeyString KeyEventToKeyString(const wxKeyEvent & event)
{
   wxString newStr;

   long key = event.GetKeyCode();

   if (event.ControlDown())
      newStr += wxT("Ctrl+");

   if (event.AltDown())
      newStr += wxT("Alt+");

   if (event.ShiftDown())
      newStr += wxT("Shift+");

#if defined(__WXMAC__)
   if (event.RawControlDown())
      newStr += wxT("RawCtrl+");
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
         newStr += wxT("Backspace");
         break;
      case WXK_DELETE:
         newStr += wxT("Delete");
         break;
      case WXK_SPACE:
         newStr += wxT("Space");
         break;
      case WXK_TAB:
         newStr += wxT("Tab");
         break;
      case WXK_RETURN:
         newStr += wxT("Return");
         break;
      case WXK_PAGEUP:
         newStr += wxT("PageUp");
         break;
      case WXK_PAGEDOWN:
         newStr += wxT("PageDown");
         break;
      case WXK_END:
         newStr += wxT("End");
         break;
      case WXK_HOME:
         newStr += wxT("Home");
         break;
      case WXK_LEFT:
         newStr += wxT("Left");
         break;
      case WXK_UP:
         newStr += wxT("Up");
         break;
      case WXK_RIGHT:
         newStr += wxT("Right");
         break;
      case WXK_DOWN:
         newStr += wxT("Down");
         break;
      case WXK_ESCAPE:
         newStr += wxT("Escape");
         break;
      case WXK_INSERT:
         newStr += wxT("Insert");
         break;
      case WXK_NUMPAD0:
         newStr += wxT("NUMPAD0");
         break;
      case WXK_NUMPAD1:
         newStr += wxT("NUMPAD1");
         break;
      case WXK_NUMPAD2:
         newStr += wxT("NUMPAD2");
         break;
      case WXK_NUMPAD3:
         newStr += wxT("NUMPAD3");
         break;
      case WXK_NUMPAD4:
         newStr += wxT("NUMPAD4");
         break;
      case WXK_NUMPAD5:
         newStr += wxT("NUMPAD5");
         break;
      case WXK_NUMPAD6:
         newStr += wxT("NUMPAD6");
         break;
      case WXK_NUMPAD7:
         newStr += wxT("NUMPAD7");
         break;
      case WXK_NUMPAD8:
         newStr += wxT("NUMPAD8");
         break;
      case WXK_NUMPAD9:
         newStr += wxT("NUMPAD9");
         break;
      case WXK_MULTIPLY:
         newStr += wxT("*");
         break;
      case WXK_ADD:
         newStr += wxT("+");
         break;
      case WXK_SUBTRACT:
         newStr += wxT("-");
         break;
      case WXK_DECIMAL:
         newStr += wxT(".");
         break;
      case WXK_DIVIDE:
         newStr += wxT("/");
         break;
      case WXK_F1:
         newStr += wxT("F1");
         break;
      case WXK_F2:
         newStr += wxT("F2");
         break;
      case WXK_F3:
         newStr += wxT("F3");
         break;
      case WXK_F4:
         newStr += wxT("F4");
         break;
      case WXK_F5:
         newStr += wxT("F5");
         break;
      case WXK_F6:
         newStr += wxT("F6");
         break;
      case WXK_F7:
         newStr += wxT("F7");
         break;
      case WXK_F8:
         newStr += wxT("F8");
         break;
      case WXK_F9:
         newStr += wxT("F9");
         break;
      case WXK_F10:
         newStr += wxT("F10");
         break;
      case WXK_F11:
         newStr += wxT("F11");
         break;
      case WXK_F12:
         newStr += wxT("F12");
         break;
      case WXK_F13:
         newStr += wxT("F13");
         break;
      case WXK_F14:
         newStr += wxT("F14");
         break;
      case WXK_F15:
         newStr += wxT("F15");
         break;
      case WXK_F16:
         newStr += wxT("F16");
         break;
      case WXK_F17:
         newStr += wxT("F17");
         break;
      case WXK_F18:
         newStr += wxT("F18");
         break;
      case WXK_F19:
         newStr += wxT("F19");
         break;
      case WXK_F20:
         newStr += wxT("F20");
         break;
      case WXK_F21:
         newStr += wxT("F21");
         break;
      case WXK_F22:
         newStr += wxT("F22");
         break;
      case WXK_F23:
         newStr += wxT("F23");
         break;
      case WXK_F24:
         newStr += wxT("F24");
         break;
      case WXK_NUMPAD_ENTER:
         newStr += wxT("NUMPAD_ENTER");
         break;
      case WXK_NUMPAD_F1:
         newStr += wxT("NUMPAD_F1");
         break;
      case WXK_NUMPAD_F2:
         newStr += wxT("NUMPAD_F2");
         break;
      case WXK_NUMPAD_F3:
         newStr += wxT("NUMPAD_F3");
         break;
      case WXK_NUMPAD_F4:
         newStr += wxT("NUMPAD_F4");
         break;
      case WXK_NUMPAD_HOME:
         newStr += wxT("NUMPAD_HOME");
         break;
      case WXK_NUMPAD_LEFT:
         newStr += wxT("NUMPAD_LEFT");
         break;
      case WXK_NUMPAD_UP:
         newStr += wxT("NUMPAD_UP");
         break;
      case WXK_NUMPAD_RIGHT:
         newStr += wxT("NUMPAD_RIGHT");
         break;
      case WXK_NUMPAD_DOWN:
         newStr += wxT("NUMPAD_DOWN");
         break;
      case WXK_NUMPAD_PAGEUP:
         newStr += wxT("NUMPAD_PAGEUP");
         break;
      case WXK_NUMPAD_PAGEDOWN:
         newStr += wxT("NUMPAD_PAGEDOWN");
         break;
      case WXK_NUMPAD_END:
         newStr += wxT("NUMPAD_END");
         break;
      case WXK_NUMPAD_BEGIN:
         newStr += wxT("NUMPAD_HOME");
         break;
      case WXK_NUMPAD_INSERT:
         newStr += wxT("NUMPAD_INSERT");
         break;
      case WXK_NUMPAD_DELETE:
         newStr += wxT("NUMPAD_DELETE");
         break;
      case WXK_NUMPAD_EQUAL:
         newStr += wxT("NUMPAD_EQUAL");
         break;
      case WXK_NUMPAD_MULTIPLY:
         newStr += wxT("NUMPAD_MULTIPLY");
         break;
      case WXK_NUMPAD_ADD:
         newStr += wxT("NUMPAD_ADD");
         break;
      case WXK_NUMPAD_SUBTRACT:
         newStr += wxT("NUMPAD_SUBTRACT");
         break;
      case WXK_NUMPAD_DECIMAL:
         newStr += wxT("NUMPAD_DECIMAL");
         break;
      case WXK_NUMPAD_DIVIDE:
         newStr += wxT("NUMPAD_DIVIDE");
         break;
      default:
         return {}; // Don't do anything if we don't recognize the key
      }
   }

   return NormalizedKeyString{ newStr };
}

NonKeystrokeInterceptingWindow::~NonKeystrokeInterceptingWindow()
{
}

TopLevelKeystrokeHandlingWindow::~TopLevelKeystrokeHandlingWindow()
{
}

bool FilterKeyEvent(AudacityProject *project, const wxKeyEvent & evt, bool permit)
{
   if (!project)
      return false;

   auto &cm = ProjectCommandManager::Get(*project);
   
   auto pWindow = FindProjectFrame( project );
   const auto keyString = KeyEventToKeyString(evt);
   auto entry = cm.Lookup(keyString);
   if (entry == NULL)
   {
      return false;
   }

   int type = evt.GetEventType();

   // Global commands aren't tied to any specific project
   if (entry->isGlobal && type == wxEVT_KEY_DOWN)
   {
      // Global commands are always disabled so they do not interfere with the
      // rest of the command handling.  But, to use the common handler, we
      // enable them temporarily and then disable them again after handling.
      // LL:  Why do they need to be disabled???
      entry->enabled = false;
      auto cleanup = valueRestorer( entry->enabled, true );
      return
         cm.HandleCommandEntry(*project, entry, NoFlagsSpecified, false, &evt);
   }

   wxWindow * pFocus = wxWindow::FindFocus();
   wxWindow * pParent = wxGetTopLevelParent( pFocus );
   bool validTarget = pParent == pWindow;
   // Bug 1557.  MixerBoard should count as 'destined for project'
   // MixerBoard IS a TopLevelWindow, and its parent is the project.
   if( pParent && pParent->GetParent() == pWindow ){
      if( dynamic_cast< TopLevelKeystrokeHandlingWindow* >( pParent ) != NULL )
         validTarget = true;
   }
   validTarget = validTarget && wxEventLoop::GetActive()->IsMain();

   // Any other keypresses must be destined for this project window
   if (!permit && !validTarget )
   {
      return false;
   }

   auto flags = MenuManager::Get(*project).GetUpdateFlags();

   wxKeyEvent temp = evt;

   // Possibly let wxWidgets do its normal key handling IF it is one of
   // the standard navigation keys.
   if((type == wxEVT_KEY_DOWN) || (type == wxEVT_KEY_UP ))
   {
      wxWindow * pWnd = wxWindow::FindFocus();
      bool bIntercept =
         pWnd && !dynamic_cast< NonKeystrokeInterceptingWindow * >( pWnd );

      //wxLogDebug("Focus: %p TrackPanel: %p", pWnd, pTrackPanel );
      // We allow the keystrokes below to be handled by wxWidgets controls IF we are
      // in some sub window rather than in the TrackPanel itself.
      // Otherwise they will go to our command handler and if it handles them
      // they will NOT be available to wxWidgets.
      if( bIntercept ){
         switch( evt.GetKeyCode() ){
         case WXK_LEFT:
         case WXK_RIGHT:
         case WXK_UP:
         case WXK_DOWN:
         // Don't trap WXK_SPACE (Bug 1727 - SPACE not starting/stopping playback
         // when cursor is in a time control)
         // case WXK_SPACE:
         case WXK_TAB:
         case WXK_BACK:
         case WXK_HOME:
         case WXK_END:
         case WXK_RETURN:
         case WXK_NUMPAD_ENTER:
         case WXK_DELETE:
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            return false;
         }
      }
   }

   if (type == wxEVT_KEY_DOWN)
   {
      if (entry->skipKeydown)
      {
         return true;
      }
      return cm.HandleCommandEntry(*project, entry, flags, false, &temp);
   }

   if (type == wxEVT_KEY_UP && entry->wantKeyup)
   {
      return cm.HandleCommandEntry(*project, entry, flags, false, &temp);
   }

   return false;
}

static struct InstallHandlers
{
   InstallHandlers()
   {
      KeyboardCapture::SetPreFilter( []( wxKeyEvent & ) {
         // We must have a project since we will be working with the
         // CommandManager, which is tied to individual projects.
         AudacityProject *project = GetActiveProject();
         return project && GetProjectFrame( *project ).IsEnabled();
      } );
      KeyboardCapture::SetPostFilter( []( wxKeyEvent &key ) {
         // Capture handler window didn't want it, so ask the CommandManager.
         AudacityProject *project = GetActiveProject();
         return FilterKeyEvent(project, key, false);
      } );
   }
} installHandlers;
