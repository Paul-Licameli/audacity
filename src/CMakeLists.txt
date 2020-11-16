
set( TARGET Audacity )
set( TARGET_ROOT ${topdir}/src )

message( STATUS "========== Configuring ${TARGET} ==========" )

# Allow modules to link against the Audacity executable
set( CMAKE_ENABLE_EXPORTS ON )

add_executable( ${TARGET} )
add_dependencies( ${TARGET} locale )
add_dependencies( ${TARGET} nyquist )
add_dependencies( ${TARGET} plug-ins )

def_vars()

# Add a target that will provide the git revision info
# whenever it changes.  (Must be done at build time, not
# configuration time.)
if( GIT_FOUND )
   add_custom_target(
      version
      COMMAND
         ${CMAKE_COMMAND} -D GIT="${GIT_EXECUTABLE}"
                          -D "_PRVDIR=${_PRVDIR}"
                          -P "${AUDACITY_MODULE_PATH}/Version.cmake"
      WORKING_DIRECTORY
         ${topdir}
   )

   add_dependencies( ${TARGET} version )
endif()

# Handle Audio Units option
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   cmd_option(
      ${_OPT}use_audio_units
      "Use Audio Units plug-in support [on, off]"
      ON
   )
   set( USE_AUDIO_UNITS ${${_OPT}use_audio_units} CACHE INTERNAL "" )
endif()

