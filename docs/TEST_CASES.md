# Test Cases

| ID | Input / action | Expected result | Status |
|---|---|---|---|
| FIX-01 | Generate schedules for 1–8 teams | Correct match counts; every pair appears once; no team appears twice per round | Pass |
| VAL-01 | Duplicate team registration (case/space normalized) | Registration rejected | Pass |
| VAL-02 | Duplicate jersey within a team | Registration rejected | Pass |
| MATCH-01 | Enter and correct a result | Updated score and scorers replace prior result | Pass |
| ST-01 | Results across a three-team sample | Correct P/W/D/L/GF/GA/GD/PTS ordering | Pass |
| STAT-01 | Credit goals to a registered player | Player totals match recorded scorer entries | Pass |
| FINAL-01 | Incomplete fixtures or unresolved first-place tie | Completion rejected | Pass |
| STORE-01 | Save and reload tournament | Teams, players, matches, results, and scorers preserved | Pass |
| STORE-02 | Load unsupported/malformed schema | Error reported; malformed records are not accepted | Pass |

Run the checks with `ctest --test-dir build --output-on-failure`. Current result: 1/1 CTest target passed. GUI interactions still need a manual Windows walkthrough.
