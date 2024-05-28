cmake_minimum_required(VERSION 3.0.0)
project(voxel_world VERSION 0.1.0 LANGUAGES C CXX)

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
find_package(OpenVDB REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)


add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
set(IMGUI_SOURCES
    "${ENGINE_DEP_DIR}/imgui/imgui.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_demo.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_draw.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_widgets.cpp"
    "${ENGINE_DEP_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
    "${ENGINE_DEP_DIR}/imgui/backends/imgui_impl_sdl2.cpp"
    "${ENGINE_DEP_DIR}/imgui/imgui_tables.cpp"
)

add_subdirectory("${GLAD_DIR}" "${CMAKE_BINARY_DIR}/glad_build")
glad_add_library(glad_gl_core_33 SHARED API gl:core=3.3)

add_executable(voxel_world 
	${IMGUI_SOURCES} 
	${ENGINE_SRC_DIR}/main.cpp 
	${ENGINE_SRC_DIR}/logging.cpp
)
target_link_libraries(voxel_world OpenVDB::openvdb ${SDL2_LIBRARIES} SDL2_image::SDL2_image glm::glm glad_gl_core_33)
target_include_directories(voxel_world PUBLIC 
		    ${ENGINE_SRC_DIR} 
                    ${SDL2_INCLUDE_DIR} 
                    "${ENGINE_DEP_DIR}/imgui/" 
                    "${ENGINE_DEP_DIR}/imgui/backends/"
)
# MY_PRINT_ALL_VARIABLES()
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