# Detect, which version of FileDialog to use for
# We always assume that GTK is used on platforms other than Windows and macOS,
# as there is no other implementation avalaible now
if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   set( wxIS_MAC on )
elseif( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   set ( wxIS_WIN on )
else()
   set ( wxIS_GTK on)
endif()

#
#
#
list( APPEND SOURCES
   PRIVATE
      ActiveProject.cpp
      ActiveProject.h
      ActiveProjects.cpp
      ActiveProjects.h
      AboutDialog.cpp
      AboutDialog.h
      AudacityApp.cpp
      AudacityApp.h
      $<$<BOOL:${wxIS_MAC}>:AudacityApp.mm>
      AudacityFileConfig.cpp
      AudacityFileConfig.h
      AudacityHeaders.cpp
      AudacityHeaders.h
      AudioIO.cpp
      AudioIO.h
      AudioIOBase.cpp
      AudioIOBase.h
      AudioIOExt.cpp
      AudioIOExt.h
      AudioIOListener.h
      AutoRecoveryDialog.cpp
      AutoRecoveryDialog.h
      Benchmark.cpp
      Benchmark.h
      Clipboard.cpp
      Clipboard.h
      CommonCommandFlags.cpp
      CommonCommandFlags.h
      CrashReport.cpp
      CrashReport.h
      DBConnection.cpp
      DBConnection.h
      DefaultCommandOutputTargets.cpp
      DefaultCommandOutputTargets.h
      DeviceChange.cpp
      DeviceChange.h
      DeviceManager.cpp
      DeviceManager.h
      Diags.cpp
      Diags.h
      HelpUtilities.cpp
      HelpUtilities.h
      KeystrokeHandler.cpp
      KeystrokeHandler.h
      LangChoice.cpp
      LangChoice.h
      Legacy.cpp
      Legacy.h
      LogWindow.cpp
      LogWindow.h
      Mix.cpp
      Mix.h
      ModuleManager.cpp
      ModuleManager.h
      ModuleSettings.cpp
      ModuleSettings.h
      PitchName.cpp
      PitchName.h
      PlaybackSchedule.cpp
      PlaybackSchedule.h
      PluginManager.cpp
      PluginManager.h
      Profiler.cpp
      Profiler.h
      ProjectAudioIO.cpp
      ProjectAudioIO.h
      ProjectAudioManager.cpp
      ProjectAudioManager.h
      ProjectCommandManager.cpp
      ProjectCommandManager.h
      ProjectFileIO.cpp
      ProjectFileIO.h
      ProjectFileManager.cpp
      ProjectFileManager.h
      ProjectHistory.cpp
      ProjectHistory.h
      ProjectManager.cpp
      ProjectManager.h
      ProjectSelectionManager.cpp
      ProjectSelectionManager.h
      ProjectSerializer.cpp
      ProjectSerializer.h
      ProjectSettings.cpp
      ProjectSettings.h
      ProjectWindow.cpp
      ProjectWindow.h
      RecordUtilities.cpp
      RecordUtilities.h
      ReloadPreferences.cpp
      ReloadPreferences.h
      RingBuffer.cpp
      RingBuffer.h
      SampleBlock.cpp
      SampleBlock.h
      SampleTrack.cpp
      SampleTrack.h
      SampleTrackCache.cpp
      SampleTrackCache.h
      SelectUtilities.cpp
      SelectUtilities.h
      SelectFile.cpp
      SelectFile.h
      SelectionState.cpp
      SelectionState.h
      Sequence.cpp
      Sequence.h
      ShuttleAutomation.cpp
      ShuttleAutomation.h
      Snap.cpp
      Snap.h
      SoundActivatedRecord.cpp
      SoundActivatedRecord.h
      SpectrumAnalyst.cpp
      SpectrumAnalyst.h
      SplashDialog.cpp
      SplashDialog.h
      SqliteSampleBlock.cpp
      SseMathFuncs.cpp
      SseMathFuncs.h
      SyncLock.cpp
      SyncLock.h
      TimeDialog.cpp
      TimeDialog.h
      TrackArtist.cpp
      TrackArtist.h
      TrackPanelAx.cpp
      TrackPanelAx.h
      TrackPanelListener.h
      TrackUtilities.cpp
      TrackUtilities.h
      TransportUtilities.cpp
      TransportUtilities.h
      UndoRedoMenu.cpp
      UndoRedoMenu.h
      UndoManager.cpp
      UndoManager.h
      WaveClip.cpp
      WaveClip.h
      WaveTrack.cpp
      WaveTrack.h

      # Commands

      commands/AudacityCommand.cpp
      commands/AudacityCommand.h
      commands/CommandDispatch.cpp
      commands/CommandDispatch.h
      commands/ResponseQueue.cpp
      commands/ResponseQueue.h

      # Built-in Effects

      effects/RealtimeEffectManager.cpp
      effects/RealtimeEffectManager.h
      effects/ScoreAlignDialog.cpp
      effects/ScoreAlignDialog.h

      # Menus

      menus/EditMenus.cpp
      menus/ExtraMenus.cpp
      menus/FileMenus.cpp
      menus/RecordMenus.cpp
      menus/TransportMenus.cpp

      # Preferences

      prefs/DirectoriesPrefs.cpp
      prefs/DirectoriesPrefs.h
      prefs/GUISettings.cpp
      prefs/GUISettings.h
      prefs/ModulePrefs.cpp
      prefs/ModulePrefs.h
      prefs/QualitySettings.cpp
      prefs/QualitySettings.h

      # Tracks UI

      tracks/ui/CommonTrackPanelCell.cpp
      tracks/ui/CommonTrackPanelCell.h
      tracks/ui/Scrubbing.cpp
      tracks/ui/Scrubbing.h
      tracks/ui/TrackControls.cpp
      tracks/ui/TrackControls.h
      tracks/ui/TrackVRulerControls.cpp
      tracks/ui/TrackVRulerControls.h
      tracks/ui/TrackView.cpp
      tracks/ui/TrackView.h

      # ui helpers
      ui/AccessibleLinksFormatter.h
      ui/AccessibleLinksFormatter.cpp

      # Widgets

      widgets/AButton.cpp
      widgets/AButton.h
      widgets/ASlider.cpp
      widgets/ASlider.h
      widgets/AttachableScrollBar.cpp
      widgets/AttachableScrollBar.h
      widgets/AudacityMessageBox.cpp
      widgets/AudacityMessageBox.h
      widgets/ErrorDialog.cpp
      widgets/ErrorDialog.h
      widgets/ExpandingToolBar.cpp
      widgets/ExpandingToolBar.h
      widgets/FileHistory.cpp
      widgets/FileHistory.h
      widgets/Grid.cpp
      widgets/Grid.h
      widgets/ImageRoll.cpp
      widgets/ImageRoll.h
      widgets/Meter.cpp
      widgets/Meter.h
      widgets/MeterPanelBase.cpp
      widgets/MeterPanelBase.h
      widgets/MultiDialog.cpp
      widgets/MultiDialog.h
      widgets/NumericTextCtrl.cpp
      widgets/NumericTextCtrl.h
      widgets/PopupMenuTable.cpp
      widgets/PopupMenuTable.h
      widgets/ProgressDialog.cpp
      widgets/ProgressDialog.h
      widgets/Ruler.cpp
      widgets/Ruler.h
      $<$<BOOL:${${_OPT}has_sentry_reporting}>:
         widgets/ErrorReportDialog.cpp
         widgets/ErrorReportDialog.h
      >
      widgets/UnwritableLocationErrorDialog.cpp
      widgets/UnwritableLocationErrorDialog.h
      widgets/VetoDialogHook.cpp
      widgets/VetoDialogHook.h
      widgets/Warning.cpp
      widgets/Warning.h
      widgets/auStaticText.cpp
      widgets/auStaticText.h
      widgets/numformatter.cpp
      widgets/numformatter.h
      widgets/valnum.cpp
      widgets/valnum.h
      widgets/wxWidgetsBasicUI.cpp
      widgets/wxWidgetsBasicUI.h

      # Update version
      $<$<BOOL:${${_OPT}has_updates_check}>:
          update/VersionId.h
          update/VersionId.cpp
          update/VersionPatch.h
          update/UpdateDataParser.h
          update/UpdateDataParser.cpp
          update/UpdateManager.h
          update/UpdateManager.cpp
          update/UpdateNoticeDialog.h
          update/UpdateNoticeDialog.cpp
          update/UpdatePopupDialog.h
          update/UpdatePopupDialog.cpp
          prefs/ApplicationPrefs.h
          prefs/ApplicationPrefs.cpp
      >
)

#
#
#
list( APPEND HEADERS
   ../include/audacity/Types.h
)

#
#
#
list( APPEND INCLUDES
   PUBLIC
      ${_PRVDIR}
      ${topdir}/include
      ${TARGET_ROOT}
)

#
# Define resources
#
list( APPEND RESOURCES
   ../presets/EQDefaultCurves.xml
)

#
#
#
list( APPEND DEFINES
   PRIVATE
      BUILDING_AUDACITY
      WXUSINGDLL
      CMAKE
      $<$<BOOL:${HAVE_LRINT}>:
         HAVE_LRINT
      >
      $<$<BOOL:${HAVE_LRINTF}>:
         HAVE_LRINTF
      >
      $<$<BOOL:${HAVE_MLOCK}>:
         HAVE_MLOCK
      >
      $<$<PLATFORM_ID:Windows>:
         _CRT_SECURE_NO_WARNINGS
         __STDC_CONSTANT_MACROS
         STRICT
      >
      $<$<BOOL:${${_OPT}has_updates_check}>:
          HAVE_UPDATES_CHECK
      >
)

# If we have cmake 3.16 or higher, we can use precompiled headers, but
# only use them if ccache is not available and the user hasn't disabled
# it.
if( CMAKE_VERSION VERSION_GREATER_EQUAL "3.16" AND NOT CCACHE_PROGRAM )
   cmd_option(
      ${_OPT}use_pch
      "Use precompiled headers [yes, no]"
      YES
   )
endif()

audacity_append_common_compiler_options( OPTIONS "${${_OPT}use_pch}" )

list( APPEND LDFLAGS
   PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>
      $<$<CXX_COMPILER_ID:GNU>:-Wl,--disable-new-dtags>
)

#
#
#
list( APPEND LIBRARIES
   PUBLIC
      ${CMAKE_REQUIRED_LIBRARIES}
      ZLIB::ZLIB
      wxwidgets::wxwidgets
      expat::expat
      portaudio-v19
      sqlite
      $<$<BOOL:${${_OPT}has_crashreports}>:crashreports>
      $<$<BOOL:${USE_LIBID3TAG}>:libid3tag::libid3tag>
      $<$<BOOL:${USE_LIBMAD}>:libmad::libmad>
      $<$<BOOL:${USE_LIBOGG}>:libogg>
      $<$<BOOL:${USE_PORTMIXER}>:portmixer>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:PkgConfig::GLIB>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:PkgConfig::GTK>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:z>
      $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:pthread>
)

set( BUILDING_AUDACITY YES )
set( INSTALL_PREFIX "${_PREFIX}" )
set( PKGLIBDIR "${_PKGLIBDIR}" )
set( LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}" )
set( HAVE_GTK ${GTK_FOUND} )

if( CMAKE_SYSTEM_NAME MATCHES "Windows" )
   set_target_properties(
      ${TARGET}
      PROPERTIES
         # Make sure we're a GUI application and not a console tool
         WIN32_EXECUTABLE ON
   )

   # Define the Windows specific resources
   list( APPEND WIN_RESOURCES
      ../win/audacity.rc
   )

   # Copy over the wxWidgets DLLs

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/Audacity.exe"
                          -D DST="${_EXEDIR}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>/"
                          -P ${AUDACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )

   # Copy the VC runtime libraries as well
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${_DEST}
      POST_BUILD
   )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   # Bug 2400 workaround
   #
   # Replaces the SDK version in the built executable with 10.13 to
   # prevent high CPU usage and slow drawing on Mojave or newer
   check_for_platform_version()
   if( PLATFORM_VERSION_SUPPORTED )
      list( APPEND LDFLAGS
         PRIVATE
            -Wl,-platform_version,macos,${MIN_MACOS_VERSION},${TARGET_MACOS_VERSION}
      )
   else()
      list( APPEND LDFLAGS
         PRIVATE
            -Wl,-sdk_version,10.13
      )
   endif()

   # Define Mac specific resources
   list( APPEND MAC_RESOURCES
      ../mac/Resources/Audacity.icns
      ../mac/Resources/AudacityAIFF.icns
      ../mac/Resources/AudacityAU.icns
      ../mac/Resources/AudacityMP3.icns
      ../mac/Resources/AudacityProject.icns
      ../mac/Resources/AudacityWAV.icns
   )

   set_target_properties(
      ${TARGET}
      PROPERTIES
         # Build a bundle
         MACOSX_BUNDLE ON
         # Add the resources
         RESOURCE "${MAC_RESOURCES};${RESOURCES}"
   )
   set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_DESTDIR}" )

   # These sources must be compiled as Objective C++ and excluded
   # from precompiled header usage
   set_source_files_properties(
      AudacityApp.mm
      effects/EffectUI.cpp
      menus/WindowMenus.cpp
      widgets/FileDialog/mac/FileDialogPrivate.mm
      PROPERTIES
         COMPILE_FLAGS "-x objective-c++"
         SKIP_PRECOMPILE_HEADERS YES
   )

   # Add our required frameworks
   list( APPEND LIBRARIES
      PRIVATE
         "-framework AudioUnit"
         "-framework CoreAudio"
         "-framework CoreAudioKit"
   )

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/Audacity"
                          -D DST="${_PKGLIB}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>"
                          -P ${AUDACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )

   # Define the Wrapper target
   set( WRAPPER_ROOT "${TARGET_ROOT}/../mac" )
   set( WRAPPER_SOURCES "${WRAPPER_ROOT}/Wrapper.c" )

   add_executable( Wrapper "${WRAPPER_SOURCES}" )
   add_dependencies( "${TARGET}" Wrapper )

   set_target_property_all( "Wrapper" RUNTIME_OUTPUT_DIRECTORY "${_EXEDIR}" )
   organize_source( "${WRAPPER_ROOT}" "mac" "${WRAPPER_SOURCES}" )

