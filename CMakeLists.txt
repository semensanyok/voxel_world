cmake_minimum_required(VERSION 3.0.0)
project(voxel_world VERSION 0.1.0 LANGUAGES C CXX)

# nvim clangd LSP server issue
#   workaround to trick buggy clangd to think we use c++20 but compile with c++23
#   > cl : Command line warning D9025 : overriding '/std:c++20' with '/std:c++latest'
#   UPDATE: fixed with 
#   '''
#      cat <<EOF > .clangd
#      CompileFlags:
#        Add: [ "-std:c++latest" ]
#      EOF
#   '''
#   OR
#   '''
#       set(CMAKE_CXX_STANDARD 20)
#       add_compile_options(/std:c++latest)
#       add_compile_options(-std:c++23)
#   '''
set(CMAKE_CXX_STANDARD 23)
include(CmakeUtils.cmake)

set(ENGINE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(ENGINE_DEP_DIR ${CMAKE_CURRENT_LIST_DIR}/dependencies)
include(CTest)
enable_testing()

# repo only installs deprecated FindOpenVDB.cmake and latest cmake automatically seeks one of [OpenVDBConfig.cmake, openvdb-config.cmake]
list(APPEND CMAKE_MODULE_PATH 
	"${OpenVDB_DIR}")
find_package(glm REQUIRED)
find_package(OpenVDB REQUIRED )

# find_package(SDL2 REQUIRED)
# find_package(SDL2_image REQUIRED)
# find_package(TBB REQUIRED)
find_package(TBB
REQUIRED             # Fail with error if Tbb is not found
COMPONENTS tbb # Tbb libraries by their canonical name
# e.g. "tbb" for "libtbb"
)
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# copy to bin dir to save dev time. TODO: remove. package dlls.
set(MY_CMAKE_EXECUTABLE_SUBDIR "")
if("${CMAKE_CXX_COMPILER}" STREQUAL "cl")
    message("-- MSBuild build")
    set(MY_CMAKE_EXECUTABLE_SUBDIR "/Debug")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("-- Debug build")
    # file(COPY ${_TBB_ROOT}/bin/tbb12_debug.dll DESTINATION ${CMAKE_BINARY_DIR}${MY_CMAKE_EXECUTABLE_SUBDIR})
    # file(COPY ${_OPENVDB_ROOT}/bin/openvdb.dll DESTINATION ${CMAKE_BINARY_DIR}${MY_CMAKE_EXECUTABLE_SUBDIR})
    # file(COPY ${Blosc_INCLUDE_DIR}/../bin/blosc.dll DESTINATION ${CMAKE_BINARY_DIR}${MY_CMAKE_EXECUTABLE_SUBDIR})
    # file(COPY ${ZLIB_INCLUDE_DIR}/../bin/zlibd.dll DESTINATION ${CMAKE_BINARY_DIR}${MY_CMAKE_EXECUTABLE_SUBDIR})
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("-- Release build")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message("-- Minimum size release build")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("-- Release build with debug information")
else()
    message("-- Unknown build type") # <---- This is the result, why?
endif()

set(IMGUI_SOURCES
    "${ENGINE_DEP_DIR}/imgui/imgui.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_demo.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_draw.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_widgets.cpp"
    "${ENGINE_DEP_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
    "${ENGINE_DEP_DIR}/imgui/backends/imgui_impl_sdl2.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_tables.cpp"
)

# set(SDL_OPENGL_SPEC gl)
# set(SDL_OPENGL_VERSION_MAJOR 4)
# set(SDL_OPENGL_VERSION_MINOR 6)
# set(SDL_OPENGL_PROFILE core)
set(SDL_OPENGL_SPEC gles2)
set(SDL_OPENGL_VERSION_MAJOR 3)
set(SDL_OPENGL_VERSION_MINOR 2)
set(SDL_OPENGL_PROFILE core)

######## GLAD INCLUDE ##########################################
# add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
# set(SDL_OPENGL_LIB_NAME glad_${SDL_OPENGL_SPEC}_${SDL_OPENGL_PROFILE}_${SDL_OPENGL_VERSION_MAJOR}${SDL_OPENGL_VERSION_MINOR})
# add_subdirectory("${GLAD_DIR}/cmake" "${CMAKE_BINARY_DIR}/glad_build")
# glad_add_library(${SDL_OPENGL_LIB_NAME} STATIC API ${SDL_OPENGL_SPEC}:${SDL_OPENGL_PROFILE}=${SDL_OPENGL_VERSION_MAJOR}.${SDL_OPENGL_VERSION_MINOR})
################################################################

add_executable(voxel_world 
	${IMGUI_SOURCES} 
	${ENGINE_SRC_DIR}/main.cpp 
	${ENGINE_SRC_DIR}/logging.cpp 
	${ENGINE_SRC_DIR}/renderer_context.cpp
)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main SDL2-static)
find_package(SDL2_image REQUIRED)
find_package(SDL2_image  REQUIRED CONFIG COMPONENTS SDL2_image-static)
target_link_libraries(voxel_world 
# ${SDL2_LIBRARIES} 
# ${SDL2_LIB_COMPONENTS} 
SDL2::SDL2-static 
SDL2::SDL2main 
SDL2_image::SDL2_image-static 
glm::glm 
# ${SDL_OPENGL_LIB_NAME} # if use glad
# static linking links libopenvdb.lib which misses some (_dllexport or import) definitions or smth like this
# and with emscripten build we use dynamic openvdb. (maybe will write why not static when remember)
OpenVDB::openvdb 
# "D:/__OpenSource/openvdb_install_MDd_DynamicDebug/lib/openvdb.lib"
TBB::tbb 
)
target_include_directories(voxel_world PRIVATE 
		    "${ENGINE_SRC_DIR}" 
		    "${SDL2_IMAGE_INCLUDE_DIR}" 
		    "${SDL2_INCLUDE_DIR}/SDL2" 
		    "${OpenVDB_openvdb_INCLUDE_DIR}" 
                    "${ENGINE_DEP_DIR}/imgui/" 
                    "${ENGINE_DEP_DIR}/imgui/backends/" 
		    "${GLAD_DIR}/glad/include/"
		    "${Boost_INCLUDE_DIR}"
)

IF ( CMAKE_EXPORT_COMPILE_COMMANDS )
	execute_process(
	   COMMAND ${CMAKE_COMMAND} -E create_symlink
	       ${CMAKE_BINARY_DIR}/compile_commands.json
	       ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
	)
endif()
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# MY_PRINT_ALL_VARIABLES()

# TODO: empscripten build to js for web thor jam
