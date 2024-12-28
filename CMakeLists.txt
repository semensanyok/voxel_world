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
set(CMAKE_CXX_STANDARD 20)

# mysterious advice given by MSVC compiler. somehow windows specific.
if(${BUILD_WINDOWS_BASE})
  add_compile_options(/bigobj)
endif()

include(CmakeUtils.cmake)

set(ENGINE_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(ENGINE_DEP_DIR ${CMAKE_CURRENT_LIST_DIR}/dependencies)
include(CTest)
enable_testing()

# repo only installs deprecated FindOpenVDB.cmake and latest cmake automatically seeks one of [OpenVDBConfig.cmake, openvdb-config.cmake]
list(APPEND CMAKE_MODULE_PATH
 "${OpenVDB_DIR}")

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(OpenVDB REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
set(FastNoise2_DIR dependencies/FastNoise2)
add_subdirectory(${FastNoise2_DIR})

# copy to bin dir to save dev time. TODO: remove. static deps. (branch 'emscripten' has some work)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("-- Debug build")
  file(COPY ${_TBB_ROOT}/bin/tbb12_debug.dll DESTINATION ${CMAKE_BINARY_DIR})
  file(COPY ${_OPENVDB_ROOT}/bin/openvdb.dll DESTINATION ${CMAKE_BINARY_DIR})
  file(COPY ${Blosc_INCLUDE_DIR}/../bin/blosc.dll DESTINATION ${CMAKE_BINARY_DIR})
  file(COPY ${ZLIB_INCLUDE_DIR}/../bin/zlibd.dll DESTINATION ${CMAKE_BINARY_DIR})
  file(GLOB SDL2_IMAGE_DLLS
      "${SDL2_image_DIR}/../bin/*.dll"
    )
  message("copying ${SDL2_IMAGE_DLLS} to target dir")
  file(COPY ${SDL2_IMAGE_DLLS} DESTINATION ${CMAKE_BINARY_DIR})
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
    "${ENGINE_DEP_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_tables.cpp"
)

add_executable(voxel_world
 ${IMGUI_SOURCES}
 ${ENGINE_SRC_DIR}/main.cpp
 ${ENGINE_SRC_DIR}/logging.cpp
 ${ENGINE_SRC_DIR}/renderer_context.cpp
 ${ENGINE_SRC_DIR}/vw_utils.cpp
 ${ENGINE_SRC_DIR}/grids.cpp
)
target_link_libraries(voxel_world OpenVDB::openvdb ${SDL2_LIBRARIES} SDL2_image::SDL2_image glm::glm FastNoise Vulkan::Vulkan)
target_include_directories(voxel_world PUBLIC
      "${ENGINE_SRC_DIR}"
        "${SDL2_INCLUDE_DIR}"
      "${OpenVDB_openvdb_INCLUDE_DIR}"
        "${ENGINE_DEP_DIR}/imgui/"
        "${ENGINE_DEP_DIR}/imgui/backends/"
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

set(VULKAN_SPV_DIR ${CMAKE_BINARY_DIR}/shaders/spv/)
target_compile_definitions(voxel_world PUBLIC VULKAN_SPV_DIR=${VULKAN_SPV_DIR})

file(GLOB shaderfiles src/shaders/*)

foreach(filename ${shaderfiles})
  get_filename_component(inext ${filename} EXT)
  string(REPLACE "." "" inexttrim ${inext})
  get_filename_component(inbase ${filename} NAME_WE)
  set(outfilename ${inbase}_${inexttrim}.spv)
  set(outfilename_full ${VULKAN_SPV_DIR}/${inbase}_${inexttrim}.spv)
  message("outfilename=${outfilename}")

  add_custom_target(
dummy_target_${outfilename} ALL
DEPENDS ${outfilename_full}
)
  add_custom_command(
OUTPUT ${outfilename_full}
COMMENT "compile shader ${filename}"
WORKING_DIRECTORY ${GTK_UI_PATH}
COMMAND ${VULKAN_BIN_DIR}/glslc.exe
ARGS
${filename}
-o
${outfilename_full}
DEPENDS ${filename}
)
endforeach()

# MY_PRINT_ALL_VARIABLES()

