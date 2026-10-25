# Football Tournament Management System

An offline C++20 desktop application for managing football tournament records. Tournament rules, local persistence, and the SFML interface live in separate modules. It supports tournament, team, and player records; round-robin fixtures; score entry; calculated standings and scorer totals; history; and CSV reports.

## Problem and objectives

Paper-based registration and score tracking can cause duplicate records, fixture errors, and inconsistent standings. FTMS aims to keep tournament data in one local file, generate round-robin schedules, calculate standings from results, and calculate goals from explicitly entered scorer records.

## Features in this version

- C++20 domain records and validation service.
- Single round-robin fixture generation for even and odd team counts. Odd schedules include a bye; a team plays at most once in each round. This does not claim balanced home/away assignments.
- Result entry with match/team/scorer validation; recording again replaces the existing result and scorer list.
- Standings recalculated from completed matches: 3/1/0 scoring, ordered by points, goal difference, goals scored, then stable team ID. The final stable ordering does not break a sporting tie: finalization rejects a tie on the first three fields.
- Versioned local text persistence (`data/tournaments.ftms`) with quoted/escaped fields and a temporary-file replacement strategy.
- SFML navigation, forms, confirmation prompts, search, selection, and scrolling.
- Tournament, team, and player create/edit/delete flows with state-aware validation.
- Score correction, statistics, finalization/reopening, and CSV report export.

The application implements the main tournament workflows but has not been visually verified through Windows UI automation in this environment. CTest's launcher previously reported Windows loader error `0xc000007b`; the test executable now links statically on MinGW and is being rechecked.

## Technology and architecture

- C++20, CMake, SFML 2.6.2, local file storage.
- `domain.hpp`: tournament, team, player, match and standings value records.
- `services.cpp`: validation, round-robin scheduling, standings, scorer totals, and state transitions.
- `storage.cpp`: `StorageInterface` and `FileRepository`.
- `ui/application.cpp`: SFML window shell, separate from business calculations.

## Prerequisites

Windows with MSYS2 UCRT64, GCC with C++20 support, CMake 3.21+, Git, and Ninja (recommended). The current environment has GCC 16.2.0, CMake 4.4.3, and Ninja 1.13.2. CMake fetches and builds the pinned SFML 2.6.2 source, so first configuration needs network access. DejaVu Sans is bundled under `assets/fonts` with its license under `assets/licenses`. For running outside this build environment, place SFML 2.6.2 and MinGW runtime DLLs beside the executable or on `PATH`.

## Build and run

From an MSYS2 UCRT64 terminal in the project folder:

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
./build/ftms.exe
```

For a core-only build without fetching SFML, configure with `-DFTMS_BUILD_GUI=OFF`; this project currently requires CMake to run either build. Application data is saved relative to the process working directory in `data/tournaments.ftms`.

## Data format

The first line is `FTMS 1`. Each following record is line-oriented, tagged, and uses C++ quoted-string escaping. Tournament records contain child team, player, match, and scorer records. Unknown tags, invalid enum values, and broken IDs/relationships cause a load error; the repository does not silently replace unreadable data with an empty set. Back up the data file before manual edits.

## Tests

Core tests cover one through eight teams, unique pairings and per-round participation, duplicate registration, result correction, pending/tied finalization, scorer totals, malformed schema rejection, and persistence round trips. Run `ctest --test-dir build --output-on-failure`. A manual Windows GUI walkthrough remains recommended before submission.

## Known limitations

- Search is opened with `/`; press Enter to leave search mode. The application does not provide authentication or role-based access control.
- Finalization rejects an unresolved standings tie; no extra playoff/tie-resolution workflow is provided.
- The app has not been visually inspected through Windows UI automation here. Report export and full restart recovery should be manually checked before college submission.

