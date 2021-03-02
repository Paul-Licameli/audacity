list( APPEND SOURCES
   MixAndRender.cpp
   MixAndRender.h
   SampleBlock.cpp
   SampleBlock.h
   Sequence.cpp
   Sequence.h
   WaveClip.cpp
   WaveClip.h
   WaveTrack.cpp
   WaveTrack.h
)
audacity_library( lib-wave-track "${SOURCES}"
   "lib-sample-track-interface;lib-project-rate-interface;PRIVATE;wxBase"
   "" ""
)
