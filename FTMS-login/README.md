# Football Tournament Management System (FTMS)

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![SFML](https://img.shields.io/badge/GUI-SFML_2.6.2-brightgreen.svg)](https://www.sfml-dev.org/)
[![Build System](https://img.shields.io/badge/Build-CMake_%2B_Ninja-orange.svg)](https://cmake.org/)
[![Tests](https://img.shields.io/badge/Tests-47%20Passed-success.svg)](tests/)

An offline, desktop **Object-Oriented Programming (OOP) Project-Based Learning (PBL)** application developed for college tournament organizers to digitize, schedule, and automate football tournaments.

---

## 1. Problem Statement & Background
Our college organizes football tournaments multiple times per academic year. Organizers traditionally managed tournament schedules, team squads, fixture pairings, match scores, and point tables manually using pen and paper. This caused:
- Duplicate team and jersey registrations.
- Errors in manual fixture pairings and scheduling.
- Miscalculations in goal differences, head-to-head records, and points.
- Lack of historical record preservation for past collegiate championships.
- Time-consuming preparation of final reports and awards.

**FTMS** solves these issues through a fully automated, desktop C++20 desktop application built on SFML 2.6.2.

---

## 2. Key Objectives & Features

- **Football-Themed Modern GUI**: Dark-pitch green aesthetic, sidebar navigation, responsive cards, interactive tables, and confirmation modal dialogs.
- **Round-Robin Fixture Engine**: Circle-method algorithm scheduling every team to meet once ($M = n(n-1)/2$), automatically handling odd teams via BYE slots with balanced home/away distribution.
- **Dynamic Standings Calculation**: Derives points ($W=3, D=1, L=0$), goals for (GF), goals against (GA), goal difference (GD), and points table strictly from verified match results.
- **Documented Tie-Breaking Policy**:
  1. Points (descending)
  2. Goal Difference (descending)
  3. Goals Scored (descending)
  4. Head-to-Head points between tied teams
  5. Deterministic tie flag / explicit resolution preventing arbitrary champions.
- **Squad & Player Golden Boot Tracking**: Player registration with position categorization (GK, DEF, MID, FWD), unique jersey numbers per team, and live top scorer leaderboards.
- **Structured File Persistence**: Versioned, human-readable text storage (`FTMS_DATA_V1`) supporting atomic file writes (`.tmp` to `.dat`) with safe crash recovery.
- **Report & Data Export**: One-click export of official Standings (CSV) and complete tournament text summaries.
- **Referee Login & Score Audit**: Referees create unique IDs, sign in before the workspace opens, and their ID is recorded with each score update in `data/referee_audit.log`.
- **OOP Four Pillars**: The authentication and role system demonstrates abstraction, encapsulation, inheritance, and polymorphism; see [docs/REFEREE_LOGIN_AND_OOP.md](docs/REFEREE_LOGIN_AND_OOP.md).

### Referee sign-in

On first launch, select **Create Referee ID**, enter a unique ID and a password of at least eight characters, confirm it, and then sign in. Accounts are saved in `data/referees.db`; passwords are stored as salted PBKDF2 SHA-256 hashes. The signed-in referee is shown in the application title and sidebar. When a match result is saved or corrected, the referee ID, time, tournament, match, and score are appended to `data/referee_audit.log`.

---

## 3. Technology Stack

- **Language**: C++20 (`-std=c++20`, concepts, `string_view`, modern STL algorithms)
- **GUI Framework**: SFML 2.6.2 (Graphics, Window, System - compiled statically)
- **Build System**: CMake (>= 3.24) with Ninja generator
- **Compiler**: MinGW-w64 GCC 16.2.0 (MSYS2 UCRT64 on Windows)
- **Persistence**: C++ File I/O (Atomic buffered text serialization)
- **Testing**: Native C++ Automated Test Suite (47 comprehensive test cases)

---

## 4. Architecture & Directory Structure

```
FootballTournamentManager/
├── CMakeLists.txt              # CMake root build script
├── README.md                   # Project overview & documentation
├── .gitignore                  # Git ignore rules
├── assets/                     # Assets bundled with executable
│   ├── fonts/                  # TrueType fonts (font.ttf, font-bold.ttf)
│   └── icons/
├── data/                       # Persistent database files
│   ├── tournaments.idx         # Tournament registry index
│   └── tournament_*.dat        # Individual tournament datasets
├── docs/                       # Academic submission documentation
│   ├── PROJECT_REPORT.md       # Comprehensive B.Tech project report
│   ├── USER_MANUAL.md          # Illustrated step-by-step user guide
│   ├── TEST_CASES.md           # 47 automated test cases specification
│   ├── VIVA_QUESTIONS.md       # Comprehensive OOP viva preparation
│   └── TEAM_CONTRIBUTIONS.md   # 4-Member responsibility breakdown
├── include/                    # Header interfaces
│   ├── models/                 # Player, Team, Match, Tournament
│   ├── managers/               # TournamentManager
│   ├── services/               # Fixtures, Standings, Stats, Validations
│   ├── storage/                # StorageInterface, FileRepository
│   └── ui/                     # UITheme, UIComponents, MainWindow
├── src/                        # Implementation files
│   ├── main.cpp                # Main executable entry point
│   ├── models/
│   ├── managers/
│   ├── services/
│   ├── storage/
│   └── ui/
├── tests/                      # Automated unit & integration tests
│   └── test_main.cpp           # 47 Automated test cases
└── thirdparty/                 # SFML 2.6.2 complete source tree
```

---

## 5. Prerequisites & Toolchain Setup

To build and run FTMS on Windows:

1. **MSYS2 UCRT64 Toolchain**:
   Install `gcc`, `cmake`, and `ninja`:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```
2. Ensure `C:\msys64\ucrt64\bin` is in your Windows environment `PATH`.

---

## 6. Build & Run Instructions

### Run from Visual Studio Code

1. Open `D:\FootballTournamentManager` as the folder in Visual Studio Code.
2. Choose **Terminal → Run Task… → Run Football Tournament Manager**.
3. On the first launch, choose **Create Referee ID**, make a unique ID and password, then sign in. On later launches, sign in with that account.

The task builds the project and launches the app. **Build Football Tournament Manager** is also available from the task menu when you only want to compile.

Open PowerShell and navigate to the project directory:

```powershell
# 1. Navigate to the project root
cd D:\FootballTournamentManager

# 2. Add MSYS2 UCRT64 toolchain to current PowerShell session PATH
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"

# 3. Configure the CMake build with Ninja
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release

# 4. Compile the project (executables and test suite)
cmake --build build

# 5. Run the Automated Tests (47 Unit & Integration tests)
.\build\run_tests.exe

# 6. Launch the Desktop GUI Application
.\build\FootballTournamentManager.exe
```

---

## 7. Automated Test Suite

Run the automated tests anytime using:
```powershell
.\build\run_tests.exe
```

Test coverage includes:
- **Test Group A**: Team & Player Validations (duplicate checks, jersey ranges 1-99, squad memberships).
- **Test Group B**: Circle-Method Fixture Algorithm ($n = 1$ to $6$ teams, odd-team BYEs, $n(n-1)/2$ verification).
- **Test Group C & D**: Match Results & Standings (Wins, Draws, Losses, GD, PTS, and dynamic score edit recalculations).
- **Test Group E**: Player Statistics & Leaderboard (Golden boot, multiple goals, tied top scorers).
- **Test Group F**: Tournament State Lifecycle (Draft $\rightarrow$ Fixtures $\rightarrow$ Active $\rightarrow$ Finalize $\rightarrow$ Reopen).
- **Test Group G**: Persistence & Safe Atomic Save/Load across restarts.

---

## 8. Data Storage & Persistence Details

All application data is persisted inside the `data/` directory relative to the running binary:
- `data/tournaments.idx`: Master catalog of all tournaments.
- `data/tournament_<id>.dat`: Dedicated structured file containing teams, players, fixtures, scores, and unique ID counters.
- **Safe Save**: Writes changes first to `.tmp` files before atomically renaming, preventing data truncation or corruption during power loss.

---

## 9. Known Limitations

- Fixtures currently generate for Single Round Robin format (Knockout/Cup format is planned for future scope).
- Offline desktop application (does not host web sockets or remote internet sync).
