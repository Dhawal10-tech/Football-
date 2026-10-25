# Viva Questions and Answers

**Why separate services from the GUI?** Tournament rules can be tested without opening a window, and GUI callbacks do not own standings calculations.

**Where is abstraction used?** `TournamentFormat` and `StorageInterface` define behavior while concrete scheduling and storage implementations provide the details.

**How are fixtures generated?** The circle method fixes one team, rotates the others, and pairs opposite positions each round. An empty slot represents a bye for an odd team count.

**How are standings calculated?** Each completed match contributes goals and outcome counters. Wins earn three points and draws one. The table is rebuilt from match results.

**How are goalscorers handled?** Only explicitly entered scorer records count. Each scorer must be registered, belong to a team in the match, and have no more goals than their team scored.

**How does persistence work?** Tagged records with quoted strings are saved in schema version 1. Loading checks IDs and references. Saving stages a temporary file and retains the prior file during replacement.

**What OOP concepts appear?** Services encapsulate responsibilities, interfaces provide abstraction and runtime polymorphism, and tournaments compose teams, players, and matches.

**What is the tie policy?** Display order uses points, goal difference, goals scored, then stable ID. Stable ID is for deterministic display only; completion is rejected when the sporting criteria tie.

**What does the UI support?** Tournament, team, player, fixture, match, standings, statistics, history, and report pages, with forms, search, confirmation prompts, and scrolling.

**What remains to verify?** Manually inspect the interface and walk through a full tournament on Windows, including CSV export and restarting with saved data.
