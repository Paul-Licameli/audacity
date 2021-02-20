list( APPEND SOURCES
   Envelope.cpp
   Envelope.h
   TimeWarper.cpp
   TimeWarper.h
   Track.cpp
   Track.h
   TrackAttachment.cpp
   TrackAttachment.h
)
audacity_library( lib-track "${SOURCES}"
   "lib-project-interface;lib-xml-interface;PRIVATE;wxBase"
   "" ""
)
