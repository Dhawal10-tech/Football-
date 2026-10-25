#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ftms {
enum class TournamentState { Draft, Active, Completed };
enum class Position { Goalkeeper, Defender, Midfielder, Forward };
enum class MatchState { Scheduled, Completed };

struct Player {
    std::string id, name, teamId;
    unsigned jersey{};
    Position position{Position::Midfielder};
};
struct Team { std::string id, name, manager, contact; };
struct Scorer { std::string playerId; unsigned goals{}; };
struct Match {
    std::string id, homeTeamId, awayTeamId, dateTime;
    unsigned round{};
    MatchState state{MatchState::Scheduled};
    std::optional<unsigned> homeScore, awayScore;
    std::vector<Scorer> scorers;
};
struct Tournament {
    std::string id, name, season, description;
    TournamentState state{TournamentState::Draft};
    std::vector<Team> teams;
    std::vector<Player> players;
    std::vector<Match> matches;
};
struct Standing {
    std::string teamId, teamName;
    unsigned played{}, won{}, drawn{}, lost{}, goalsFor{}, goalsAgainst{}, points{};
    int goalDifference{};
};
struct PlayerGoals { std::string playerId, playerName, teamName; unsigned jersey{}, goals{}; };
}
