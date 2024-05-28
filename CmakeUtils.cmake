function(MY_PRINT_ALL_VARIABLES)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

function(MIGRATE_GLADE_GTK3_TO_GTK4_WITH_SUFFIX GLADE_ASSETS_PATH)
    file(GLOB files "${GLADE_ASSETS_PATH}/*.glade")
    foreach(file ${files})
        list(APPEND gresources_glade_gtk3_deps ${file})
    endforeach()

    foreach(file ${gresources_glade_gtk3_deps})
        if (file MATCHES ".*_gtk4.glade")
            continue()
        endif()
        get_filename_component(filename ${file} NAME)
        get_filename_component(filename_base ${file} NAME_WE)
        get_filename_component(filename_ext ${file} EXT)
        set(filename_gtk4 "${filename_base}_gtk4${filename_ext}")
    add_custom_target(
        dummy_target_${filename_gtk4} ALL
        DEPENDS ${filename_gtk4}
    )
    add_custom_command(
        OUTPUT ${filename_gtk4}
        COMMENT "gtk4-builder-tool 3to4 ${filename}"
        WORKING_DIRECTORY ${GTK_UI_PATH}
        COMMAND gtk4-builder-tool
        ARGS
            simplify
            --3to4
            #--replace
            ${filename}
            > ${filename_gtk4}
        DEPENDS ${file}
    )
    endforeach()
endfunction()