else()
   set_target_property_all( ${TARGET} RUNTIME_OUTPUT_DIRECTORY "${_DEST}" )

   # Build the list of mimetypes
   #
   # (Don't use generator expressions since it will leave null/empty
   # entries in the list.)
   set( MIMETYPES
      application/x-audacity-project
   )
   # Create the desktop file
   configure_file( audacity.desktop.in ${_INTDIR}/audacity.desktop )

   # Copy the required wxWidgets libs into the bundle
   add_custom_command(
      TARGET
         ${TARGET}
      COMMAND
         ${CMAKE_COMMAND} -D SRC="${_EXEDIR}/audacity"
                          -D DST="${_DEST}/${_PKGLIB}"
                          -D WXWIN="${_SHARED_PROXY_BASE_PATH}/$<CONFIG>"
                          -P ${AUDACITY_MODULE_PATH}/CopyLibs.cmake
      POST_BUILD
   )
endif()

if(CRASH_REPORT_URL)
    list(APPEND DEFINES PRIVATE -DCRASH_REPORT_URL="${CRASH_REPORT_URL}")
endif()

set_target_property_all( ${TARGET} RUNTIME_OUTPUT_NAME ${AUDACITY_NAME} )

organize_source( "${TARGET_ROOT}/.." "include" "${HEADERS}" )
organize_source( "${TARGET_ROOT}/../presets" "presets" "${RESOURCES}" )
organize_source( "${TARGET_ROOT}" "src" "${SOURCES}" )
organize_source( "${TARGET_ROOT}/.." "mac/resources" "${MAC_RESOURCES}" )
organize_source( "${TARGET_ROOT}/../win" "win" "${WIN_RESOURCES}" )

