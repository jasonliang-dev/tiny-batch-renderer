cmake_minimum_required(VERSION 3.12)
project(app)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} src/main.c)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_directories(${PROJECT_NAME} PUBLIC libs)
target_link_libraries(${PROJECT_NAME} glfw3 OpenGL::GL)
