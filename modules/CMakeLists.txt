# Include the modules that we'll build

# The list of modules is ordered so that each module occurs after any others
# that it depends on
set( MODULES
   mod-time-track
   mod-label-track
   mod-play-indicator-overlay
   mod-edit-cursor-overlay
   mod-scripting
   mod-common-track-ui
   mod-playable-track-ui
   mod-wave-track-ui
   mod-spectrum-ui
   mod-waveform-ui
   mod-tags
   mod-tags-ui
   mod-import-export
   mod-label-track-ui
   mod-time-track-ui
   mod-note-track-ui
   mod-control-tool-bar
   mod-selection-tool-bar
   mod-spectral-selection-tool-bar
   mod-time-tool-bar
   mod-scrubbing-tool-bar
   mod-tools-tool-bar
   mod-meter-tool-bar
   mod-edit-tool-bar
   mod-mixer-tool-bar
   mod-device-tool-bar
   mod-transcription-tool-bar
   mod-effects
   mod-effect-classes
   mod-midi-import-export
   mod-pcm
   mod-mp3
   mod-ffmpeg
   mod-mp2
   mod-ogg
   mod-flac
   mod-plugin-menus
   mod-macros
   mod-printing
   mod-screenshot
   mod-command-classes
   mod-vst
   mod-vamp
   mod-nyquist
   mod-lv2
   mod-ladspa
   mod-audiounits
   mod-navigation-menus
   mod-help-menus
   mod-view-menus
   mod-select-menus
   mod-track-menus
   mod-clip-menu-items
   mod-timer-record
   mod-frequency-plot
   mod-contrast
   mod-undo-history
   mod-lyrics
   mod-mixer-board
   mod-soundtouch
   mod-script-pipe
)
if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows" )
   list( APPEND MODULES
      mod-null
      mod-nyq-bench
   )
endif()

foreach( MODULE ${MODULES} )
   add_subdirectory("${MODULE}")
endforeach()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   if( NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio*")
      install( DIRECTORY "${_DEST}/modules"
               DESTINATION "${_PKGLIB}" )
   endif()
endif()

# Propagate some collected values
foreach(var MIMETYPES MAC_RESOURCES)
   set( ${var} "${${var}}" PARENT_SCOPE )
endforeach()

#propagate collected edges up to root CMakeLists.txt
set( GRAPH_EDGES "${GRAPH_EDGES}" PARENT_SCOPE )
