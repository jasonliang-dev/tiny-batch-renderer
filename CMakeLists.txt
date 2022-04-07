cmake_minimum_required(VERSION 3.11)
project(app)

include(FetchContent)
set(FETCHCONTENT_QUIET NO)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.3.6.tar.gz
)
FetchContent_MakeAvailable(glfw)

add_executable(${PROJECT_NAME} src/main.c src/renderer.c src/texture.c)
target_link_libraries(${PROJECT_NAME} glfw OpenGL::GL)
