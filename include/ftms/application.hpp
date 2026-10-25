#pragma once
#include <SFML/Graphics.hpp>
#include "ftms/storage.hpp"
namespace ftms {
class Application {
    enum class Page { Dashboard, Tournaments, Teams, Players, Fixtures, Matches, Standings, Statistics, History, Reports, About };
    enum class Dialog { None, Tournament, Team, Player, Score, EditTournament, EditTeam, EditPlayer, ConfirmDeleteTournament, ConfirmDeleteTeam, ConfirmDeletePlayer };
    sf::RenderWindow window_;
    sf::Font font_;
    bool fontLoaded_{};
    FileRepository repository_;
    std::vector<Tournament> tournaments_;
    size_t active_{};
    Page page_{Page::Dashboard};
    Dialog dialog_{Dialog::None};
    std::vector<std::string> fields_, labels_;
    size_t focusedField_{};
    std::string message_, loadError_, search_;
    std::string selectedMatchId_;
    bool messageIsError_{}, searchFocused_{};
    int selectedRow_{-1}, scroll_{};
    sf::Vector2i mouse_{};
    void drawText(const std::string&,float,float,unsigned,sf::Color=sf::Color::White);
    void drawButton(const std::string&,const sf::FloatRect&,bool enabled=true,bool accent=false);
    void drawCard(const sf::FloatRect&,sf::Color=sf::Color(31,57,46));
    Tournament* activeTournament();
    const Tournament* activeTournament() const;
    void beginDialog(Dialog,std::vector<std::string>,std::vector<std::string>);
    void submitDialog();
    void saveOrRollback(std::vector<Tournament> before);
    std::string nextId(const std::string&,const std::vector<std::string>&) const;
    void drawSidebar();
    void drawPage();
    void drawDialog();
    void handleClick(sf::Vector2f);
    void showMessage(std::string,bool error=false);
    void exportReport();
public:
    Application();
    int run();
};
}
