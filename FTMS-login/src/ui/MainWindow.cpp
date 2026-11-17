#include "ui/MainWindow.hpp"
#include "ui/UITheme.hpp"
#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>

namespace ftms {

MainWindow::MainWindow(std::shared_ptr<TournamentManager> manager,
                       std::shared_ptr<const User> referee)
    : manager_(std::move(manager)), referee_(std::move(referee)) {
    sf::ContextSettings settings;
    settings.antialiasingLevel = 4;

    window_.create(sf::VideoMode(1280, 768), "Football Tournament Management System (FTMS)", sf::Style::Default, settings);
    ShowWindow(window_.getSystemHandle(), SW_MAXIMIZE);
    window_.setTitle("Football Tournament Manager | Signed in: " + (referee_ ? referee_->userId() : "No referee"));
    window_.setFramerateLimit(60);

    UITheme::loadFonts();
    setupNavigation();
    setupScreenControls();
    refreshActiveScreenData();
}

void MainWindow::setupNavigation() {
    navButtons_.clear();
    const float startY = 120.f;
    const float btnH = 40.f;
    const float spacing = 6.f;
    const float btnW = 190.f;

    std::vector<std::pair<ScreenType, std::string>> navItems = {
        {ScreenType::Dashboard, "  Dashboard"},
        {ScreenType::TournamentList, "  Tournaments"},
        {ScreenType::TeamManagement, "  Teams Squad"},
        {ScreenType::PlayerManagement, "  Players"},
        {ScreenType::Fixtures, "  Fixtures"},
        {ScreenType::MatchScoreEntry, "  Score Entry"},
        {ScreenType::Standings, "  Points Table"},
        {ScreenType::PlayerStats, "  Player Stats"},
        {ScreenType::WinnerSummary, "  Winner & Final"},
        {ScreenType::HistoryReports, "  History / Report"},
        {ScreenType::About, "  About System"}
    };

    for (std::size_t i = 0; i < navItems.size(); ++i) {
        sf::Vector2f pos(15.f, startY + static_cast<float>(i) * (btnH + spacing));
        UIButton btn(pos, sf::Vector2f(btnW, btnH), navItems[i].second, UIButton::Variant::Ghost);
        ScreenType scr = navItems[i].first;
        btn.setOnClick([this, scr]() {
            currentScreen_ = scr;
            refreshActiveScreenData();
        });
        navButtons_.emplace_back(scr, btn);
    }
}

void MainWindow::setupScreenControls() {
    // Tournament Screen
    tourneyNameInput_ = UITextInput(sf::Vector2f(250.f, 130.f), sf::Vector2f(280.f, 38.f), "Tournament Name");
    tourneySeasonInput_ = UITextInput(sf::Vector2f(545.f, 130.f), sf::Vector2f(160.f, 38.f), "Season/Year");
    tourneyDescInput_ = UITextInput(sf::Vector2f(720.f, 130.f), sf::Vector2f(340.f, 38.f), "Description / Notes");
    createTourneyBtn_ = UIButton(sf::Vector2f(1075.f, 130.f), sf::Vector2f(160.f, 38.f), "+ Create New", UIButton::Variant::Primary);

    createTourneyBtn_.setOnClick([this]() {
        auto val = manager_->createTournament(tourneyNameInput_.getText(), tourneySeasonInput_.getText(), tourneyDescInput_.getText());
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Validation Error", val.errorMessage);
        } else {
            tourneyNameInput_.clear();
            tourneySeasonInput_.clear();
            tourneyDescInput_.clear();
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Success", "Tournament created successfully!");
        }
    });

    tourneyTable_ = UITable(sf::Vector2f(250.f, 240.f), sf::Vector2f(985.f, 430.f),
                           {"ID", "Tournament Name", "Season", "Status", "Format", "Champion"},
                           {60.f, 260.f, 120.f, 130.f, 150.f, 200.f});

