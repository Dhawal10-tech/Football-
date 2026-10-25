#include "ftms/services.hpp"
#include "ftms/storage.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace ftms;

static void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
template<class F> static void mustThrow(F&& f,const char* message){bool threw=false;try{f();}catch(const std::exception&){threw=true;}require(threw,message);}

int main(){try{
    Tournament one;one.teams.push_back({"T1","Only team",{}, {}});
    require(RoundRobinFormat{}.generate(one).empty(),"one-team tournament has no pairings");
    for(unsigned n=2;n<=8;++n){
        Tournament t;for(unsigned i=0;i<n;++i)t.teams.push_back({"T"+std::to_string(i),"Team "+std::to_string(i),{}, {}});
        auto matches=RoundRobinFormat{}.generate(t);require(matches.size()==n*(n-1)/2,"round robin match count");
        std::set<std::pair<std::string,std::string>> pairs;std::set<unsigned> rounds;
        for(const auto&m:matches){require(m.homeTeamId!=m.awayTeamId,"self match");auto a=m.homeTeamId,b=m.awayTeamId;if(a>b)std::swap(a,b);require(pairs.emplace(a,b).second,"duplicate pairing");rounds.insert(m.round);}
        require(pairs.size()==matches.size(),"pair uniqueness");require(rounds.size()==(n%2?n:n-1),"round count with BYE");
        for(const auto round:rounds){std::set<std::string> playing;for(const auto&m:matches)if(m.round==round){require(playing.insert(m.homeTeamId).second,"team plays twice in a round");require(playing.insert(m.awayTeamId).second,"team plays twice in a round");}}
    }
    Tournament t{"cup","College Cup","2026","",TournamentState::Draft,{{"a","Alpha"},{"b","Beta"},{"c","Gamma"}},{{"p1","A Player","b",9,Position::Forward},{"p2","B Player","a",10,Position::Forward}}, {}};
    mustThrow([&]{ValidationService::addTeam(t,{"x"," alpha ",{},{}});},"duplicate team rejected");
    mustThrow([&]{ValidationService::addPlayer(t,{"p3","Another","a",10,Position::Defender});},"duplicate jersey rejected");
    mustThrow([&]{ValidationService::addPlayer(t,{"p3","Another","missing",11,Position::Defender});},"invalid team assignment rejected");
    TournamentManager::start(t);require(t.matches.size()==3,"three-team fixtures");
    const auto first=t.matches[0].id;TournamentManager::recordResult(t,first,2,0,{{"p1",2}});
    TournamentManager::recordResult(t,t.matches[1].id,1,1);mustThrow([&]{TournamentManager::finalize(t);},"pending matches block finalization");TournamentManager::recordResult(t,t.matches[2].id,0,3);
    auto table=StandingsCalculator::calculate(t);require(table.size()==3,"standings size");require(table[0].played==2&&table[0].won==2&&table[0].points==6,"win standings arithmetic");
    require(StatisticsManager::goals(t)[0].goals==2,"scorer total");
    TournamentManager::recordResult(t,first,1,0,{{"p1",1}});require(StatisticsManager::goals(t)[0].goals==1,"result correction replaces scorer total");
    mustThrow([&]{TournamentManager::recordResult(t,first,static_cast<unsigned>(-1),0);},"negative score rejected");
    mustThrow([&]{TournamentManager::recordResult(t,first,2,0,{{"p2",1}});},"opponent scorer cannot be credited to wrong match team");

    Tournament tied{"tie","Tie Cup","2026","",TournamentState::Draft,{{"a","A"},{"b","B"}}, {}, {}};
    TournamentManager::start(tied);mustThrow([&]{TournamentManager::finalize(tied);},"pending result blocks finalization");
    TournamentManager::recordResult(tied,tied.matches[0].id,0,0);mustThrow([&]{TournamentManager::finalize(tied);},"unresolved standings tie blocks champion");
    TournamentManager::recordResult(tied,tied.matches[0].id,2,0);TournamentManager::finalize(tied);require(tied.state==TournamentState::Completed,"unique leader finalizes");
    TournamentManager::reopen(tied);require(tied.state==TournamentState::Active,"explicit reopen enables corrections");

    const auto path=std::filesystem::temp_directory_path()/"ftms-core-test.ftms";FileRepository repo(path);
    repo.save({t,tied});auto loaded=repo.load();require(loaded.size()==2&&loaded[0].matches.size()==3&&loaded[0].matches[0].scorers[0].goals==1,"persistence round trip");
    std::filesystem::remove(path);require(repo.load().empty(),"missing file gives empty new repository");
    {std::ofstream corrupt(path);corrupt<<"FTMS 99\n";}mustThrow([&]{repo.load();},"unsupported schema rejected");std::filesystem::remove(path);

    std::cout<<"All core tests passed.\n";return 0;
}catch(const std::exception&e){std::cerr<<"Test failure: "<<e.what()<<'\n';return 1;}}
