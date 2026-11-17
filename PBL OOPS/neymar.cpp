#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <vector>

// FTMS manager prototype built for the PBL OOPS project.
// Features: team and player registration, round-robin fixtures, SFML dashboard/pages,
// score entry, results and standings. Storage, player squads and knockout stage
// are deliberately reserved for later milestones. Team registration is available
// before scores are entered; fixture generation adjusts automatically.

struct Team {
    std::string name;
    std::string manager;
    std::string contact;
    int played = 0, wins = 0, draws = 0, losses = 0;
    int goalsFor = 0, goalsAgainst = 0, points = 0;
    int goalDifference() const { return goalsFor - goalsAgainst; }
};

struct Player {
    std::string name;
    int jersey = 0;
    std::string position = "Forward";
    std::size_t team = 0;
};

struct Match {
    std::size_t home = 0, away = 0;
    bool played = false;
    int homeGoals = 0, awayGoals = 0;
};

class Tournament {
public:
    Tournament() = default;

    const std::vector<Team>& getTeams() const { return teams; }
    const std::vector<Match>& getMatches() const { return matches; }
    const std::vector<Player>& getPlayers() const { return players; }
    int playedCount() const {
        return static_cast<int>(std::count_if(matches.begin(), matches.end(),
            [](const Match& match) { return match.played; }));
    }
    bool record(std::size_t index, int homeGoals, int awayGoals) {
        if (index >= matches.size() || matches[index].played || homeGoals < 0 || awayGoals < 0)
            return false;
        Match& match = matches[index];
        match.played = true; match.homeGoals = homeGoals; match.awayGoals = awayGoals;
        Team& home = teams[match.home];
        Team& away = teams[match.away];
        ++home.played; ++away.played;
        home.goalsFor += homeGoals; home.goalsAgainst += awayGoals;
        away.goalsFor += awayGoals; away.goalsAgainst += homeGoals;
        if (homeGoals > awayGoals) { ++home.wins; ++away.losses; home.points += 3; }
        else if (awayGoals > homeGoals) { ++away.wins; ++home.losses; away.points += 3; }
        else { ++home.draws; ++away.draws; ++home.points; ++away.points; }
        return true;
    }
    bool addTeam(const std::string& name, const std::string& manager, const std::string& contact) {
        if (name.size() < 2 || playedCount() != 0) return false;
        for (const auto& team : teams)
            if (lower(team.name) == lower(name)) return false;
        Team team;
        team.name = name;
        team.manager = manager;
        team.contact = contact;
        teams.push_back(team);
        rebuildFixtures();
        return true;
    }
    bool addPlayer(const std::string& name, int jersey, const std::string& position, std::size_t teamIndex) {
        if (name.size() < 2 || jersey < 1 || jersey > 99 || teamIndex >= teams.size()) return false;
        for (const auto& p : players)
            if (p.team == teamIndex && p.jersey == jersey) return false;
        players.push_back(Player{name, jersey, position, teamIndex});
        return true;
    }
    std::vector<std::size_t> standings() const {
        std::vector<std::size_t> order(teams.size());
        for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::stable_sort(order.begin(), order.end(), [this](std::size_t a, std::size_t b) {
            if (teams[a].points != teams[b].points) return teams[a].points > teams[b].points;
            if (teams[a].goalDifference() != teams[b].goalDifference())
                return teams[a].goalDifference() > teams[b].goalDifference();
            return teams[a].goalsFor > teams[b].goalsFor;
        });
        return order;
    }
private:
    static std::string lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }
    void rebuildFixtures() {
        matches.clear();
        for (std::size_t home = 0; home < teams.size(); ++home)
            for (std::size_t away = home + 1; away < teams.size(); ++away)
                matches.push_back(Match{home, away});
    }
    std::vector<Team> teams;
    std::vector<Player> players;
    std::vector<Match> matches;
};