    selectTourneyBtn_ = UIButton(sf::Vector2f(250.f, 690.f), sf::Vector2f(160.f, 40.f), "Select & Open", UIButton::Variant::Primary);
    selectTourneyBtn_.setOnClick([this]() {
        int id = tourneyTable_.getSelectedRowId();
        if (id <= 0) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Selection Required", "Please select a tournament from the table first.");
            return;
        }
        manager_->selectTournament(id);
        refreshActiveScreenData();
        modalDialog_.show(UIModalDialog::Type::Alert, "Tournament Opened", "Selected tournament is now active.");
    });

    startTourneyBtn_ = UIButton(sf::Vector2f(425.f, 690.f), sf::Vector2f(160.f, 40.f), "Start Tournament", UIButton::Variant::Secondary);
    startTourneyBtn_.setOnClick([this]() {
        auto val = manager_->startActiveTournament();
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Cannot Start", val.errorMessage);
        } else {
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Tournament Started", "Tournament is now in ACTIVE state! Matches can be played.");
        }
    });

    deleteTourneyBtn_ = UIButton(sf::Vector2f(600.f, 690.f), sf::Vector2f(160.f, 40.f), "Delete Tournament", UIButton::Variant::Danger);
    deleteTourneyBtn_.setOnClick([this]() {
        int id = tourneyTable_.getSelectedRowId();
        if (id <= 0) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Selection Required", "Please select a tournament to delete.");
            return;
        }
        modalDialog_.show(UIModalDialog::Type::Confirm, "Confirm Deletion",
                          "Are you sure you want to permanently delete tournament #" + std::to_string(id) + "?",
                          [this, id](bool confirmed) {
                              if (confirmed) {
                                  manager_->deleteTournament(id);
                                  refreshActiveScreenData();
                              }
                          });
    });

    // Team Screen
    teamNameInput_ = UITextInput(sf::Vector2f(250.f, 120.f), sf::Vector2f(260.f, 38.f), "Team Name (e.g. Red Dragons)");
    teamManagerInput_ = UITextInput(sf::Vector2f(525.f, 120.f), sf::Vector2f(240.f, 38.f), "Coach / Manager");
    teamContactInput_ = UITextInput(sf::Vector2f(780.f, 120.f), sf::Vector2f(260.f, 38.f), "Contact Email/Phone");
    addTeamBtn_ = UIButton(sf::Vector2f(1055.f, 120.f), sf::Vector2f(180.f, 38.f), "+ Register Team", UIButton::Variant::Primary);

    addTeamBtn_.setOnClick([this]() {
        auto val = manager_->addTeam(teamNameInput_.getText(), teamManagerInput_.getText(), teamContactInput_.getText());
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Registration Error", val.errorMessage);
        } else {
            teamNameInput_.clear();
            teamManagerInput_.clear();
            teamContactInput_.clear();
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Success", "Team registered successfully!");
        }
    });

    teamTable_ = UITable(sf::Vector2f(250.f, 180.f), sf::Vector2f(580.f, 490.f),
                        {"ID", "Team Name", "Manager", "Squad Size"},
                        {60.f, 220.f, 180.f, 90.f});

    teamSquadTable_ = UITable(sf::Vector2f(850.f, 180.f), sf::Vector2f(385.f, 490.f),
                             {"#", "Player Name", "Pos", "Goals"},
                             {50.f, 180.f, 85.f, 50.f});

    deleteTeamBtn_ = UIButton(sf::Vector2f(250.f, 690.f), sf::Vector2f(160.f, 40.f), "Delete Team", UIButton::Variant::Danger);
    deleteTeamBtn_.setOnClick([this]() {
        int id = teamTable_.getSelectedRowId();
        if (id <= 0) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Selection Required", "Select a team from the table to delete.");
            return;
        }
        modalDialog_.show(UIModalDialog::Type::Confirm, "Delete Team", "Delete team #" + std::to_string(id) + "?",
                          [this, id](bool confirmed) {
                              if (confirmed) {
                                  auto val = manager_->deleteTeam(id);
                                  if (!val.valid) {
                                      modalDialog_.show(UIModalDialog::Type::Error, "Cannot Delete", val.errorMessage);
                                  } else {
                                      refreshActiveScreenData();
                                  }
                              }
                          });
    });

    // Player Screen
    playerNameInput_ = UITextInput(sf::Vector2f(250.f, 120.f), sf::Vector2f(260.f, 38.f), "Player Full Name");
    playerJerseyInput_ = UITextInput(sf::Vector2f(525.f, 120.f), sf::Vector2f(120.f, 38.f), "Jersey (1-99)", true);
    playerSearchInput_ = UITextInput(sf::Vector2f(660.f, 120.f), sf::Vector2f(220.f, 38.f), "Search by name/team");
    addPlayerBtn_ = UIButton(sf::Vector2f(1060.f, 120.f), sf::Vector2f(175.f, 38.f), "+ Add Player", UIButton::Variant::Primary);

    addPlayerBtn_.setOnClick([this]() {
        int selectedTeamId = teamTable_.getSelectedRowId();
        if (selectedTeamId <= 0 && !manager_->getTeams().empty()) {
            selectedTeamId = manager_->getTeams().front().getId();
        }
        if (selectedTeamId <= 0) {
            modalDialog_.show(UIModalDialog::Type::Error, "No Team", "Please register at least one team first.");
            return;
        }

        int jersey = 0;
        try { jersey = std::stoi(playerJerseyInput_.getText()); } catch (...) {}

        PlayerPosition pos = PlayerPosition::Forward;
        if (selectedPlayerPosIndex_ == 0) pos = PlayerPosition::Goalkeeper;
        else if (selectedPlayerPosIndex_ == 1) pos = PlayerPosition::Defender;
        else if (selectedPlayerPosIndex_ == 2) pos = PlayerPosition::Midfielder;

        auto val = manager_->addPlayer(playerNameInput_.getText(), jersey, pos, selectedTeamId);
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Validation Error", val.errorMessage);
        } else {
            playerNameInput_.clear();
            playerJerseyInput_.clear();
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Success", "Player added to squad successfully!");
        }
    });

    playerTable_ = UITable(sf::Vector2f(250.f, 210.f), sf::Vector2f(985.f, 460.f),
                          {"ID", "Player Name", "Jersey", "Position", "Team Name", "Goals"},
                          {60.f, 250.f, 90.f, 140.f, 280.f, 80.f});

    deletePlayerBtn_ = UIButton(sf::Vector2f(250.f, 690.f), sf::Vector2f(160.f, 40.f), "Delete Player", UIButton::Variant::Danger);
    deletePlayerBtn_.setOnClick([this]() {
        int id = playerTable_.getSelectedRowId();
        if (id <= 0) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Selection Required", "Select a player to delete.");
            return;
        }
        modalDialog_.show(UIModalDialog::Type::Confirm, "Delete Player", "Delete player #" + std::to_string(id) + "?",
                          [this, id](bool confirmed) {
                              if (confirmed) {
                                  auto val = manager_->deletePlayer(id);
                                  if (!val.valid) {
                                      modalDialog_.show(UIModalDialog::Type::Error, "Cannot Delete", val.errorMessage);
                                  } else {
                                      refreshActiveScreenData();
                                  }
                              }
                          });
    });

    // Fixtures Screen
    generateFixturesBtn_ = UIButton(sf::Vector2f(250.f, 100.f), sf::Vector2f(200.f, 40.f), "Generate Fixtures", UIButton::Variant::Primary);
    generateFixturesBtn_.setOnClick([this]() {
        auto val = manager_->generateFixtures();
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Cannot Generate", val.errorMessage);
        } else {
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Fixtures Generated", "Round Robin schedule generated successfully!");
        }
    });

    resetFixturesBtn_ = UIButton(sf::Vector2f(465.f, 100.f), sf::Vector2f(180.f, 40.f), "Reset Fixtures", UIButton::Variant::Danger);
    resetFixturesBtn_.setOnClick([this]() {
        modalDialog_.show(UIModalDialog::Type::Confirm, "Reset Fixtures", "Are you sure you want to clear generated fixtures?",
                          [this](bool confirmed) {
                              if (confirmed) {
                                  auto val = manager_->resetFixtures();
                                  if (!val.valid) {
                                      modalDialog_.show(UIModalDialog::Type::Error, "Cannot Reset", val.errorMessage);
                                  } else {
                                      refreshActiveScreenData();
                                  }
                              }
                          });
    });

    fixtureTable_ = UITable(sf::Vector2f(250.f, 160.f), sf::Vector2f(985.f, 550.f),
                           {"Match #", "Round", "Home Team", "Away Team", "Status", "Score Result"},
                           {90.f, 100.f, 260.f, 260.f, 130.f, 120.f});

    // Score Entry Screen
    matchTable_ = UITable(sf::Vector2f(250.f, 120.f), sf::Vector2f(620.f, 590.f),
                         {"Match #", "Round", "Home Team vs Away Team", "Result", "Status"},
                         {75.f, 75.f, 280.f, 90.f, 85.f});

    homeScoreInput_ = UITextInput(sf::Vector2f(910.f, 200.f), sf::Vector2f(90.f, 42.f), "0", true);
    awayScoreInput_ = UITextInput(sf::Vector2f(1040.f, 200.f), sf::Vector2f(90.f, 42.f), "0", true);
    saveScoreBtn_ = UIButton(sf::Vector2f(910.f, 270.f), sf::Vector2f(220.f, 42.f), "Save Match Score", UIButton::Variant::Primary);

    saveScoreBtn_.setOnClick([this]() {
        if (!referee_ || !referee_->canRecordScores()) {
            modalDialog_.show(UIModalDialog::Type::Error, "Referee Login Required",
                              "Only a signed-in referee can enter or update match results.");
            return;
        }
        int mId = matchTable_.getSelectedRowId();
        if (mId <= 0) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Selection Required", "Please click and select a match from the table on the left.");
            return;
        }

        int hScore = 0, aScore = 0;
        try { hScore = std::stoi(homeScoreInput_.getText()); } catch (...) {}
        try { aScore = std::stoi(awayScoreInput_.getText()); } catch (...) {}

        auto val = manager_->recordMatchResult(mId, hScore, aScore, pendingGoalEvents_);
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Score Entry Error", val.errorMessage);
        } else {
            bool auditSaved = false;
            try {
                std::filesystem::create_directories("data");
                std::ofstream audit("data/referee_audit.log", std::ios::app);
                const auto active = manager_->getActiveTournament();
                const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                const std::tm* timestamp = std::localtime(&now);
                if (audit && referee_ && timestamp) {
                    audit << std::put_time(timestamp, "%Y-%m-%d %H:%M:%S")
                          << " | referee=" << referee_->userId()
                          << " | tournament=" << (active ? active->getId() : 0)
                          << " | match=" << mId << " | result=" << hScore << "-" << aScore << '\n';
                    auditSaved = static_cast<bool>(audit);
                }
            } catch (...) {
                auditSaved = false;
            }
            pendingGoalEvents_.clear();
            refreshActiveScreenData();
            const std::string message = auditSaved
                ? "Result saved. Referee " + referee_->userId() + " was added to the audit log."
                : "Result saved, but the referee audit log could not be written.";
            modalDialog_.show(UIModalDialog::Type::Alert, "Result Saved", message);
        }
    });

    // Standings Screen
    standingsTable_ = UITable(sf::Vector2f(250.f, 130.f), sf::Vector2f(985.f, 540.f),
                             {"Pos", "Team Name", "P", "W", "D", "L", "GF", "GA", "GD", "PTS"},
                             {60.f, 320.f, 65.f, 65.f, 65.f, 65.f, 75.f, 75.f, 85.f, 85.f});

    exportStandingsBtn_ = UIButton(sf::Vector2f(1055.f, 690.f), sf::Vector2f(180.f, 40.f), "Export Standings CSV", UIButton::Variant::Primary);
    exportStandingsBtn_.setOnClick([this]() {
        std::string exportPath = "data/standings_export.csv";
        if (manager_->exportStandingsCsv(exportPath)) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Export Successful", "Standings CSV exported to:\n" + exportPath);
        } else {
            modalDialog_.show(UIModalDialog::Type::Error, "Export Failed", "Could not export CSV file.");
        }
    });

    // Statistics Screen
    statsTable_ = UITable(sf::Vector2f(250.f, 140.f), sf::Vector2f(560.f, 540.f),
                         {"Rank", "Player Name", "Team Name", "Jersey", "Goals"},
                         {60.f, 210.f, 170.f, 60.f, 50.f});

    teamStatsTable_ = UITable(sf::Vector2f(830.f, 140.f), sf::Vector2f(405.f, 540.f),
                             {"Team Name", "Goals", "Top Scorer"},
                             {170.f, 65.f, 160.f});

    // Winner Screen
    completeTourneyBtn_ = UIButton(sf::Vector2f(450.f, 580.f), sf::Vector2f(250.f, 46.f), "Finalize & Declare Champion", UIButton::Variant::Primary);
    completeTourneyBtn_.setOnClick([this]() {
        auto val = manager_->completeActiveTournament();
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Cannot Finalize", val.errorMessage);
        } else {
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Tournament Completed", "Congratulations! Tournament champion officially recorded.");
        }
    });

    reopenTourneyBtn_ = UIButton(sf::Vector2f(720.f, 580.f), sf::Vector2f(200.f, 46.f), "Reopen Tournament", UIButton::Variant::Secondary);
    reopenTourneyBtn_.setOnClick([this]() {
        auto val = manager_->reopenActiveTournament();
        if (!val.valid) {
            modalDialog_.show(UIModalDialog::Type::Error, "Cannot Reopen", val.errorMessage);
        } else {
            refreshActiveScreenData();
            modalDialog_.show(UIModalDialog::Type::Alert, "Reopened", "Tournament state returned to Active.");
        }
    });

    // History Screen
    historyTable_ = UITable(sf::Vector2f(250.f, 130.f), sf::Vector2f(985.f, 540.f),
                           {"ID", "Tournament Name", "Season", "Status", "Champion Team"},
                           {60.f, 320.f, 140.f, 140.f, 300.f});

    exportFullReportBtn_ = UIButton(sf::Vector2f(1035.f, 690.f), sf::Vector2f(200.f, 40.f), "Export Full Text Report", UIButton::Variant::Primary);
    exportFullReportBtn_.setOnClick([this]() {
        std::string exportPath = "data/tournament_full_report.txt";
        if (manager_->exportTextReport(exportPath)) {
            modalDialog_.show(UIModalDialog::Type::Alert, "Report Generated", "Full tournament report exported to:\n" + exportPath);
        } else {
            modalDialog_.show(UIModalDialog::Type::Error, "Export Failed", "Could not export report.");
        }
    });
}

