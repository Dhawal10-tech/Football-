# User Manual

## Build and launch

Open an MSYS2 UCRT64 terminal in the project directory, then run:

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
cmake --build build --config Debug
./build/latest/ftms.exe
```

On first launch, create a referee account and then sign in. The bundled DejaVu Sans font is loaded from `assets/fonts`. Tournament data and referee credentials are stored in the project `data` folder. Do not delete or manually edit those files while the application is open.

## Create and manage a tournament

1. Open **Tournaments**, choose **New tournament**, and enter a unique name, season, and optional description.
2. Select its row to make it the current tournament. The sidebar shows the current season and state.
3. Use **Edit** while it is a draft to correct tournament details. **Delete** asks for confirmation before removing the saved tournament.
4. Add teams and players before starting. Once fixtures exist, those records are locked to protect match references.

## Add teams and players

- In **Teams**, choose **Add team** and provide a name plus optional manager/contact. Names are unique within a tournament without regard to case or surrounding spaces. Select a row to edit or delete it. A team cannot be deleted if fixtures or players refer to it.
- In **Players**, choose **Register player**. The form lists numbered teams; enter a team number, jersey number from 1–99, and a position (Goalkeeper, Defender, Midfielder, or Forward). Select a row to edit/delete before fixtures are generated. Jersey numbers must be unique within each team.
- IDs are generated automatically and shown in the lists. For goalscorers, use player names from the selected match.

## Fixtures and results

1. With at least two teams registered, open **Fixtures** or **Tournaments** and choose **Generate & start fixtures** / **Start tournament**. The circle method schedules every pair once; odd team counts receive a bye.
2. Open **Matches & scores** and select a fixture. Enter both non-negative scores. Optional scorer records use `Player Name=Goals`, separated by commas. For example, if Messi scores twice and Mukesh once, enter `Messi=2, Mukesh=1`. The score form lists players eligible for that match. The goals credited to each team cannot exceed that team’s match score. The statistic total is based only on explicit scorer entries.
3. Select a completed match again to correct its result. Saving replaces the old score and scorer list, so repeated submission does not double-count.
4. Use `/` to open the search field and Enter to finish. The current list filters on the search phrase. Use the mouse wheel to scroll long lists.

## Standings, statistics, and completion

- **Points table** is recalculated from completed results: win 3, draw 1, loss 0. Display order is points, goal difference, goals scored, then team ID. Team ID is only a stable display order; a tie on the sporting criteria remains unresolved.
- **Player statistics** shows scorer totals and player IDs for score entry.
- In **Tournaments**, choose **Complete** only after every fixture is complete and one team uniquely leads under the published criteria. If the leader is tied, the app reports the issue and leaves the tournament active.
- A completed tournament can be reopened with **Reopen** for a correction workflow; it returns to Active and results can then be edited.

## History and reports

**History** lists completed tournaments, their state, teams, match count, and champion status. Select a historical tournament to view its teams, fixtures, standings, and player statistics in the other sections. **Reports** exports standings, player goals, and fixtures/results to `reports/<tournament-id>.csv` under the app’s current working directory.

## Troubleshooting

- If the app reports a data-load error, it refuses to overwrite the unreadable file. Back up `data/tournaments.ftms` before attempting recovery.
- If text does not appear, ensure `assets/fonts/DejaVuSans.ttf` exists beside the source tree and rebuild.
- A result can be saved only while its tournament is Active. Reopen a completed tournament before making a correction.
- First configuration downloads SFML 2.6.2 and therefore requires a network connection. Later builds use the cached source in `build/_deps`.