#
# Get those pesky CMake files out of the way (on Windows)
#
source_group(
    TREE
        "${_INTDIR}"
    PREFIX
        "CMake"
    FILES
        ${_INTDIR}/CMakeFiles/Audacity.dir/cmake_pch.cxx
        ${_INTDIR}/CMakeFiles/Audacity.dir/cmake_pch.hxx
)

# Define AUDACITY_DLL_API
import_symbol_define( import_symbol AUDACITY_DLL )
export_symbol_define( export_symbol AUDACITY_DLL )
list( APPEND DEFINES PRIVATE "${export_symbol}" INTERFACE "${import_symbol}" )

target_sources( ${TARGET} PRIVATE ${HEADERS} ${SOURCES} ${RESOURCES} ${MAC_RESOURCES} ${WIN_RESOURCES} )
target_compile_definitions( ${TARGET} PRIVATE ${DEFINES} )
target_compile_options( ${TARGET} PRIVATE ${OPTIONS} )
target_include_directories( ${TARGET} PRIVATE ${INCLUDES} )
target_link_options( ${TARGET} PRIVATE ${LDFLAGS} )
target_link_libraries( ${TARGET} PUBLIC ${AUDACITY_LIBRARIES} )
target_link_libraries( ${TARGET} ${LIBRARIES} )

