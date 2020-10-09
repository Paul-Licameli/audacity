list( APPEND SOURCES
   SpectralSelectionBar.cpp
   SpectralSelectionBar.h
)
audacity_module( mod-spectral-selection-tool-bar "${SOURCES}" "Audacity"
   "" "" )
