# Manual Verification Checklist

The consolidated root project does not currently include an automated test target. Use this checklist for a manual review before a presentation or release.

| Area | Action | Expected result |
|---|---|---|
| Referee access | Create an account, sign out, and sign in again | A referee can create and use a local account |
| Registration | Add a tournament, two or more teams, and players | Records appear in their lists and persist after restart |
| Fixtures | Start a tournament with an even and an odd number of teams | Each pair is scheduled once; odd schedules include a bye |
| Results | Enter a score and scorer names using `Name=Goals` | The result saves and scorer totals update |
| Validation | Enter a scorer not in the selected match or a goal count that exceeds the team score | The app explains why it rejected the entry |
| Standings | Record wins, draws, and losses | Points and goal statistics update from completed matches |
| Corrections | Edit a completed match result | The old score and scorer values are replaced |
| Export | Export a CSV report | A report appears in `reports/<tournament-id>.csv` under the app working directory |
| Recovery | Close and reopen the application | Tournament records remain available |