void MainWindow::refreshActiveScreenData() {
    auto activeOpt = manager_->getActiveTournament();

    // Refresh Tournaments Table
    tourneyTable_.clearRows();
    for (const auto& t : manager_->getTournamentSummaries()) {
        std::string champName = "Not Decided";
        if (t.getChampionTeamId() > 0) {
            const auto* team = manager_->findTeamById(t.getChampionTeamId());
            if (team) champName = team->getName();
            else champName = "Team #" + std::to_string(t.getChampionTeamId());
        }
        tourneyTable_.addRow({
            std::to_string(t.getId()),
            t.getName(),
            t.getSeasonYear(),
            t.getStatusString(),
            t.getFormatString(),
            champName
        }, t.getId());
    }

    // Refresh Teams & Squad
    teamTable_.clearRows();
    for (const auto& team : manager_->getTeams()) {
        teamTable_.addRow({
            std::to_string(team.getId()),
            team.getName(),
            team.getManager().empty() ? "-" : team.getManager(),
            std::to_string(team.getSquadSize())
        }, team.getId());
    }

    teamSquadTable_.clearRows();
    int selTeamId = teamTable_.getSelectedRowId();
    if (selTeamId > 0) {
        auto squad = manager_->getPlayersForTeam(selTeamId);
        for (const auto& p : squad) {
            teamSquadTable_.addRow({
                "#" + std::to_string(p.getJerseyNumber()),
                p.getName(),
                p.getPositionString(),
                std::to_string(p.getGoals())
            }, p.getId());
        }
    }

    // Refresh Players
    playerTable_.clearRows();
    for (const auto& p : manager_->getPlayers()) {
        const auto* t = manager_->findTeamById(p.getTeamId());
        std::string tName = t ? t->getName() : "Team #" + std::to_string(p.getTeamId());
        playerTable_.addRow({
            std::to_string(p.getId()),
            p.getName(),
            "#" + std::to_string(p.getJerseyNumber()),
            p.getPositionString(),
            tName,
            std::to_string(p.getGoals())
        }, p.getId());
    }

    // Refresh Fixtures
    fixtureTable_.clearRows();
    for (const auto& m : manager_->getMatches()) {
        const auto* home = manager_->findTeamById(m.getHomeTeamId());
        const auto* away = manager_->findTeamById(m.getAwayTeamId());
        std::string homeName = home ? home->getName() : "Team " + std::to_string(m.getHomeTeamId());
        std::string awayName = away ? away->getName() : "Team " + std::to_string(m.getAwayTeamId());

        std::string scoreStr = "-";
        if (m.isCompleted()) {
            scoreStr = std::to_string(m.getHomeScore()) + " - " + std::to_string(m.getAwayScore());
        }

        fixtureTable_.addRow({
            "#" + std::to_string(m.getId()),
            "Round " + std::to_string(m.getRound()),
            homeName,
            awayName,
            m.getStatusString(),
            scoreStr
        }, m.getId());
    }

    // Refresh Match Score Entry
    matchTable_.clearRows();
    for (const auto& m : manager_->getMatches()) {
        const auto* home = manager_->findTeamById(m.getHomeTeamId());
        const auto* away = manager_->findTeamById(m.getAwayTeamId());
        std::string homeName = home ? home->getName() : "Team " + std::to_string(m.getHomeTeamId());
        std::string awayName = away ? away->getName() : "Team " + std::to_string(m.getAwayTeamId());

        std::string scoreStr = m.isCompleted() ? (std::to_string(m.getHomeScore()) + " - " + std::to_string(m.getAwayScore())) : "vs";

        matchTable_.addRow({
            "#" + std::to_string(m.getId()),
            "R" + std::to_string(m.getRound()),
            homeName + " vs " + awayName,
            scoreStr,
            m.getStatusString()
        }, m.getId());
    }

    // Refresh Standings
    standingsTable_.clearRows();
    auto standings = manager_->getStandings();
    for (const auto& s : standings) {
        standingsTable_.addRow({
            std::to_string(s.position),
            s.teamName + (s.tieUnresolved ? " *" : ""),
            std::to_string(s.played),
            std::to_string(s.won),
            std::to_string(s.drawn),
            std::to_string(s.lost),
            std::to_string(s.goalsFor),
            std::to_string(s.goalsAgainst),
            (s.goalDifference > 0 ? "+" : "") + std::to_string(s.goalDifference),
            std::to_string(s.points)
        }, s.teamId);
    }

    // Refresh Statistics
    statsTable_.clearRows();
    auto leaderboard = manager_->getPlayerLeaderboard();
    for (const auto& entry : leaderboard) {
        statsTable_.addRow({
            std::to_string(entry.rank),
            entry.playerName,
            entry.teamName,
            "#" + std::to_string(entry.jerseyNumber),
            std::to_string(entry.goals)
        }, entry.playerId);
    }

    teamStatsTable_.clearRows();
    auto teamStats = manager_->getTeamStatistics();
    for (const auto& ts : teamStats) {
        std::string top = ts.topScorerName.empty() ? "-" : (ts.topScorerName + " (" + std::to_string(ts.topScorerGoals) + ")");
        teamStatsTable_.addRow({
            ts.teamName,
            std::to_string(ts.totalGoals),
            top
        }, ts.teamId);
    }

    // Refresh History Table
    historyTable_.clearRows();
    for (const auto& t : manager_->getTournamentSummaries()) {
        std::string champ = "In Progress";
        if (t.isCompleted() && t.getChampionTeamId() > 0) {
            const auto* team = manager_->findTeamById(t.getChampionTeamId());
            champ = team ? team->getName() : "Team #" + std::to_string(t.getChampionTeamId());
        }
        historyTable_.addRow({
            std::to_string(t.getId()),
            t.getName(),
            t.getSeasonYear(),
            t.getStatusString(),
            champ
        }, t.getId());
    }
}

