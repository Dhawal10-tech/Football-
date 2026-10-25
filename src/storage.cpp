#include "ftms/storage.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <set>
#include <stdexcept>

namespace ftms {
namespace {
void check(bool ok,const std::filesystem::path& p){if(!ok)throw std::runtime_error("Invalid or corrupt tournament data in "+p.string());}
}
FileRepository::FileRepository(std::filesystem::path p):path_(std::move(p)){}
std::vector<Tournament> FileRepository::load() const {
    std::ifstream in(path_); if(!in){if(!std::filesystem::exists(path_))return {};throw std::runtime_error("Could not open tournament data file: "+path_.string());}
    std::string line;check(static_cast<bool>(std::getline(in,line))&&line=="FTMS 1",path_);
    std::vector<Tournament> ts; std::string tag;
    while(std::getline(in,line)) {
        if(line.empty()) continue;
        std::istringstream row(line); row>>tag;
        if(tag=="T") { Tournament t;int st;row>>std::quoted(t.id)>>std::quoted(t.name)>>std::quoted(t.season)>>std::quoted(t.description)>>st;check(static_cast<bool>(row)&&st>=0&&st<=2,path_);t.state=static_cast<TournamentState>(st);ts.push_back(std::move(t)); }
        else if(tag=="E") { std::string id;row>>std::quoted(id);check(static_cast<bool>(row)&&!ts.empty()&&ts.back().id==id,path_); }
        else {
            check(!ts.empty(),path_); auto& t=ts.back();
            if(tag=="TEAM"){Team x;row>>std::quoted(x.id)>>std::quoted(x.name)>>std::quoted(x.manager)>>std::quoted(x.contact);check(static_cast<bool>(row),path_);t.teams.push_back(std::move(x));}
            else if(tag=="PLAYER"){Player x;int pos;row>>std::quoted(x.id)>>std::quoted(x.name)>>std::quoted(x.teamId)>>x.jersey>>pos;check(static_cast<bool>(row)&&pos>=0&&pos<=3,path_);x.position=static_cast<Position>(pos);t.players.push_back(std::move(x));}
            else if(tag=="MATCH"){Match x;int state,hh,ha;unsigned hs=0,as=0;row>>std::quoted(x.id)>>std::quoted(x.homeTeamId)>>std::quoted(x.awayTeamId)>>std::quoted(x.dateTime)>>x.round>>state>>hh>>hs>>ha>>as;check(static_cast<bool>(row)&&state>=0&&state<=1&&hh>=0&&hh<=1&&ha>=0&&ha<=1,path_);x.state=static_cast<MatchState>(state);if(hh)x.homeScore=hs;if(ha)x.awayScore=as;t.matches.push_back(std::move(x));}
            else if(tag=="SCORER"){std::string mid;Scorer x;row>>std::quoted(mid)>>std::quoted(x.playerId)>>x.goals;check(static_cast<bool>(row),path_);auto m=std::find_if(t.matches.begin(),t.matches.end(),[&](const Match&v){return v.id==mid;});check(m!=t.matches.end(),path_);m->scorers.push_back(std::move(x));}
            else check(false,path_);
        }
    }
    check(in.eof(),path_);
    std::set<std::string> tids;
    for(const auto&t:ts){check(!t.id.empty()&&tids.insert(t.id).second,path_);std::set<std::string> teams,players,matches;for(const auto&x:t.teams)check(!x.id.empty()&&teams.insert(x.id).second,path_);for(const auto&x:t.players)check(!x.id.empty()&&players.insert(x.id).second&&teams.contains(x.teamId),path_);for(const auto&m:t.matches)check(!m.id.empty()&&matches.insert(m.id).second&&teams.contains(m.homeTeamId)&&teams.contains(m.awayTeamId)&&m.homeTeamId!=m.awayTeamId&&(m.homeScore.has_value()==m.awayScore.has_value())&&((m.state==MatchState::Completed)==m.homeScore.has_value()),path_);for(const auto&m:t.matches){std::set<std::string> scorerIds;for(const auto&s:m.scorers){auto p=std::find_if(t.players.begin(),t.players.end(),[&](const Player&v){return v.id==s.playerId;});check(p!=t.players.end()&&scorerIds.insert(s.playerId).second&&s.goals>0&&m.state==MatchState::Completed&&(p->teamId==m.homeTeamId||p->teamId==m.awayTeamId)&&s.goals<=(p->teamId==m.homeTeamId?*m.homeScore:*m.awayScore),path_);}}}
    return ts;
}
void FileRepository::save(const std::vector<Tournament>& ts) const {
    if(!path_.parent_path().empty())std::filesystem::create_directories(path_.parent_path());
    auto temp=path_;temp += ".tmp";auto backup=path_;backup += ".bak";
    {std::ofstream out(temp,std::ios::trunc);if(!out)throw std::runtime_error("Could not create temporary data file.");out<<"FTMS 1\n";
    for(const auto&t:ts){out<<"T "<<std::quoted(t.id)<<' '<<std::quoted(t.name)<<' '<<std::quoted(t.season)<<' '<<std::quoted(t.description)<<' '<<static_cast<int>(t.state)<<'\n';for(const auto&x:t.teams)out<<"TEAM "<<std::quoted(x.id)<<' '<<std::quoted(x.name)<<' '<<std::quoted(x.manager)<<' '<<std::quoted(x.contact)<<'\n';for(const auto&x:t.players)out<<"PLAYER "<<std::quoted(x.id)<<' '<<std::quoted(x.name)<<' '<<std::quoted(x.teamId)<<' '<<x.jersey<<' '<<static_cast<int>(x.position)<<'\n';for(const auto&m:t.matches){out<<"MATCH "<<std::quoted(m.id)<<' '<<std::quoted(m.homeTeamId)<<' '<<std::quoted(m.awayTeamId)<<' '<<std::quoted(m.dateTime)<<' '<<m.round<<' '<<static_cast<int>(m.state)<<' '<<(m.homeScore?1:0)<<' '<<m.homeScore.value_or(0)<<' '<<(m.awayScore?1:0)<<' '<<m.awayScore.value_or(0)<<'\n';for(const auto&s:m.scorers)out<<"SCORER "<<std::quoted(m.id)<<' '<<std::quoted(s.playerId)<<' '<<s.goals<<'\n';}out<<"E "<<std::quoted(t.id)<<'\n';}
     out.flush();if(!out)throw std::runtime_error("Failed writing tournament data; existing data was preserved.");}
    bool moved=false;try{if(std::filesystem::exists(path_)){std::error_code ec;std::filesystem::remove(backup,ec);std::filesystem::rename(path_,backup);moved=true;}std::filesystem::rename(temp,path_);if(moved){std::error_code ec;std::filesystem::remove(backup,ec);}}catch(...){if(moved&&!std::filesystem::exists(path_)){std::error_code ec;std::filesystem::rename(backup,path_,ec);}throw;}
}
}
