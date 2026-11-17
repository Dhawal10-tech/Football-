# PBL OOPS — Neymar Football Tournament Manager

This is the standalone C++ OOP football tournament manager prototype. It includes the workflows listed below; see Remaining work for features not included in this copy.

## Implemented features

- SFML 2.6.2 desktop window styled after the original manager, with dashboard, team squad, player registration, fixtures, and standings screens.
- `Team`, `Match`, and `Tournament` classes.
- The project starts with no example teams. Register teams and players using the forms in **Teams Squad** and **Players**.
- Fixture counts update automatically as teams are registered. Score entry is available after at least two teams exist.
- Click the sidebar to change screens. On Fixtures, select a match with the mouse or Up/Down, press F2, type both scores, use Tab to switch score fields, then press Enter.
- Automatic wins/draws/losses, goals, goal difference, points, and sorted standings.

## Remaining work

Editing/deleting teams and players, save/load, knockout stage, statistics/report export, and automated tests are not included in this prototype. Team names, managers and contacts, players, and scores are currently kept in memory and reset when the app closes.

## Open and run in VS Code

1. Open this `PBL OOPS` folder in VS Code (`C:\Users\Admin\Documents\ChatGPT\footballll\PBL OOPS`).
2. Select **Terminal → Run Task… → Run PBL OOPS** to configure, build, and launch the app.
3. To build only, select **Terminal → Run Build Task** or press `Ctrl+Shift+B`.

The included VS Code tasks use MSYS2 UCRT64 at `C:\msys64` and Ninja. CMake reuses SFML 2.6.2 from the parent project's `build/_deps` folder when available. If this folder is opened independently and that source is not present, CMake fetches the pinned SFML 2.6.2 source. The main source file is `neymar.cpp`.

### Manual terminal option

Open a PowerShell terminal in this folder and run:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& "C:\msys64\ucrt64\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe
& "C:\msys64\ucrt64\bin\cmake.exe" --build build
.\build\neymar.exe
```
