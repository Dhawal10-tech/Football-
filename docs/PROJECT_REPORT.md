# Project Report — Football Tournament Management System

## Abstract

FTMS is an offline C++20 desktop application for managing football tournaments. It supports tournament, team, and player records; round-robin fixtures; match scores and scorers; standings and player statistics; history; and CSV reports.

## Problem and objectives

Paper-based registration, fixture creation, and score tracking can produce duplicate records and inconsistent tables. FTMS keeps tournament records in a local versioned file and calculates standings and player statistics from match results.

## Scope

The application supports referee sign-in, tournament creation and state transitions, team and player registration, round-robin fixture generation, score entry and correction, standings, player goal statistics, history, search, and CSV export. It validates player/team references and prevents accidental overwrite after a data-load error.

## Architecture and OOP

The current application is consolidated in `src/main.cpp`. `Player`, `Team`, `Match`, and `Tournament` represent domain data. `User` is an abstract base class and `Referee` inherits from it. `AuthenticationService` and `StorageInterface` define abstractions with concrete implementations. Services handle validation, scheduling, standings, statistics, and tournament transitions. `Application` coordinates the services and the SFML interface. The code demonstrates encapsulation, inheritance, abstraction, and polymorphism.

## Scheduling and calculations

The circle method rotates a team ring while fixing one position. An empty slot represents a bye for an odd team count. For `n` teams, the schedule has `n(n-1)/2` matches over `n-1` rounds when even or `n` rounds when odd. Points are `3W + D`; goal difference is `GF - GA`. Display order is points, goal difference, goals scored, then stable ID. Completion is blocked if fixtures remain or the top sporting criteria are tied.

## Persistence

The first line `FTMS 1` identifies the tournament schema. Tagged line records use quoted string escaping. Loading validates IDs and relationships; saving stages a temporary file and retains a backup during replacement. Tournament data and referee credentials are local files under `data/` and are excluded from Git.

## Verification

The current CMake target builds with the configured MSYS2 UCRT64 GCC, CMake, and Ninja toolchain. A complete manual workflow should be reviewed on Windows, including first-run referee account creation, score entry, saved-data reload, and CSV export. No automated test suite is included in the consolidated root project.

## Limitations

The app is local/offline and supports referee sign-in only; it has no online synchronization. Finalization rejects unresolved standings ties rather than providing a playoff workflow. Match date/time entry and GUI automation are not included.

## Conclusion

FTMS provides local tournament-management workflows in a C++20/SFML desktop application. The project uses CMake and includes VS Code tasks for configuring, building, and launching the app.
