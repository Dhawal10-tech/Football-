#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>
#include "managers/TournamentManager.hpp"
#include "ui/UIComponents.hpp"
#include "security/Authentication.hpp"

namespace ftms {

enum class ScreenType {
    Dashboard,
    TournamentList,
    TeamManagement,
    PlayerManagement,
    Fixtures,
    MatchScoreEntry,
    Standings,
    PlayerStats,
    WinnerSummary,
    HistoryReports,
    About
};

class MainWindow {
private:
    sf::RenderWindow window_;
    std::shared_ptr<TournamentManager> manager_;
    std::shared_ptr<const User> referee_;

    ScreenType currentScreen_{ScreenType::Dashboard};
    UIModalDialog modalDialog_;

    // Navigation Buttons (Sidebar)
    std::vector<std::pair<ScreenType, UIButton>> navButtons_;

    // UI State for Forms & Views
    // Tournament Screen controls
    UITextInput tourneyNameInput_;
    UITextInput tourneySeasonInput_;
    UITextInput tourneyDescInput_;
    UIButton createTourneyBtn_;
    UIButton selectTourneyBtn_;
    UIButton startTourneyBtn_;
    UIButton deleteTourneyBtn_;
    UITable tourneyTable_;

    // Team Screen controls
    UITextInput teamNameInput_;
    UITextInput teamManagerInput_;
    UITextInput teamContactInput_;
    UIButton addTeamBtn_;
    UIButton editTeamBtn_;
    UIButton deleteTeamBtn_;
    UITable teamTable_;
    UITable teamSquadTable_;

    // Player Screen controls
    UITextInput playerNameInput_;
    UITextInput playerJerseyInput_;
    UITextInput playerSearchInput_;
    UIButton addPlayerBtn_;
    UIButton deletePlayerBtn_;
    UITable playerTable_;
    int selectedPlayerPosIndex_{3}; // Default Forward

    // Fixture Screen controls
    UIButton generateFixturesBtn_;
    UIButton resetFixturesBtn_;
    UITable fixtureTable_;

    // Match Score Entry controls
    UITable matchTable_;
    UITextInput homeScoreInput_;
    UITextInput awayScoreInput_;
    UIButton saveScoreBtn_;
    int selectedMatchId_{-1};

    // Goalscorer entry controls
    UITextInput goalMinutesInput_;
    UIButton addGoalScorerBtn_;
    std::vector<GoalEvent> pendingGoalEvents_;
    UITable matchGoalEventsTable_;

    // Standings Screen controls
    UITable standingsTable_;
    UIButton exportStandingsBtn_;

    // Statistics Screen controls
    UITable statsTable_;
    UITable teamStatsTable_;

    // Winner Screen controls
    UIButton completeTourneyBtn_;
    UIButton reopenTourneyBtn_;

    // History / Reports Screen controls
    UITable historyTable_;
    UIButton exportFullReportBtn_;

    void setupNavigation();
    void setupScreenControls();
    void refreshActiveScreenData();

    // Event & Rendering dispatchers
    void handleEvent(const sf::Event& event);
    void update(const sf::Vector2f& mousePos);
    void render();

    // Sub-renderers
    void renderSidebar();
    void renderDashboard();
    void renderTournamentList();
    void renderTeamManagement();
    void renderPlayerManagement();
    void renderFixtures();
    void renderMatchScoreEntry();
    void renderStandings();
    void renderPlayerStats();
    void renderWinnerSummary();
    void renderHistoryReports();
    void renderAbout();

public:
    MainWindow(std::shared_ptr<TournamentManager> manager,
               std::shared_ptr<const User> referee);
    void run();
};

} // namespace ftms
