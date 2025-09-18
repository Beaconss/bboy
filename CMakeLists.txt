cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)

project(std-boy)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        a96677bdf6b4acb84af4ec294e5f60a4e8cbbe03
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(SDL3)

include_directories("src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(std-boy ${SOURCES})

target_link_libraries(std-boy PRIVATE SDL3::SDL3)
