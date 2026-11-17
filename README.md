# Football Tournament Management System (FTMS)

FTMS is a C++20 desktop application for organizing football tournaments. It uses SFML for its graphical interface and stores tournament records locally.

## Features

- Referee sign-in and first-run referee account creation.
- Tournament, team, and player registration and management.
- Round-robin fixture generation, including byes for odd team counts.
- Match score entry and correction. Goalscorers can be entered by player name, for example `Messi=2, Mukesh=1`.
- Automatically calculated standings and player goal statistics.
- Tournament history, search, CSV report export, and local file persistence.
- Dark sports dashboard with forms, match cards, tables, and navigation.

## Technology and OOP

- C++20, SFML 2.6.2, CMake, and Ninja.
- `src/main.cpp` contains the current consolidated application.
- The four OOP pillars appear in the application: `User` is an abstract base class and `Referee` inherits from it; service interfaces and implementations provide abstraction and runtime polymorphism; class state is encapsulated; and domain objects compose tournament records.
- The referee authentication store is created locally at `data/referees.db`. It is intentionally not included in Git.

## Build and run on Windows

Install Visual Studio Code, the CMake Tools and C/C++ extensions, and MSYS2 UCRT64 with GCC, CMake, and Ninja. Open the repository folder in VS Code. In a UCRT64 terminal at the repository root, run:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build --config Debug
```

The first configure downloads the pinned SFML 2.6.2 source and therefore needs an internet connection. Start the application with:

```powershell
./build/latest/ftms.exe
```

Or run `build/latest/ftms.exe` from the VS Code Explorer. On first launch, select **Create referee account** and then sign in. CMake copies the required runtime DLLs beside the executable.

## Data and reports

Tournament data is stored in `data/tournaments.ftms`; referee credentials are kept separately in `data/referees.db`. CSV reports are written under a `reports` folder relative to the app’s current working directory, using the tournament ID as the filename.

## Repository folders

- The repository root is the current FTMS application.
- `FTMS-login/` and `PBL OOPS/` contain earlier standalone project versions and their documentation.
- Build output, local tournament data, referee credentials, and temporary Code Runner files are excluded from Git.

## Documentation

See `docs/USER_MANUAL.md` for workflows, `docs/PROJECT_REPORT.md` for the project overview, and `docs/VIVA_QUESTIONS.md` for review notes.
