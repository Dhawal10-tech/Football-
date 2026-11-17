cmake_minimum_required(VERSION 3.20)
project(PblOopsNeymar VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML statically" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Audio is not used by this application" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "Networking is not used by this application" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

# Reuse the SFML 2.6.2 source already fetched by the parent FTMS project.
# If this folder is opened on its own, FetchContent downloads the same pinned version.
set(FTMS_SFML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/sfml-src"
    CACHE PATH "Optional local SFML 2.6.2 source folder")
if(EXISTS "${FTMS_SFML_SOURCE_DIR}/CMakeLists.txt")
  add_subdirectory("${FTMS_SFML_SOURCE_DIR}" sfml-build)
else()
  include(FetchContent)
  FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SUBMODULES ""
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SFML)
endif()

add_executable(neymar neymar.cpp)
target_link_libraries(neymar PRIVATE sfml-graphics)
if(MSVC)
  target_compile_options(neymar PRIVATE /W4 /permissive-)
else()
  target_compile_options(neymar PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET neymar POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:neymar>/assets/fonts"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/font.ttf"
    "$<TARGET_FILE_DIR:neymar>/assets/fonts/font.ttf"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/font-bold.ttf"
    "$<TARGET_FILE_DIR:neymar>/assets/fonts/font-bold.ttf")
foreach(runtime_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
  if(EXISTS "C:/msys64/ucrt64/bin/${runtime_dll}")
    add_custom_command(TARGET neymar POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/msys64/ucrt64/bin/${runtime_dll}"
        "$<TARGET_FILE_DIR:neymar>/${runtime_dll}")
  endif()
endforeach()