if( CMAKE_VERSION VERSION_GREATER_EQUAL "3.16" AND NOT CCACHE_PROGRAM )
   if( ${_OPT}use_pch )
      message( STATUS "Using precompiled headers" )
      target_precompile_headers( ${TARGET} PRIVATE
         $<$<PLATFORM_ID:Windows>:${CMAKE_BINARY_DIR}/src/private/configwin.h>
         $<$<PLATFORM_ID:Darwin>:${CMAKE_BINARY_DIR}/src/private/configmac.h>
         $<$<NOT:$<PLATFORM_ID:Windows,Darwin>>:${CMAKE_BINARY_DIR}/src/private/configunix.h>
         AudacityHeaders.h
      )
   else()
      message( STATUS "Not using precompiled headers" )
   endif()
endif()

if( "${CMAKE_GENERATOR}" MATCHES "Xcode|Visual Studio*" )
   install(
      DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/"
      DESTINATION "."
      USE_SOURCE_PERMISSIONS
      PATTERN "*.pdb" EXCLUDE
      PATTERN "*.ilk" EXCLUDE
      PATTERN "*.dSYM" EXCLUDE
   )
else()
   if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
      install( TARGETS ${TARGET}
               DESTINATION "."
               RESOURCE DESTINATION "${_APPDIR}/Resources" )
   else()
      install( TARGETS ${TARGET} RUNTIME )
      install( DIRECTORY "${_DEST}/${_LIBDIR}/"
               DESTINATION "${_LIBDIR}"
               USE_SOURCE_PERMISSIONS
               FILES_MATCHING PATTERN "*.so*" )
      install( FILES "${_INTDIR}/audacity.desktop"
               DESTINATION "${_DATADIR}/applications" )
      install( FILES "${topdir}/LICENSE.txt" "${topdir}/README.txt"
               DESTINATION "${_DATADIR}/doc/${AUDACITY_NAME}" )
      install( FILES "${_SRCDIR}/audacity.xml"
               DESTINATION "${_DATADIR}/mime/packages" )
      install( FILES "${topdir}/presets/EQDefaultCurves.xml"
               DESTINATION "${_PKGDATA}" )
      install( PROGRAMS "${PROJECT_SOURCE_DIR}/linux/audacity.sh"
               DESTINATION "."
               RENAME "audacity" )
   endif()
endif()

# collect dependency information for third party libraries
list( APPEND GRAPH_EDGES "Audacity [shape=house]" )
foreach( LIBRARY ${LIBRARIES} )
   if (NOT LIBRARY MATCHES "PUBLIC|PRIVATE|INTERFACE")
      canonicalize_node_name(LIBRARY "${LIBRARY}")
      list( APPEND GRAPH_EDGES "\"${TARGET}\" -> \"${LIBRARY}\"" )
   endif()
endforeach()

#propagate collected edgets to root CMakeLists.txt
set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
