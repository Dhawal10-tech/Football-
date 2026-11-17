cmake_minimum_required(VERSION 3.21)
project(FootballTournamentManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
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

add_executable(ftms src/main.cpp)
# Keep updated builds separate from an older FTMS process that Windows may
# still have open, so the refreshed executable can be linked without a lock.
set_target_properties(ftms PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/latest")
target_link_libraries(ftms PRIVATE sfml-graphics sfml-window sfml-system)
if(WIN32)
  target_link_libraries(ftms PRIVATE bcrypt)
endif()
target_compile_definitions(ftms PRIVATE
  FTMS_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
  FTMS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Keep the shared SFML and MinGW runtime DLLs beside the Windows executable.
if(WIN32)
  add_custom_command(TARGET ftms POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:ftms>
      $<TARGET_FILE_DIR:ftms>
    COMMAND_EXPAND_LISTS)
endif()
if(MINGW)
  get_filename_component(FTMS_MINGW_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
  foreach(runtime_dll IN ITEMS libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
    add_custom_command(TARGET ftms POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FTMS_MINGW_BIN}/${runtime_dll}"
        "$<TARGET_FILE_DIR:ftms>")
  endforeach()
endif()

if(MSVC)
  target_compile_options(ftms PRIVATE /W4 /permissive-)
else()
  target_compile_options(ftms PRIVATE -Wall -Wextra -Wpedantic)
endif()
