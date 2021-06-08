function( ensure_wxBase target )
   if( NOT TARGET wxBase )
      add_library( wxBase ALIAS "${target}")
   endif()
endfunction()

if( ${_OPT}use_wxwidgets STREQUAL "system" ) 
    if( NOT TARGET wxwidgets::wxwidgets )
        add_library( wxwidgets::wxwidgets INTERFACE IMPORTED GLOBAL)
    endif()

    foreach( lib base core html xml xrc qa aui adv )
       set( target "wxwidgets::${lib}" )
       if( NOT TARGET ${target} )
          add_library( ${target} ALIAS wxwidgets::wxwidgets )
       endif()
    endforeach()

    ensure_wxBase( wxwidgets::wxwidgets )

    if( NOT TARGET wxwidgets::wxwidgets )
        add_library( wxwidgets::wxwidgets ALIAS wxwidgets::wxwidgets )
    endif()

    if( wxWidgets_INCLUDE_DIRS_NO_SYSTEM )
        target_include_directories( wxwidgets::wxwidgets INTERFACE ${wxWidgets_INCLUDE_DIRS_NO_SYSTEM} )
    else()
        target_include_directories( wxwidgets::wxwidgets INTERFACE ${wxWidgets_INCLUDE_DIRS} )
    endif() 

    target_compile_definitions( wxwidgets::wxwidgets INTERFACE
        ${wxWidgets_DEFINITIONS_GENERAL}
        $<$<CONFIG:Debug>:
            ${wxWidgets_DEFINITIONS_DEBUG}
        >
        $<$<NOT:$<CONFIG:Debug>>:
            ${wxWidgets_DEFINITIONS_OPTIMIZED}
        >
    )

    target_link_directories( wxwidgets::wxwidgets INTERFACE
        $<$<PLATFORM_ID:Windows>:
           ${wxWidgets_LIB_DIR}
        >
    )

    target_link_libraries( wxwidgets::wxwidgets INTERFACE
        ${wxWidgets_LIBRARIES}
        $<$<NOT:$<PLATFORM_ID:Windows>>:
           z
        >
    )

    set( toolkit "${wxWidgets_LIBRARIES}" )

    if( "${toolkit}" MATCHES ".*gtk2.*" )
        set( gtk gtk+-2.0 )
        set( glib glib-2.0 )
    elseif( "${toolkit}" MATCHES ".*gtk3.*" )
        set( gtk gtk+-3.0 )
        set( glib glib-2.0 )
    elseif( "${toolkit}" MATCHES ".*gtk4.*" )
        set( gtk gtk+-4.0 )
        set( glib glib-2.0 )
    endif()
else()
    set_target_properties(wxwidgets::base PROPERTIES IMPORTED_GLOBAL On)
    ensure_wxBase( wxwidgets::base )
endif()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )

    if( NOT DEFINED gtk )
        set( gtk gtk+-2.0 )
        set( glib glib-2.0 )
    endif()

    pkg_check_modules( GTK REQUIRED IMPORTED_TARGET GLOBAL ${gtk} )
    pkg_check_modules( GLIB REQUIRED IMPORTED_TARGET GLOBAL ${glib} )
endif()

