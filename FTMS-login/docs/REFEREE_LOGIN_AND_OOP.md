# Referee login and the four OOP pillars

## Referee account flow

1. Launch FTMS. The tournament screens stay behind the login window.
2. The first referee selects **Create Referee ID**, enters a unique ID, a password of at least eight characters, and confirms the password.
3. The referee signs in with that ID and password. Reusing an ID is rejected without regard to letter case.
4. The signed-in referee can enter and correct scores. Each successful result update is recorded in `data/referee_audit.log` with the referee ID, local time, tournament, match, and score.

Account credentials are saved in `data/referees.db`. The file stores a random salt and a PBKDF2 SHA-256 password hash, not the entered password.

## How the code demonstrates OOP

- **Abstraction:** `IAuthenticationService` defines account registration and authentication without exposing how credentials are saved. `User` defines the common user and permission contract.
- **Encapsulation:** `User` keeps the referee ID, salt, and password hash private. `FileAuthenticationService` owns the account collection and enforces unique IDs and password rules.
- **Inheritance:** `Referee` derives from the abstract `User` class and supplies referee role behavior.
- **Polymorphism:** `LoginWindow` calls the authentication interface, while the application checks `User::canRecordScores()` through the base class. `Referee` provides the concrete permission behavior; `clone()` returns a role-specific object through `User`.

The score-entry action also checks the authenticated user's permission before it calls `TournamentManager::recordMatchResult`.
