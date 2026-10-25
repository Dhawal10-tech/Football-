#include "ftms/services.hpp"
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace ftms {
std::string ValidationService::normalize(const std::string& s) {
    auto first=s.find_first_not_of(" \t\r\n"), last=s.find_last_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    std::string out=s.substr(first,last-first+1);
    std::transform(out.begin(),out.end(),out.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return out;
}
void ValidationService::addTeam(const Tournament& t,const Team& team) {
    if(normalize(team.id).empty()||normalize(team.name).empty()) throw std::invalid_argument("Team ID and name are required.");
    for(const auto& x:t.teams) { if(x.id==team.id) throw std::invalid_argument("Team ID already exists."); if(normalize(x.name)==normalize(team.name)) throw std::invalid_argument("A team with this name already exists in the tournament."); }
    if(t.state!=TournamentState::Draft || !t.matches.empty()) throw std::logic_error("Teams can only be changed before fixtures are generated.");
}
void ValidationService::addPlayer(const Tournament& t,const Player& p) {
    if(normalize(p.id).empty()||normalize(p.name).empty()||p.jersey==0) throw std::invalid_argument("Player ID, name and a positive jersey number are required.");
    if(std::none_of(t.teams.begin(),t.teams.end(),[&](const Team& x){return x.id==p.teamId;})) throw std::invalid_argument("Player must belong to a registered team.");
    for(const auto& x:t.players) { if(x.id==p.id) throw std::invalid_argument("Player ID already exists."); if(x.teamId==p.teamId&&x.jersey==p.jersey) throw std::invalid_argument("Jersey number already exists for this team."); }
    if(t.state!=TournamentState::Draft || !t.matches.empty()) throw std::logic_error("Players can only be changed before fixtures are generated.");
}
void ValidationService::validateResult(const Tournament& t,const Match& m,unsigned h,unsigned a,const std::vector<Scorer>& scorers) {
    if(t.state!=TournamentState::Active) throw std::logic_error("Results can only be recorded while the tournament is active.");
    if(h>1000||a>1000) throw std::invalid_argument("Scores must be non-negative integers no greater than 1000.");
    if(m.homeTeamId==m.awayTeamId) throw std::invalid_argument("A team cannot play itself.");
    std::set<std::string> seen;
    for(auto& s:scorers) {
        auto p=std::find_if(t.players.begin(),t.players.end(),[&](const Player& x){return x.id==s.playerId;});
        if(p==t.players.end()) throw std::invalid_argument("Goalscorer is not a registered player.");
        if(p->teamId!=m.homeTeamId&&p->teamId!=m.awayTeamId) throw std::invalid_argument("Goalscorer must belong to a team in this match.");
        if(!seen.insert(s.playerId).second) throw std::invalid_argument("Use one goalscorer entry per player.");
        if(s.goals==0||s.goals>(p->teamId==m.homeTeamId?h:a)) throw std::invalid_argument("Goalscorer total exceeds that player's team's match score.");
    }
}
std::vector<Match> RoundRobinFormat::generate(const Tournament& t) const {
    if(t.teams.empty()) throw std::invalid_argument("Add at least one team before previewing fixtures.");
    if(t.teams.size()==1) return {};
    if(!t.matches.empty()) throw std::logic_error("Fixtures already exist; regeneration is not allowed.");
    std::vector<std::string> ids; for(auto& team:t.teams) ids.push_back(team.id);
    if(ids.size()%2) ids.push_back("");
    const auto n=ids.size(); std::vector<Match> result; unsigned round=1;
    for(size_t r=0;r+1<n;++r) {
        for(size_t i=0;i<n/2;++i) { auto a=ids[i],b=ids[n-1-i]; if(a.empty()||b.empty()) continue; if(r%2) std::swap(a,b);
            result.push_back(Match{"M"+std::to_string(result.size()+1),a,b,{},round,MatchState::Scheduled,{},{},{}});
        }
        auto last=ids.back(); ids.pop_back(); ids.insert(ids.begin()+1,last); ++round;
    }
    const size_t expected=t.teams.size()*(t.teams.size()-1)/2;
    if(result.size()!=expected) throw std::logic_error("Fixture generator produced an unexpected number of matches.");
    return result;
}
std::vector<Standing> StandingsCalculator::calculate(const Tournament& t) {
    std::vector<Standing> table; for(auto& team:t.teams)table.push_back(Standing{team.id,team.name});
    std::unordered_map<std::string,size_t> ix; for(size_t i=0;i<table.size();++i)ix[table[i].teamId]=i;
    for(auto& m:t.matches) if(m.state==MatchState::Completed&&m.homeScore&&m.awayScore) {
        if(!ix.contains(m.homeTeamId)||!ix.contains(m.awayTeamId)) throw std::runtime_error("Match references an unknown team.");
        auto& h=table[ix.at(m.homeTeamId)]; auto& a=table[ix.at(m.awayTeamId)]; ++h.played;++a.played;
        h.goalsFor+=*m.homeScore;h.goalsAgainst+=*m.awayScore;a.goalsFor+=*m.awayScore;a.goalsAgainst+=*m.homeScore;
        if(*m.homeScore>*m.awayScore){++h.won;++a.lost;h.points+=3;} else if(*m.homeScore<*m.awayScore){++a.won;++h.lost;a.points+=3;} else {++h.drawn;++a.drawn;++h.points;++a.points;}
    }
    for(auto& s:table)s.goalDifference=static_cast<int>(s.goalsFor)-static_cast<int>(s.goalsAgainst);
    std::sort(table.begin(),table.end(),[](const Standing&a,const Standing&b){if(a.points!=b.points)return a.points>b.points;if(a.goalDifference!=b.goalDifference)return a.goalDifference>b.goalDifference;if(a.goalsFor!=b.goalsFor)return a.goalsFor>b.goalsFor;return a.teamId<b.teamId;});
    return table;
}
std::vector<PlayerGoals> StatisticsManager::goals(const Tournament& t) {
    std::unordered_map<std::string,unsigned> totals;
    for(auto& m:t.matches) if(m.state==MatchState::Completed) for(auto& s:m.scorers) totals[s.playerId]+=s.goals;
    std::vector<PlayerGoals> result;
    for(auto& p:t.players) {auto team=std::find_if(t.teams.begin(),t.teams.end(),[&](const Team&x){return x.id==p.teamId;});result.push_back({p.id,p.name,team==t.teams.end()?"[missing team]":team->name,p.jersey,totals[p.id]});}
    std::sort(result.begin(),result.end(),[](auto&a,auto&b){if(a.goals!=b.goals)return a.goals>b.goals;return a.playerId<b.playerId;}); return result;
}
void TournamentManager::start(Tournament& t) { if(t.state!=TournamentState::Draft) throw std::logic_error("Only a draft tournament can be started.");if(t.teams.size()<2)throw std::logic_error("At least two teams are required to start a tournament."); RoundRobinFormat format; auto matches=format.generate(t);t.matches=std::move(matches);t.state=TournamentState::Active; }
void TournamentManager::recordResult(Tournament& t,const std::string& id,unsigned h,unsigned a,std::vector<Scorer> scorers) {
    if(t.state!=TournamentState::Active) throw std::logic_error("Tournament is not active.");
    auto it=std::find_if(t.matches.begin(),t.matches.end(),[&](const Match&m){return m.id==id;});
    if(it==t.matches.end()) throw std::invalid_argument("Match was not found.");
    ValidationService::validateResult(t,*it,h,a,scorers); it->homeScore=h;it->awayScore=a;it->scorers=std::move(scorers);it->state=MatchState::Completed;
}
void TournamentManager::finalize(Tournament& t) {
    if(t.state!=TournamentState::Active||t.matches.empty())throw std::logic_error("An active tournament with fixtures is required.");
    if(std::any_of(t.matches.begin(),t.matches.end(),[](auto&m){return m.state!=MatchState::Completed;}))throw std::logic_error("All fixtures must be completed before finalization.");
    auto s=StandingsCalculator::calculate(t);
    const bool tied=s.size()>=2&&s[0].points==s[1].points&&s[0].goalDifference==s[1].goalDifference&&s[0].goalsFor==s[1].goalsFor;
    if(s.size()<2||tied) throw std::logic_error("The champion is tied under points, goal difference and goals scored; resolve the tie explicitly.");
    t.state=TournamentState::Completed;
}
void TournamentManager::reopen(Tournament& t){if(t.state!=TournamentState::Completed)throw std::logic_error("Only a completed tournament can be reopened.");t.state=TournamentState::Active;}
}
