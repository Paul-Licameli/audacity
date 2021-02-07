list( APPEND SOURCES
   TagsEditor.cpp
   TagsEditor.h
)
audacity_module( mod-tags-ui "${SOURCES}" "Audacity"
   "" "" )