void MainWindow::handleEvent(const sf::Event& event) {
    sf::Vector2f mousePos = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));

    // Modal dialog captures all events when active
    if (modalDialog_.isVisible()) {
        modalDialog_.handleEvent(event, mousePos);
        return;
    }

    // Navigation buttons
    for (auto& [scr, btn] : navButtons_) {
        btn.handleEvent(event, mousePos);
    }

    // Screen specific controls
    switch (currentScreen_) {
        case ScreenType::Dashboard:
            break;
        case ScreenType::TournamentList:
            tourneyNameInput_.handleEvent(event, mousePos);
            tourneySeasonInput_.handleEvent(event, mousePos);
            tourneyDescInput_.handleEvent(event, mousePos);
            createTourneyBtn_.handleEvent(event, mousePos);
            selectTourneyBtn_.handleEvent(event, mousePos);
            startTourneyBtn_.handleEvent(event, mousePos);
            deleteTourneyBtn_.handleEvent(event, mousePos);
            tourneyTable_.handleEvent(event, mousePos);
            break;
        case ScreenType::TeamManagement:
            teamNameInput_.handleEvent(event, mousePos);
            teamManagerInput_.handleEvent(event, mousePos);
            teamContactInput_.handleEvent(event, mousePos);
            addTeamBtn_.handleEvent(event, mousePos);
            deleteTeamBtn_.handleEvent(event, mousePos);
            if (teamTable_.handleEvent(event, mousePos)) {
                refreshActiveScreenData(); // Reload squad when team clicked
            }
            teamSquadTable_.handleEvent(event, mousePos);
            break;
        case ScreenType::PlayerManagement:
            playerNameInput_.handleEvent(event, mousePos);
            playerJerseyInput_.handleEvent(event, mousePos);
            playerSearchInput_.handleEvent(event, mousePos);
            addPlayerBtn_.handleEvent(event, mousePos);
            deletePlayerBtn_.handleEvent(event, mousePos);
            playerTable_.handleEvent(event, mousePos);

            // Handle position selector click (Forward, Midfielder, Defender, Goalkeeper)
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                for (int posIdx = 0; posIdx < 4; ++posIdx) {
                    sf::FloatRect posRect(895.f + static_cast<float>(posIdx) * 38.f, 120.f, 34.f, 38.f);
                    if (posRect.contains(mousePos)) {
                        selectedPlayerPosIndex_ = posIdx;
                        break;
                    }
                }
            }
            break;
        case ScreenType::Fixtures:
            generateFixturesBtn_.handleEvent(event, mousePos);
            resetFixturesBtn_.handleEvent(event, mousePos);
            fixtureTable_.handleEvent(event, mousePos);
            break;
        case ScreenType::MatchScoreEntry:
            if (matchTable_.handleEvent(event, mousePos)) {
                // Populate current score in inputs
                int mId = matchTable_.getSelectedRowId();
                const auto* m = manager_->findMatchById(mId);
                if (m && m->isCompleted()) {
                    homeScoreInput_.setText(std::to_string(m->getHomeScore()));
                    awayScoreInput_.setText(std::to_string(m->getAwayScore()));
                } else {
                    homeScoreInput_.setText("0");
                    awayScoreInput_.setText("0");
                }
            }
            homeScoreInput_.handleEvent(event, mousePos);
            awayScoreInput_.handleEvent(event, mousePos);
            saveScoreBtn_.handleEvent(event, mousePos);
            break;
        case ScreenType::Standings:
            standingsTable_.handleEvent(event, mousePos);
            exportStandingsBtn_.handleEvent(event, mousePos);
            break;
        case ScreenType::PlayerStats:
            statsTable_.handleEvent(event, mousePos);
            teamStatsTable_.handleEvent(event, mousePos);
            break;
        case ScreenType::WinnerSummary:
            completeTourneyBtn_.handleEvent(event, mousePos);
            reopenTourneyBtn_.handleEvent(event, mousePos);
            break;
        case ScreenType::HistoryReports:
            historyTable_.handleEvent(event, mousePos);
            exportFullReportBtn_.handleEvent(event, mousePos);
            break;
        case ScreenType::About:
            break;
    }
}

