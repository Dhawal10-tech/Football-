# Project Report — Football Tournament Management System

## Abstract

FTMS is an offline C++20 desktop application for managing football tournaments. It provides tournament, team, and player records; generates single round-robin fixtures; records and corrects match scores and scorers; calculates standings and player statistics; preserves tournament history; and exports CSV reports.

## Problem and objectives

Paper-based registration, fixture creation, and score tracking can produce duplicate records and inconsistent tables. FTMS centralizes tournament data in a local versioned file and derives standings and statistics from match results.

## Scope and requirements

The application supports creating, editing, switching, starting, completing, reopening, and deleting tournaments subject to state validation; team and player CRUD; round-robin fixture generation; score entry and correction; standings; player goal statistics; history; search; and CSV exports. It rejects invalid references and prevents accidental overwrite after a data load error.

## Architecture

`Player`, `Team`, `Match`, and `Tournament` are domain records. `TournamentFormat` is an abstraction implemented by `RoundRobinFormat`. `ValidationService` centralizes checks; `StandingsCalculator` derives the table; `StatisticsManager` derives player goals; and `TournamentManager` controls tournament transitions and results. `StorageInterface` abstracts persistence and `FileRepository` implements local storage. `Application` provides the SFML interface and coordinates services.

## Scheduling and calculations

The circle method rotates a team ring while fixing one position. An empty slot represents a bye for odd team counts. For `n` teams, the schedule has `n(n-1)/2` matches over `n-1` rounds when even or `n` rounds when odd. Points are `3W + D`; goal difference is `GF - GA`. Display order is points, goal difference, goals scored, and stable ID. Completion is blocked if fixtures remain or the top sporting criteria are tied.

## Persistence

The first line `FTMS 1` identifies the schema. Tagged line records use quoted string escaping. Loading validates IDs and relationships. Saving stages a temporary file and keeps a backup during replacement. Data is stored in `data/tournaments.ftms`, relative to the process working directory.

## Verification

The C++ core tests cover schedules for one through eight teams, pair uniqueness and round participation, validation, score corrections, finalization guards, scorer totals, malformed data, and persistence round trips. Build and test commands are documented in README. The current CTest run passes. The SFML GUI compiles; interactive visual review still requires a manual Windows walkthrough.

## Limitations

FTMS has no user authentication or roles, online synchronization, playoff/tiebreak workflow, or GUI automation tests. Match date/time entry is not part of the current interface. A manual walkthrough of export and restart recovery is recommended before deployment.

## Conclusion

FTMS provides the main local tournament management workflows in a modular desktop application, with a passing core test suite. The executable is ready to run in the configured MSYS2 build environment.
