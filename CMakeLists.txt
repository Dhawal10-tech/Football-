cmake_minimum_required(VERSION 3.24)
project(FootballTournamentManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# SFML 2.6.2 static setup
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SFML-2.6.2/CMakeLists.txt")
    add_subdirectory(thirdparty/SFML-2.6.2 sfml_build)
else()
    message(FATAL_ERROR "SFML 2.6.2 source not found in thirdparty/SFML-2.6.2")
endif()

# Core library sources (models, services, managers, storage)
set(CORE_SOURCES
    src/security/Authentication.cpp
    src/models/Player.cpp
    src/models/Team.cpp
    src/models/Match.cpp
    src/models/Tournament.cpp
    src/services/FixtureGenerator.cpp
    src/services/StandingsCalculator.cpp
    src/services/StatisticsManager.cpp
    src/services/ValidationService.cpp
    src/storage/FileRepository.cpp
    src/managers/TournamentManager.cpp
)

# UI sources
set(UI_SOURCES
    src/ui/LoginWindow.cpp
    src/ui/UITheme.cpp
    src/ui/UIComponents.cpp
    src/ui/MainWindow.cpp
)

# Core static library used by both application and automated tests
add_library(ftms_core STATIC ${CORE_SOURCES})
target_include_directories(ftms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(WIN32)
    target_link_libraries(ftms_core PUBLIC bcrypt)
endif()

# UI library
add_library(ftms_ui STATIC ${UI_SOURCES})
target_include_directories(ftms_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ftms_ui PUBLIC ftms_core sfml-graphics)

# Main desktop application executable
add_executable(FootballTournamentManager src/main.cpp)
target_include_directories(FootballTournamentManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(FootballTournamentManager PRIVATE ftms_ui)

# Automated test executable
add_executable(run_tests tests/test_main.cpp)
target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(run_tests PRIVATE ftms_core)

# Post-build asset copying to build folder
add_custom_command(TARGET FootballTournamentManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:FootballTournamentManager>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:FootballTournamentManager>/data"
)