void MainWindow::update(const sf::Vector2f& mousePos) {
    if (modalDialog_.isVisible()) {
        modalDialog_.update(mousePos);
        return;
    }

    for (auto& [scr, btn] : navButtons_) {
        btn.update(mousePos);
    }

    createTourneyBtn_.update(mousePos);
    selectTourneyBtn_.update(mousePos);
    startTourneyBtn_.update(mousePos);
    deleteTourneyBtn_.update(mousePos);
    addTeamBtn_.update(mousePos);
    deleteTeamBtn_.update(mousePos);
    addPlayerBtn_.update(mousePos);
    deletePlayerBtn_.update(mousePos);
    generateFixturesBtn_.update(mousePos);
    resetFixturesBtn_.update(mousePos);
    saveScoreBtn_.update(mousePos);
    exportStandingsBtn_.update(mousePos);
    completeTourneyBtn_.update(mousePos);
    reopenTourneyBtn_.update(mousePos);
    exportFullReportBtn_.update(mousePos);

    tourneyNameInput_.update();
    tourneySeasonInput_.update();
    tourneyDescInput_.update();
    teamNameInput_.update();
    teamManagerInput_.update();
    teamContactInput_.update();
    playerNameInput_.update();
    playerJerseyInput_.update();
    playerSearchInput_.update();
    homeScoreInput_.update();
    awayScoreInput_.update();
}

void MainWindow::renderSidebar() {
    sf::RectangleShape sidebar(sf::Vector2f(220.f, 768.f));
    sidebar.setFillColor(UITheme::BgSidebar);
    window_.draw(sidebar);

    // Sidebar divider
    sf::RectangleShape div(sf::Vector2f(1.f, 768.f));
    div.setPosition(220.f, 0.f);
    div.setFillColor(UITheme::BorderColor);
    window_.draw(div);

    // App Branding
    sf::Text logoText("FTMS 2026", UITheme::boldFont, 22);
    logoText.setFillColor(UITheme::PrimaryGreen);
    logoText.setPosition(25.f, 25.f);
    window_.draw(logoText);

    sf::Text subText("Football Tournament Manager", UITheme::regularFont, 11);
    subText.setFillColor(UITheme::TextMuted);
    subText.setPosition(25.f, 55.f);
    window_.draw(subText);

    sf::Text refereeText("Signed in: " + (referee_ ? referee_->userId() : "Unknown"), UITheme::regularFont, 10);
    refereeText.setFillColor(UITheme::PrimaryGreen);
    refereeText.setPosition(25.f, 635.f);
    window_.draw(refereeText);

    // Active Tournament Status Card in Sidebar
    sf::RectangleShape tCard(sf::Vector2f(190.f, 80.f));
    tCard.setPosition(15.f, 665.f);
    tCard.setFillColor(UITheme::BgCard);
    tCard.setOutlineThickness(1.f);
    tCard.setOutlineColor(UITheme::BorderColor);
    window_.draw(tCard);

    auto activeOpt = manager_->getActiveTournament();
    sf::Text tLabel(activeOpt.has_value() ? activeOpt->getName() : "No Tournament Active", UITheme::boldFont, 12);
    tLabel.setFillColor(UITheme::TextWhite);
    tLabel.setPosition(25.f, 675.f);
    window_.draw(tLabel);

    std::string st = activeOpt.has_value() ? ("Status: " + activeOpt->getStatusString()) : "Draft a tournament";
    sf::Text tStatus(st, UITheme::regularFont, 11);
    tStatus.setFillColor(activeOpt.has_value() && activeOpt->isActive() ? UITheme::PrimaryGreen : UITheme::AccentGold);
    tStatus.setPosition(25.f, 695.f);
    window_.draw(tStatus);

    std::string yr = activeOpt.has_value() ? ("Season: " + activeOpt->getSeasonYear()) : "College PBL 2026";
    sf::Text tYear(yr, UITheme::regularFont, 10);
    tYear.setFillColor(UITheme::TextMuted);
    tYear.setPosition(25.f, 715.f);
    window_.draw(tYear);

    // Render navigation buttons with active highlight
    for (const auto& [scr, btn] : navButtons_) {
        if (scr == currentScreen_) {
            // Draw active green accent bar
            sf::RectangleShape activeIndicator(sf::Vector2f(4.f, 36.f));
            activeIndicator.setPosition(15.f, btn.contains(sf::Vector2f(20.f, 0.f)) ? 0.f : 0.f); // simplified
            activeIndicator.setFillColor(UITheme::PrimaryGreen);
        }
        btn.draw(window_);
    }
}

