cmake_minimum_required(VERSION 3.0.0)
project(CBlock VERSION 0.1.0 LANGUAGES C)
set(C_STANDARD 11)

include(CTest)
enable_testing()

set (
    CBLOCK_SOURCE_FILES

    src/main.c
    src/list.h
    src/file.c src/file.h
    src/input.c src/input.h
    src/chunk.c src/chunk.h
    src/world.c src/world.h
    src/camera.c src/camera.h
    src/window.c src/window.h
    src/directions.c src/directions.h
    src/graphics/mesh.c src/graphics/mesh.h
    src/graphics/mesher.c src/graphics/mesher.h
    src/graphics/resources.c src/graphics/resources.h
    src/graphics/sprite_batch.c src/graphics/sprite_batch.h
    src/graphics/meshing_info.c src/graphics/meshing_info.h
)

add_executable(
    CBlock

    ${CBLOCK_SOURCE_FILES}
    deps/glad/src/glad.c
)
target_include_directories(CBlock PRIVATE deps/glad/include deps/stb_image/include)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

add_subdirectory(deps/cglm)

target_link_libraries(CBlock PRIVATE glfw cglm)

if(NOT MSVC)
    set_source_files_properties(${CBLOCK_SOURCE_FILES} PROPERTIES COMPILE_FLAGS -Wall -Werror -Wpedantic)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