class ManagerWindow {
public:
    ManagerWindow() {
        sf::ContextSettings settings;
        settings.antialiasingLevel = 4;
        window.create(sf::VideoMode(1280, 768),
            "Football Tournament Management System (FTMS)", sf::Style::Titlebar | sf::Style::Close, settings);
        window.setFramerateLimit(60);
        fontLoaded = font.loadFromFile("assets/fonts/font.ttf");
        if (!fontLoaded) fontLoaded = font.loadFromFile("C:/Windows/Fonts/segoeui.ttf");
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event{};
            while (window.pollEvent(event)) handle(event);
            draw();
        }
    }

private:
    sf::RenderWindow window;
    sf::Font font;
    bool fontLoaded = false;
    Tournament tournament;
    int page = 0; // Dashboard, Teams, Players, Fixtures, Standings
    int selectedFixture = 0;
    bool enteringScore = false, scoreAway = false;
    std::string homeInput, awayInput;
    std::string teamNameInput, managerInput, contactInput;
    std::string playerNameInput, jerseyInput;
    int focusedField = 0;
    int selectedPosition = 3;
    std::size_t selectedTeam = 0;
    std::string notice = "League season is ready";

    const sf::Color bg{18, 24, 22};
    const sf::Color sidebarColor{13, 19, 17};
    const sf::Color card{26, 36, 33};
    const sf::Color border{46, 62, 56};
    const sf::Color green{16, 185, 129};
    const sf::Color gold{245, 158, 11};
    const sf::Color white{248, 250, 252};
    const sf::Color muted{148, 163, 184};

    void handle(const sf::Event& event) {
        if (event.type == sf::Event::Closed) window.close();
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2f mouse(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
            if (!enteringScore && mouse.x < 220.f && mouse.y >= 112.f && mouse.y < 367.f) {
                page = std::max(0, std::min(4, static_cast<int>((mouse.y - 112.f) / 51.f)));
                focusedField = 0;
            } else if (!enteringScore && page == 1 && mouse.y >= 112.f && mouse.y < 165.f) {
                if (mouse.x >= 250.f && mouse.x < 520.f) focusedField = 0;
                else if (mouse.x >= 525.f && mouse.x < 770.f) focusedField = 1;
                else if (mouse.x >= 780.f && mouse.x < 1045.f) focusedField = 2;
                else if (mouse.x >= 1050.f && mouse.x < 1250.f) registerTeam();
            } else if (!enteringScore && page == 2 && mouse.y >= 112.f && mouse.y < 165.f) {
                if (mouse.x >= 250.f && mouse.x < 510.f) focusedField = 0;
                else if (mouse.x >= 525.f && mouse.x < 640.f) focusedField = 1;
                else if (mouse.x >= 660.f && mouse.x < 885.f && !tournament.getTeams().empty()) {
                    selectedTeam = (selectedTeam + 1) % tournament.getTeams().size();
                } else if (mouse.x >= 895.f && mouse.x < 1050.f) {
                    selectedPosition = std::min(3, static_cast<int>((mouse.x - 895.f) / 39.f));
                } else if (mouse.x >= 1060.f && mouse.x < 1250.f) registerPlayer();
            } else if (!enteringScore && page == 3 && mouse.x >= 248.f && mouse.y >= 210.f && mouse.y < 640.f) {
                const int first = visibleFixtureStart();
                const int row = static_cast<int>((mouse.y - 220.f) / 29.f);
                if (row >= 0 && row < 14 && !tournament.getMatches().empty())
                    selectedFixture = std::min(static_cast<int>(tournament.getMatches().size()) - 1, first + row);
            }
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                if (enteringScore) enteringScore = false;
                else window.close();
            }
            if (enteringScore && event.key.code == sf::Keyboard::Tab) scoreAway = !scoreAway;
            else if (!enteringScore && event.key.code == sf::Keyboard::Tab) page = (page + 1) % 5;
            if (!enteringScore && page == 1 && event.key.code == sf::Keyboard::BackSpace) backspaceTeamField();
            if (!enteringScore && page == 1 && event.key.code == sf::Keyboard::Enter) registerTeam();
            if (!enteringScore && page == 2 && event.key.code == sf::Keyboard::BackSpace) backspacePlayerField();
            if (!enteringScore && page == 2 && event.key.code == sf::Keyboard::Enter) registerPlayer();
            if (!enteringScore && page == 3 && event.key.code == sf::Keyboard::Up) selectedFixture = std::max(0, selectedFixture - 1);
            if (!enteringScore && page == 3 && event.key.code == sf::Keyboard::Down && !tournament.getMatches().empty()) selectedFixture = std::min(static_cast<int>(tournament.getMatches().size()) - 1, selectedFixture + 1);
            if (!enteringScore && page == 3 && event.key.code == sf::Keyboard::F2) openScoreEntry();
            if (enteringScore && event.key.code == sf::Keyboard::BackSpace) {
                std::string& value = scoreAway ? awayInput : homeInput;
                if (!value.empty()) value.pop_back();
            }
            if (enteringScore && event.key.code == sf::Keyboard::Enter) saveScore();
        }
        if (enteringScore && event.type == sf::Event::TextEntered && event.text.unicode >= '0' && event.text.unicode <= '9') {
            std::string& value = scoreAway ? awayInput : homeInput;
            if (value.size() < 2) value += static_cast<char>(event.text.unicode);
        }
        if (!enteringScore && event.type == sf::Event::TextEntered && event.text.unicode >= 32 && event.text.unicode < 127) {
            const char ch = static_cast<char>(event.text.unicode);
            if (page == 1) appendTeamField(ch);
            if (page == 2) appendPlayerField(ch);
        }
    }

    void appendTeamField(char ch) {
        if (focusedField == 0 && teamNameInput.size() < 30) teamNameInput += ch;
        else if (focusedField == 1 && managerInput.size() < 30) managerInput += ch;
        else if (focusedField == 2 && contactInput.size() < 38) contactInput += ch;
    }
    void appendPlayerField(char ch) {
        if (focusedField == 0 && playerNameInput.size() < 30) playerNameInput += ch;
        else if (focusedField == 1 && std::isdigit(static_cast<unsigned char>(ch)) && jerseyInput.size() < 2) jerseyInput += ch;
    }
    void backspaceTeamField() {
        std::string* fields[] = {&teamNameInput, &managerInput, &contactInput};
        auto& value = *fields[std::clamp(focusedField, 0, 2)];
        if (!value.empty()) value.pop_back();
    }
    void backspacePlayerField() {
        std::string* fields[] = {&playerNameInput, &jerseyInput};
        auto& value = *fields[std::clamp(focusedField, 0, 1)];
        if (!value.empty()) value.pop_back();
    }

    int visibleFixtureStart() const { return std::max(0, std::min(selectedFixture - 6, static_cast<int>(tournament.getMatches().size()) - 14)); }
    void registerTeam() {
        if (teamNameInput.size() < 2) { notice = "Enter a team name with at least 2 characters"; return; }
        if (tournament.playedCount() != 0) { notice = "Team registration closes after the first score is recorded"; return; }
        if (tournament.addTeam(teamNameInput, managerInput, contactInput)) {
            notice = teamNameInput + " registered; fixtures regenerated";
            teamNameInput.clear(); managerInput.clear(); contactInput.clear();
            selectedTeam = tournament.getTeams().size() - 1;
        } else notice = "That team name is already registered; choose another name";
    }
    void registerPlayer() {
        if (tournament.getTeams().empty()) { notice = "Register a team before adding players"; return; }
        int jersey = 0;
        try { jersey = std::stoi(jerseyInput); } catch (...) {}
        const std::array<std::string, 4> positions = {"Goalkeeper", "Defender", "Midfielder", "Forward"};
        if (tournament.addPlayer(playerNameInput, jersey, positions[static_cast<std::size_t>(selectedPosition)], selectedTeam)) {
            notice = playerNameInput + " added to " + tournament.getTeams()[selectedTeam].name;
            playerNameInput.clear(); jerseyInput.clear();
        } else if (playerNameInput.size() < 2) notice = "Enter the player's name";
        else if (jersey < 1 || jersey > 99) notice = "Enter a jersey number from 1 to 99";
        else notice = "That jersey number is already used by this team";
    }
    void openScoreEntry() {
        if (selectedFixture < 0 || static_cast<std::size_t>(selectedFixture) >= tournament.getMatches().size()) return;
        if (tournament.getMatches()[static_cast<std::size_t>(selectedFixture)].played) {
            notice = "This match already has a result";
            return;
        }
        enteringScore = true; scoreAway = false; homeInput.clear(); awayInput.clear();
    }
    void saveScore() {
        if (homeInput.empty() || awayInput.empty()) { notice = "Enter both scores before saving"; return; }
        if (tournament.record(static_cast<std::size_t>(selectedFixture), std::stoi(homeInput), std::stoi(awayInput))) {
            notice = "Result saved - standings updated";
            enteringScore = false;
        } else notice = "Could not save that result";
    }

    void drawText(const std::string& value, float x, float y, unsigned size,
                  sf::Color color = sf::Color(248, 250, 252), bool bold = false) {
        if (!fontLoaded) return;
        sf::Text item(value, font, size);
        item.setFillColor(color);
        if (bold) item.setStyle(sf::Text::Bold);
        item.setPosition(x, y);
        window.draw(item);
    }
    void drawRect(float x, float y, float width, float height, sf::Color fill,
                  sf::Color outline = sf::Color::Transparent) {
        sf::RectangleShape shape({width, height});
        shape.setPosition(x, y); shape.setFillColor(fill);
        if (outline.a != 0) { shape.setOutlineThickness(1.f); shape.setOutlineColor(outline); }
        window.draw(shape);
    }

    void draw() {
        window.clear(bg);
        drawSidebar();
        drawHeader();
        if (page == 0) drawDashboard();
        else if (page == 1) drawTeams();
        else if (page == 2) drawPlayers();
        else if (page == 3) drawFixtures();
        else drawStandings();
        drawFooter();
        if (enteringScore) drawScoreDialog();
        window.display();
    }

    void drawSidebar() {
        drawRect(0, 0, 220, 768, sidebarColor);
        drawRect(219, 0, 1, 768, border);
        drawText("FTMS", 24, 24, 24, green, true);
        drawText("FOOTBALL MANAGER", 25, 58, 10, muted);
        drawText("PBL OOPS  /  SEASON 2026", 24, 88, 10, gold);
        const std::array<std::string, 5> labels = {"Dashboard", "Teams Squad", "Players", "Fixtures", "Points Table"};
        for (int i = 0; i < 5; ++i) {
            const float y = 112.f + static_cast<float>(i) * 51.f;
            if (page == i) {
                drawRect(14, y, 190, 40, card);
                drawRect(14, y, 4, 40, green);
            }
            drawText((page == i ? ">  " : "   ") + labels[static_cast<std::size_t>(i)],
                     27, y + 10, 14, page == i ? white : muted, page == i);
        }
    }

    void drawHeader() {
        const std::array<std::string, 5> titles = {"Dashboard & Overview", "Team & Squad Management", "Player Registration & Management", "Fixtures", "League Table"};
        drawText(titles[static_cast<std::size_t>(page)], 250, 24, 25, white, true);
        drawText("League season 2026  /  Single round robin  /  " + notice, 251, 61, 13, muted);
        drawRect(250, 88, 1000, 1, border);
    }

    void drawStatCard(float x, const std::string& label, const std::string& value, sf::Color accent) {
        drawRect(x, 108, 235, 92, card, border);
        drawRect(x, 108, 4, 92, accent);
        drawText(value, x + 18, 123, 26, accent, true);
        drawText(label, x + 18, 165, 12, muted);
    }

    void drawDashboard() {
        const int played = tournament.playedCount();
        const int total = static_cast<int>(tournament.getMatches().size());
        drawStatCard(250, "REGISTERED TEAMS", std::to_string(tournament.getTeams().size()), green);
        drawStatCard(505, "REGISTERED PLAYERS", std::to_string(tournament.getPlayers().size()), white);
        drawStatCard(760, "MATCHES PLAYED", std::to_string(played) + " / " + std::to_string(total), gold);
        drawStatCard(1015, "PENDING MATCHES", std::to_string(total - played), muted);

        const auto order = tournament.standings();
        const auto& teams = tournament.getTeams();
        drawRect(250, 220, 485, 170, card, border);
        drawText("CURRENT TABLE LEADER", 270, 240, 14, green, true);
        if (!order.empty()) {
            drawText(teams[order.front()].name, 270, 275, 23, white, true);
            drawText(std::to_string(teams[order.front()].points) + " points", 270, 316, 15, muted);
        } else drawText("Register teams to start the league", 270, 280, 16, muted);
        drawText("Ranked by points, goal difference, goals scored", 270, 354, 12, muted);

        drawRect(755, 220, 495, 170, card, border);
        drawText("UPCOMING FIXTURES", 775, 240, 14, gold, true);
        const auto& matches = tournament.getMatches();
        int shown = 0;
        for (std::size_t i = 0; i < matches.size() && shown < 3; ++i) {
            const auto& m = matches[i];
            if (m.played) continue;
            drawText(teams[m.home].name + "  vs  " + teams[m.away].name, 775, 272.f + shown * 32.f, 13, white);
            ++shown;
        }
        if (shown == 0) drawText(matches.empty() ? "Register at least two teams to create fixtures" : "All league fixtures are complete", 775, 278, 13, muted);

        drawRect(250, 410, 1000, 245, card, border);
        drawText("LEAGUE SNAPSHOT", 270, 428, 15, white, true);
        drawText("POS", 275, 464, 11, muted); drawText("CLUB", 330, 464, 11, muted);
        drawText("P", 720, 464, 11, muted); drawText("GD", 820, 464, 11, muted);
        drawText("PTS", 925, 464, 11, muted);
        for (std::size_t row = 0; row < order.size(); ++row) {
            const auto& t = teams[order[row]];
            const float y = 491.f + static_cast<float>(row) * 20.f;
            if (row % 2 == 0) drawRect(266, y - 2, 960, 20, sf::Color(22, 30, 28));
            drawText(std::to_string(row + 1), 275, y, 11, row == 0 ? green : muted);
            drawText(t.name, 330, y, 11, white);
            drawText(std::to_string(t.played), 720, y, 11, muted);
            drawText(std::to_string(t.goalDifference()), 820, y, 11, muted);
            drawText(std::to_string(t.points), 925, y, 11, green, true);
        }
    }

    void drawFixtures() {
        drawText("Select a match and press F2 to enter its score", 250, 105, 13, muted);
        const auto& teams = tournament.getTeams();
        const auto& matches = tournament.getMatches();
        drawRect(250, 137, 1000, 500, card, border);
        if (matches.empty()) {
            drawText("No fixtures yet. Register at least two teams in Teams Squad.", 280, 200, 15, muted);
            return;
        }
        drawText("MATCH", 272, 151, 11, muted); drawText("HOME", 370, 151, 11, muted);
        drawText("AWAY", 650, 151, 11, muted); drawText("RESULT", 920, 151, 11, muted);
        const int first = visibleFixtureStart();
        for (int row = 0; row < 14 && first + row < static_cast<int>(matches.size()); ++row) {
            const int index = first + row;
            const auto& m = matches[static_cast<std::size_t>(index)];
            const float y = 180.f + row * 31.f;
            if (index == selectedFixture) drawRect(264, y - 3, 972, 28, sf::Color(36, 54, 47));
            else if (row % 2 == 1) drawRect(264, y - 3, 972, 28, sf::Color(22, 30, 28));
            drawText("#" + std::to_string(index + 1), 277, y, 12, index == selectedFixture ? green : muted);
            drawText(teams[m.home].name, 370, y, 12, white);
            drawText(teams[m.away].name, 650, y, 12, white);
            drawText(m.played ? std::to_string(m.homeGoals) + "  -  " + std::to_string(m.awayGoals) : "Pending",
                     920, y, 12, m.played ? green : muted);
        }
    }

    void drawTeams() {
        drawText("Register participating teams. Team names are entered here, not pre-filled.", 250, 70, 13, muted);
        drawRect(250, 112, 260, 42, bg, focusedField == 0 ? green : border);
        drawRect(525, 112, 240, 42, bg, focusedField == 1 ? green : border);
        drawRect(780, 112, 260, 42, bg, focusedField == 2 ? green : border);
        drawText(teamNameInput.empty() ? "Team name (required)" : teamNameInput, 262, 122, 12, teamNameInput.empty() ? muted : white);
        drawText(managerInput.empty() ? "Coach / manager (optional)" : managerInput, 537, 122, 12, managerInput.empty() ? muted : white);
        drawText(contactInput.empty() ? "Contact (optional)" : contactInput, 792, 122, 12, contactInput.empty() ? muted : white);
        drawRect(1055, 112, 190, 42, green);
        drawText("+ REGISTER TEAM", 1070, 124, 13, bg, true);
        drawText("Registered Teams", 250, 177, 17, green, true);
        drawRect(250, 207, 995, 424, card, border);
        drawText("TEAM", 270, 219, 11, muted); drawText("COACH / MANAGER", 575, 219, 11, muted);
        drawText("CONTACT", 875, 219, 11, muted); drawText("PLAYERS", 1160, 219, 11, muted);
        const auto& teams = tournament.getTeams();
        if (teams.empty()) drawText("No teams registered yet. Enter a team name above and click REGISTER TEAM.", 270, 266, 14, muted);
        for (std::size_t i = 0; i < teams.size(); ++i) {
            const float y = 248.f + static_cast<float>(i) * 45.f;
            if (i % 2 == 0) drawRect(262, y - 3, 971, 39, sf::Color(22, 30, 28));
            drawText(teams[i].name, 270, y + 5, 13, white, true);
            drawText(teams[i].manager.empty() ? "—" : teams[i].manager, 575, y + 5, 12, muted);
            drawText(teams[i].contact.empty() ? "—" : teams[i].contact, 875, y + 5, 12, muted);
            const auto count = std::count_if(tournament.getPlayers().begin(), tournament.getPlayers().end(),
                [i](const Player& player) { return player.team == i; });
            drawText(std::to_string(count), 1172, y + 5, 13, green);
        }
    }

    void drawPlayers() {
        drawText("Add players to a registered team. Jersey numbers must be unique within that team.", 250, 70, 13, muted);
        drawRect(250, 112, 260, 42, bg, focusedField == 0 ? green : border);
        drawRect(525, 112, 120, 42, bg, focusedField == 1 ? green : border);
        drawText(playerNameInput.empty() ? "Player full name" : playerNameInput, 262, 122, 12, playerNameInput.empty() ? muted : white);
        drawText(jerseyInput.empty() ? "Jersey #" : jerseyInput, 537, 122, 12, jerseyInput.empty() ? muted : white);
        drawRect(660, 112, 225, 42, bg, border);
        const auto& teams = tournament.getTeams();
        const std::string teamChoice = teams.empty() ? "Register a team first" : teams[selectedTeam % teams.size()].name + "  (click to change)";
        drawText(teamChoice, 672, 122, 11, teams.empty() ? muted : white);
        const std::array<std::string, 4> shortPositions = {"GK", "DEF", "MID", "FWD"};
        for (int i = 0; i < 4; ++i) {
            const float x = 895.f + static_cast<float>(i) * 39.f;
            drawRect(x, 112, 35, 42, i == selectedPosition ? green : card, i == selectedPosition ? green : border);
            drawText(shortPositions[static_cast<std::size_t>(i)], x + 5, 124, 10, i == selectedPosition ? bg : white, true);
        }
        drawRect(1060, 112, 185, 42, green);
        drawText("+ ADD PLAYER", 1080, 124, 13, bg, true);
        drawText("Players", 250, 177, 17, green, true);
        drawRect(250, 207, 995, 424, card, border);
        drawText("PLAYER", 270, 219, 11, muted); drawText("JERSEY", 580, 219, 11, muted);
        drawText("POSITION", 700, 219, 11, muted); drawText("TEAM", 910, 219, 11, muted);
        const auto& players = tournament.getPlayers();
        if (teams.empty()) drawText("Register a team in Teams Squad before adding players.", 270, 266, 14, muted);
        else if (players.empty()) drawText("No players registered yet. Fill in the form above to add your first player.", 270, 266, 14, muted);
        for (std::size_t i = 0; i < players.size(); ++i) {
            const float y = 248.f + static_cast<float>(i) * 37.f;
            if (i % 2 == 0) drawRect(262, y - 3, 971, 32, sf::Color(22, 30, 28));
            drawText(players[i].name, 270, y + 4, 12, white);
            drawText("#" + std::to_string(players[i].jersey), 580, y + 4, 12, muted);
            drawText(players[i].position, 700, y + 4, 12, muted);
            if (players[i].team < teams.size()) drawText(teams[players[i].team].name, 910, y + 4, 12, white);
        }
    }

    void drawStandings() {
        drawText("Points table  /  3 points for a win, 1 for a draw", 250, 105, 13, muted);
        drawRect(250, 137, 1000, 500, card, border);
        drawText("POS", 272, 151, 11, muted); drawText("CLUB", 330, 151, 11, muted);
        drawText("P", 720, 151, 11, muted); drawText("W", 780, 151, 11, muted);
        drawText("D", 835, 151, 11, muted); drawText("L", 890, 151, 11, muted);
        drawText("GF", 945, 151, 11, muted); drawText("GA", 995, 151, 11, muted);
        drawText("GD", 1050, 151, 11, muted); drawText("PTS", 1120, 151, 11, muted);
        const auto order = tournament.standings();
        const auto& teams = tournament.getTeams();
        for (std::size_t row = 0; row < order.size(); ++row) {
            const auto& t = teams[order[row]];
            const float y = 190.f + static_cast<float>(row) * 48.f;
            if (row == 0) drawRect(264, y - 5, 972, 38, sf::Color(36, 54, 47));
            else if (row % 2 == 0) drawRect(264, y - 5, 972, 38, sf::Color(22, 30, 28));
            drawText(std::to_string(row + 1), 278, y, 13, row == 0 ? green : muted);
            drawText(t.name, 330, y, 13, white);
            drawText(std::to_string(t.played), 720, y, 13, muted);
            drawText(std::to_string(t.wins), 780, y, 13, muted);
            drawText(std::to_string(t.draws), 835, y, 13, muted);
            drawText(std::to_string(t.losses), 890, y, 13, muted);
            drawText(std::to_string(t.goalsFor), 945, y, 13, muted);
            drawText(std::to_string(t.goalsAgainst), 995, y, 13, muted);
            drawText(std::to_string(t.goalDifference()), 1050, y, 13, muted);
            drawText(std::to_string(t.points), 1120, y, 13, green, true);
        }
    }

    void drawFooter() {
        drawRect(220, 710, 1060, 58, sidebarColor);
        drawRect(220, 710, 1060, 1, border);
        drawText("Click menu to navigate     ENTER submit forms     UP / DOWN select fixture     F2 enter score", 250, 720, 12, white);
        drawText("Register teams and squad players in Teams Squad and Players", 250, 741, 10, muted);
    }

    void drawScoreDialog() {
        sf::RectangleShape overlay({1280.f, 768.f});
        overlay.setFillColor(sf::Color(0, 0, 0, 180)); window.draw(overlay);
        drawRect(390, 250, 500, 270, card, border);
        const auto& match = tournament.getMatches()[static_cast<std::size_t>(selectedFixture)];
        const auto& teams = tournament.getTeams();
        drawText("RECORD MATCH RESULT", 425, 276, 20, green, true);
        drawText(teams[match.home].name + " goals", 430, 335, 14, scoreAway ? muted : white);
        drawRect(670, 326, 90, 38, bg, scoreAway ? border : green);
        drawText(homeInput.empty() ? "-" : homeInput, 704, 334, 18, white);
        drawText(teams[match.away].name + " goals", 430, 390, 14, scoreAway ? white : muted);
        drawRect(670, 381, 90, 38, bg, scoreAway ? green : border);
        drawText(awayInput.empty() ? "-" : awayInput, 704, 389, 18, white);
        drawText("Type score  /  TAB changes field  /  ENTER saves  /  ESC cancels", 425, 460, 11, muted);
    }
};

int main() {
    ManagerWindow app;
    app.run();
    return 0;
}



