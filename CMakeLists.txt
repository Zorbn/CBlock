cmake_minimum_required(VERSION 3.0.0)
project(CBlock VERSION 0.1.0 LANGUAGES C)
set(C_STANDARD 11)

include(CTest)
enable_testing()

add_executable(
    CBlock
    
    src/main.c
    src/list.h
    src/chunk.c src/chunk.h
    src/world.c src/world.h
    src/camera.c src/camera.h
    src/file.c src/file.h
    src/window.c src/window.h
    src/graphics/mesher.c src/graphics/mesher.h
    src/graphics/mesh.c src/graphics/mesh.h
    src/graphics/resources.c src/graphics/resources.h
    deps/glad/src/glad.c
)
target_include_directories(CBlock PRIVATE deps/glad/include deps/stb_image/include)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/glfw)

add_subdirectory(deps/cglm)

target_link_libraries(CBlock PRIVATE glfw cglm)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
