cmake_minimum_required(VERSION 3.21)
project(FootballTournamentManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(FTMS_BUILD_GUI "Build the SFML desktop application" ON)
option(FTMS_BUILD_TESTS "Build core logic tests" ON)

include(FetchContent)
if(FTMS_BUILD_GUI)
  set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SUBMODULES ""
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SFML)
endif()

add_library(ftms_core STATIC
  src/models.cpp
  src/services.cpp
  src/storage.cpp)
target_include_directories(ftms_core PUBLIC include)
if(MSVC)
  target_compile_options(ftms_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(ftms_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(FTMS_BUILD_GUI)
  add_executable(ftms src/main.cpp src/ui/application.cpp)
  target_link_libraries(ftms PRIVATE ftms_core sfml-graphics sfml-window sfml-system)
  target_compile_definitions(ftms PRIVATE FTMS_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")

endif()

if(FTMS_BUILD_TESTS)
  enable_testing()
  add_executable(ftms_tests tests/core_tests.cpp)
  target_link_libraries(ftms_tests PRIVATE ftms_core)
  if(MINGW)
    target_link_options(ftms_tests PRIVATE -static)
  endif()
  add_test(NAME ftms_core_tests COMMAND ftms_tests)
endif()
