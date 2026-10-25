#pragma once
#include "ftms/domain.hpp"
#include <string>
#include <vector>

namespace ftms {
class ValidationService {
public:
    static std::string normalize(const std::string& value);
    static void addTeam(const Tournament&, const Team&);
    static void addPlayer(const Tournament&, const Player&);
    static void validateResult(const Tournament&, const Match&, unsigned, unsigned, const std::vector<Scorer>&);
};
class TournamentFormat { public: virtual ~TournamentFormat() = default; virtual std::vector<Match> generate(const Tournament&) const = 0; };
class RoundRobinFormat final : public TournamentFormat { public: std::vector<Match> generate(const Tournament&) const override; };
class StandingsCalculator { public: static std::vector<Standing> calculate(const Tournament&); };
class StatisticsManager { public: static std::vector<PlayerGoals> goals(const Tournament&); };
class TournamentManager {
public:
    static void start(Tournament&);
    static void recordResult(Tournament&, const std::string&, unsigned, unsigned, std::vector<Scorer> = {});
    static void finalize(Tournament&);
    static void reopen(Tournament&);
};
}