void MainWindow::renderDashboard() {
    auto stats = manager_->getDashboardStats();
    auto activeOpt = manager_->getActiveTournament();

    // Screen Title
    sf::Text title("Dashboard & Overview", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    std::string subtitleStr = activeOpt.has_value()
        ? (activeOpt->getName() + " (" + activeOpt->getSeasonYear() + ") - " + activeOpt->getStatusString() + " State")
        : "No active tournament loaded. Create or select a tournament to get started.";
    sf::Text subtitle(subtitleStr, UITheme::regularFont, 14);
    subtitle.setFillColor(UITheme::TextMuted);
    subtitle.setPosition(250.f, 60.f);
    window_.draw(subtitle);

    // KPI Summary Cards: Teams, Players, Completed Matches, Pending Matches
    struct KpiCard {
        std::string label;
        std::string value;
        sf::Color accent;
    };
    std::vector<KpiCard> kpis = {
        {"Registered Teams", std::to_string(stats.totalTeams), UITheme::PrimaryGreen},
        {"Registered Players", std::to_string(stats.totalPlayers), UITheme::PrimaryGreen},
        {"Matches Played", std::to_string(stats.completedMatches) + " / " + std::to_string(stats.totalMatches), UITheme::AccentGold},
        {"Pending Matches", std::to_string(stats.pendingMatches), UITheme::TextWhite}
    };

    const float cardW = 230.f;
    const float cardH = 95.f;
    for (std::size_t i = 0; i < kpis.size(); ++i) {
        float cx = 250.f + static_cast<float>(i) * (cardW + 20.f);
        sf::RectangleShape card(sf::Vector2f(cardW, cardH));
        card.setPosition(cx, 105.f);
        card.setFillColor(UITheme::BgCard);
        card.setOutlineThickness(1.f);
        card.setOutlineColor(UITheme::BorderColor);
        window_.draw(card);

        sf::Text valText(kpis[i].value, UITheme::boldFont, 24);
        valText.setFillColor(kpis[i].accent);
        valText.setPosition(cx + 20.f, 120.f);
        window_.draw(valText);

        sf::Text lblText(kpis[i].label, UITheme::regularFont, 13);
        lblText.setFillColor(UITheme::TextMuted);
        lblText.setPosition(cx + 20.f, 155.f);
        window_.draw(lblText);
    }

    // Leaderboards / Overview Preview Panels
    // Top Team Card
    sf::RectangleShape teamLeaderCard(sf::Vector2f(480.f, 120.f));
    teamLeaderCard.setPosition(250.f, 225.f);
    teamLeaderCard.setFillColor(UITheme::BgCard);
    teamLeaderCard.setOutlineThickness(1.f);
    teamLeaderCard.setOutlineColor(UITheme::BorderColor);
    window_.draw(teamLeaderCard);

    sf::Text tlHead("Current Table Leader", UITheme::boldFont, 16);
    tlHead.setFillColor(UITheme::PrimaryGreen);
    tlHead.setPosition(270.f, 240.f);
    window_.draw(tlHead);

    sf::Text tlVal(stats.leadingTeam, UITheme::boldFont, 20);
    tlVal.setFillColor(UITheme::TextWhite);
    tlVal.setPosition(270.f, 275.f);
    window_.draw(tlVal);

    // Top Scorer Card
    sf::RectangleShape scorerCard(sf::Vector2f(480.f, 120.f));
    scorerCard.setPosition(755.f, 225.f);
    scorerCard.setFillColor(UITheme::BgCard);
    scorerCard.setOutlineThickness(1.f);
    scorerCard.setOutlineColor(UITheme::BorderColor);
    window_.draw(scorerCard);

    sf::Text tsHead("Leading Goalscorer (Golden Boot)", UITheme::boldFont, 16);
    tsHead.setFillColor(UITheme::AccentGold);
    tsHead.setPosition(775.f, 240.f);
    window_.draw(tsHead);

    std::string tsValStr = (stats.topScorerGoals > 0)
        ? (stats.topScorer + " - " + std::to_string(stats.topScorerGoals) + " Goals")
        : "No goals scored yet";
    sf::Text tsVal(tsValStr, UITheme::boldFont, 20);
    tsVal.setFillColor(UITheme::TextWhite);
    tsVal.setPosition(775.f, 275.f);
    window_.draw(tsVal);

    // Upcoming Fixtures & Recent Results side-by-side
    sf::Text upHead("Upcoming Fixtures", UITheme::boldFont, 16);
    upHead.setFillColor(UITheme::TextWhite);
    upHead.setPosition(250.f, 370.f);
    window_.draw(upHead);

    float matchY = 405.f;
    if (stats.upcomingMatches.empty()) {
        sf::Text emptyMsg("No pending fixtures scheduled.", UITheme::regularFont, 13);
        emptyMsg.setFillColor(UITheme::TextMuted);
        emptyMsg.setPosition(250.f, matchY);
        window_.draw(emptyMsg);
    } else {
        for (const auto& m : stats.upcomingMatches) {
            const auto* h = manager_->findTeamById(m.getHomeTeamId());
            const auto* a = manager_->findTeamById(m.getAwayTeamId());
            std::string hName = h ? h->getName() : "Team " + std::to_string(m.getHomeTeamId());
            std::string aName = a ? a->getName() : "Team " + std::to_string(m.getAwayTeamId());

            sf::RectangleShape mBox(sf::Vector2f(480.f, 44.f));
            mBox.setPosition(250.f, matchY);
            mBox.setFillColor(UITheme::BgCard);
            mBox.setOutlineThickness(1.f);
            mBox.setOutlineColor(UITheme::BorderColor);
            window_.draw(mBox);

            sf::Text mText("Match #" + std::to_string(m.getId()) + " (" + m.getMatchDate() + "): " + hName + " vs " + aName, UITheme::regularFont, 13);
            mText.setFillColor(UITheme::TextWhite);
            mText.setPosition(265.f, matchY + 12.f);
            window_.draw(mText);
            matchY += 52.f;
        }
    }

    // Recent Results
    sf::Text resHead("Recent Match Results", UITheme::boldFont, 16);
    resHead.setFillColor(UITheme::TextWhite);
    resHead.setPosition(755.f, 370.f);
    window_.draw(resHead);

    matchY = 405.f;
    if (stats.recentResults.empty()) {
        sf::Text emptyRes("No completed match results yet.", UITheme::regularFont, 13);
        emptyRes.setFillColor(UITheme::TextMuted);
        emptyRes.setPosition(755.f, matchY);
        window_.draw(emptyRes);
    } else {
        for (const auto& m : stats.recentResults) {
            const auto* h = manager_->findTeamById(m.getHomeTeamId());
            const auto* a = manager_->findTeamById(m.getAwayTeamId());
            std::string hName = h ? h->getName() : "Team " + std::to_string(m.getHomeTeamId());
            std::string aName = a ? a->getName() : "Team " + std::to_string(m.getAwayTeamId());

            sf::RectangleShape mBox(sf::Vector2f(480.f, 44.f));
            mBox.setPosition(755.f, matchY);
            mBox.setFillColor(UITheme::BgCard);
            mBox.setOutlineThickness(1.f);
            mBox.setOutlineColor(UITheme::BorderColor);
            window_.draw(mBox);

            sf::Text mText(hName + " " + std::to_string(m.getHomeScore()) + " - " + std::to_string(m.getAwayScore()) + " " + aName, UITheme::boldFont, 13);
            mText.setFillColor(UITheme::PrimaryGreen);
            mText.setPosition(770.f, matchY + 12.f);
            window_.draw(mText);
            matchY += 52.f;
        }
    }
}

void MainWindow::renderTournamentList() {
    sf::Text title("Tournament Management", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Create new tournaments, switch active competition, or start/finalize tournament cycles.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    // Form inputs
    tourneyNameInput_.draw(window_);
    tourneySeasonInput_.draw(window_);
    tourneyDescInput_.draw(window_);
    createTourneyBtn_.draw(window_);

    // Tournaments table
    tourneyTable_.draw(window_);

    // Action buttons
    selectTourneyBtn_.draw(window_);
    startTourneyBtn_.draw(window_);
    deleteTourneyBtn_.draw(window_);
}

void MainWindow::renderTeamManagement() {
    sf::Text title("Team & Squad Management", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Register participating college teams and view registered player squads.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    teamNameInput_.draw(window_);
    teamManagerInput_.draw(window_);
    teamContactInput_.draw(window_);
    addTeamBtn_.draw(window_);

    teamTable_.draw(window_);
    teamSquadTable_.draw(window_);
    deleteTeamBtn_.draw(window_);
}

void MainWindow::renderPlayerManagement() {
    sf::Text title("Player Registration & Management", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Register squad members, assign positions (GK, DEF, MID, FWD), and enforce jersey number rules.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    playerNameInput_.draw(window_);
    playerJerseyInput_.draw(window_);
    playerSearchInput_.draw(window_);

    // Position Selector Pills: [GK] [DEF] [MID] [FWD]
    std::vector<std::string> posLabels = {"GK", "DEF", "MID", "FWD"};
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape posPill(sf::Vector2f(34.f, 38.f));
        posPill.setPosition(895.f + static_cast<float>(i) * 38.f, 120.f);
        if (selectedPlayerPosIndex_ == i) {
            posPill.setFillColor(UITheme::PrimaryGreen);
            posPill.setOutlineThickness(1.f);
            posPill.setOutlineColor(UITheme::PrimaryGreen);
        } else {
            posPill.setFillColor(UITheme::BgCard);
            posPill.setOutlineThickness(1.f);
            posPill.setOutlineColor(UITheme::BorderColor);
        }
        window_.draw(posPill);

        sf::Text posText(posLabels[i], UITheme::boldFont, 11);
        posText.setFillColor(selectedPlayerPosIndex_ == i ? UITheme::BgDark : UITheme::TextWhite);
        posText.setPosition(900.f + static_cast<float>(i) * 38.f, 130.f);
        window_.draw(posText);
    }

    addPlayerBtn_.draw(window_);
    playerTable_.draw(window_);
    deletePlayerBtn_.draw(window_);
}

void MainWindow::renderFixtures() {
    sf::Text title("Automatic Fixture Schedule", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Single Round Robin circle-method generator with balanced home/away pairings and odd-team BYEs.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    generateFixturesBtn_.draw(window_);
    resetFixturesBtn_.draw(window_);
    fixtureTable_.draw(window_);
}

void MainWindow::renderMatchScoreEntry() {
    sf::Text title("Match Management & Score Entry", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Click a scheduled match on the left, enter official final scores, and update standings automatically.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    matchTable_.draw(window_);

    // Right Score Box Panel
    sf::RectangleShape scoreCard(sf::Vector2f(340.f, 400.f));
    scoreCard.setPosition(890.f, 120.f);
    scoreCard.setFillColor(UITheme::BgCard);
    scoreCard.setOutlineThickness(1.f);
    scoreCard.setOutlineColor(UITheme::BorderColor);
    window_.draw(scoreCard);

    int mId = matchTable_.getSelectedRowId();
    std::string mHead = (mId > 0) ? ("Selected Match #" + std::to_string(mId)) : "Select Match From Left";
    sf::Text mTitle(mHead, UITheme::boldFont, 18);
    mTitle.setFillColor(UITheme::PrimaryGreen);
    mTitle.setPosition(910.f, 145.f);
    window_.draw(mTitle);

    sf::Text vsLabel("Home Goals         vs         Away Goals", UITheme::regularFont, 13);
    vsLabel.setFillColor(UITheme::TextMuted);
    vsLabel.setPosition(910.f, 178.f);
    window_.draw(vsLabel);

    homeScoreInput_.draw(window_);
    awayScoreInput_.draw(window_);
    saveScoreBtn_.draw(window_);

    sf::Text noteText("Note: Editing a completed match recalculates\nall points, goal differences, and head-to-head\nrecords instantly.", UITheme::regularFont, 12);
    noteText.setFillColor(UITheme::TextMuted);
    noteText.setPosition(910.f, 335.f);
    window_.draw(noteText);
}

void MainWindow::renderStandings() {
    sf::Text title("Official Points Table", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Standings calculated automatically from match results (Win = 3 pts, Draw = 1 pt, Loss = 0 pts).", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    standingsTable_.draw(window_);
    exportStandingsBtn_.draw(window_);

    sf::Text tiePolicy("Tie-Breaking Hierarchy: 1. Total Points  ->  2. Goal Difference (GD)  ->  3. Goals Scored (GF)  ->  4. Head-to-Head points", UITheme::regularFont, 12);
    tiePolicy.setFillColor(UITheme::AccentGold);
    tiePolicy.setPosition(250.f, 700.f);
    window_.draw(tiePolicy);
}

void MainWindow::renderPlayerStats() {
    sf::Text title("Player Statistics & Golden Boot", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Top scorer rankings, individual goal tallies, and team attack performance.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    statsTable_.draw(window_);
    teamStatsTable_.draw(window_);
}

void MainWindow::renderWinnerSummary() {
    sf::Text title("Tournament Winner & Completion", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    auto activeOpt = manager_->getActiveTournament();
    bool isCompleted = activeOpt.has_value() && activeOpt->isCompleted();

    sf::RectangleShape trophyCard(sf::Vector2f(985.f, 480.f));
    trophyCard.setPosition(250.f, 80.f);
    trophyCard.setFillColor(UITheme::BgCard);
    trophyCard.setOutlineThickness(1.f);
    trophyCard.setOutlineColor(isCompleted ? UITheme::AccentGold : UITheme::BorderColor);
    window_.draw(trophyCard);

    if (isCompleted) {
        sf::Text champHead("TOURNAMENT CHAMPION", UITheme::boldFont, 28);
        champHead.setFillColor(UITheme::AccentGold);
        champHead.setPosition(520.f, 130.f);
        window_.draw(champHead);

        const auto* team = manager_->findTeamById(activeOpt->getChampionTeamId());
        std::string tName = team ? team->getName() : "Team #" + std::to_string(activeOpt->getChampionTeamId());
        sf::Text champTeam(tName, UITheme::boldFont, 36);
        champTeam.setFillColor(UITheme::TextWhite);
        champTeam.setPosition(550.f, 180.f);
        window_.draw(champTeam);

        // Top Scorer Award
        sf::Text gbHead("GOLDEN BOOT WINNER(S)", UITheme::boldFont, 20);
        gbHead.setFillColor(UITheme::PrimaryGreen);
        gbHead.setPosition(550.f, 270.f);
        window_.draw(gbHead);

        std::string tsNames;
        for (int pId : activeOpt->getTopScorerPlayerIds()) {
            const auto* p = manager_->findPlayerById(pId);
            if (p) {
                if (!tsNames.empty()) tsNames += ", ";
                tsNames += p->getName() + " (" + std::to_string(p->getGoals()) + " goals)";
            }
        }
        if (tsNames.empty()) tsNames = "No goals recorded";

        sf::Text gbPlayer(tsNames, UITheme::boldFont, 22);
        gbPlayer.setFillColor(UITheme::TextWhite);
        gbPlayer.setPosition(520.f, 310.f);
        window_.draw(gbPlayer);

        sf::Text certNote("Status: Officially Completed and Locked against further modifications.", UITheme::regularFont, 14);
        certNote.setFillColor(UITheme::TextMuted);
        certNote.setPosition(470.f, 390.f);
        window_.draw(certNote);

        reopenTourneyBtn_.draw(window_);
    } else {
        sf::Text compHead("Tournament Status: In Progress", UITheme::boldFont, 24);
        compHead.setFillColor(UITheme::PrimaryGreen);
        compHead.setPosition(550.f, 180.f);
        window_.draw(compHead);

        sf::Text compInfo("To finalize the tournament and declare the champion:\n"
                          "1. All scheduled matches must be completed.\n"
                          "2. Scores must be verified.\n"
                          "3. No unresolved 1st-place ties can remain.", UITheme::regularFont, 16);
        compInfo.setFillColor(UITheme::TextWhite);
        compInfo.setPosition(490.f, 240.f);
        window_.draw(compInfo);

        completeTourneyBtn_.draw(window_);
    }
}

void MainWindow::renderHistoryReports() {
    sf::Text title("Tournament History & Reports", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::Text sub("Historical tournament archives, past winners, and exportable academic documentation.", UITheme::regularFont, 14);
    sub.setFillColor(UITheme::TextMuted);
    sub.setPosition(250.f, 60.f);
    window_.draw(sub);

    historyTable_.draw(window_);
    exportFullReportBtn_.draw(window_);
}

void MainWindow::renderAbout() {
    sf::Text title("About Football Tournament Management System", UITheme::boldFont, 24);
    title.setFillColor(UITheme::TextWhite);
    title.setPosition(250.f, 25.f);
    window_.draw(title);

    sf::RectangleShape aboutCard(sf::Vector2f(985.f, 600.f));
    aboutCard.setPosition(250.f, 80.f);
    aboutCard.setFillColor(UITheme::BgCard);
    aboutCard.setOutlineThickness(1.f);
    aboutCard.setOutlineColor(UITheme::BorderColor);
    window_.draw(aboutCard);

    sf::Text abHead("College Project-Based Learning (PBL) Submission", UITheme::boldFont, 20);
    abHead.setFillColor(UITheme::PrimaryGreen);
    abHead.setPosition(280.f, 110.f);
    window_.draw(abHead);

    std::string textBlock =
        "Course: Object-Oriented Programming (OOP) in C++20\n"
        "GUI Framework: SFML 2.6.2 (Static Build)\n"
        "Build System: CMake & Ninja Toolchain\n"
        "Compiler: MinGW-w64 GCC 16.2.0 (UCRT64)\n\n"
        "Team Role Division (4 Members):\n"
        " - Member 1 (Domain Models): Player, Team, Match, Tournament classes, encapsulation & validations.\n"
        " - Member 2 (Algorithms): Circle-method Round Robin schedule, Standings & Head-to-Head tie breaker.\n"
        " - Member 3 (SFML GUI): Desktop GUI architecture, dashboard KPI cards, navigation, reusable tables & dialogs.\n"
        " - Member 4 (Persistence & Reports): FileRepository, CSV and text reporting, integration testing.\n\n"
        "System Features:\n"
        " [x] Automated single Round-Robin fixture generation with odd-team BYE handling\n"
        " [x] Dynamic standings recalculation from match truth\n"
        " [x] Top scorer golden boot tracking and squad management\n"
        " [x] Versioned file persistence and safety against data loss\n"
        " [x] Comprehensive automated unit and integration tests\n";

    sf::Text details(textBlock, UITheme::regularFont, 14);
    details.setFillColor(UITheme::TextWhite);
    details.setPosition(280.f, 160.f);
    window_.draw(details);
}

void MainWindow::render() {
    window_.clear(UITheme::BgDark);

    renderSidebar();

    switch (currentScreen_) {
        case ScreenType::Dashboard:        renderDashboard(); break;
        case ScreenType::TournamentList:   renderTournamentList(); break;
        case ScreenType::TeamManagement:   renderTeamManagement(); break;
        case ScreenType::PlayerManagement: renderPlayerManagement(); break;
        case ScreenType::Fixtures:         renderFixtures(); break;
        case ScreenType::MatchScoreEntry:  renderMatchScoreEntry(); break;
        case ScreenType::Standings:        renderStandings(); break;
        case ScreenType::PlayerStats:      renderPlayerStats(); break;
        case ScreenType::WinnerSummary:    renderWinnerSummary(); break;
        case ScreenType::HistoryReports:   renderHistoryReports(); break;
        case ScreenType::About:            renderAbout(); break;
    }

    modalDialog_.draw(window_);
    window_.display();
}

void MainWindow::run() {
    while (window_.isOpen()) {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
            }
            handleEvent(event);
        }

        sf::Vector2f mousePos = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
        update(mousePos);
        render();
    }
}

} // namespace ftms
