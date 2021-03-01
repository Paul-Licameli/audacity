list( APPEND SOURCES
   SampleTrack.cpp
   SampleTrack.h
   SampleTrackCache.cpp
   SampleTrackCache.h
   Mix.cpp
   Mix.h
)
audacity_library( lib-sample-track "${SOURCES}"
   "lib-math-interface;lib-track-interface;PRIVATE;wxBase"
   "" ""
)
