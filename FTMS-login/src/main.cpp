#include "managers/TournamentManager.hpp"
#include "storage/FileRepository.hpp"
#include "services/FixtureGenerator.hpp"
#include "ui/MainWindow.hpp"
#include "ui/LoginWindow.hpp"
#include "security/Authentication.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Starting Football Tournament Management System (FTMS)..." << std::endl;

    auto repo = std::make_unique<ftms::FileRepository>("data");
    auto generator = std::make_unique<ftms::FixtureGenerator>();
    auto manager = std::make_shared<ftms::TournamentManager>(std::move(repo), std::move(generator));

    manager->initialize();

    // A referee must authenticate before the tournament workspace is opened.
    ftms::FileAuthenticationService authentication("data/referees.db");
    ftms::LoginWindow login(authentication);
    auto signedInUser = login.run();
    if (!signedInUser) {
        std::cout << "No referee session was started." << std::endl;
        return 0;
    }
    std::shared_ptr<const ftms::User> referee(std::move(signedInUser));

    // If no tournaments exist yet, seed a default college tournament with 4 teams for immediate demo
    if (manager->getTournamentSummaries().empty()) {
        std::cout << "Seeding initial College Championship tournament..." << std::endl;
        manager->createTournament("Inter-Department Cup", "2026", "Annual College Football Championship");

        manager->addTeam("Computer Science FC", "Dr. Turing", "cs@college.edu");
        manager->addTeam("Mechanical Warriors", "Dr. Diesel", "mech@college.edu");
        manager->addTeam("Electrical Sparks", "Dr. Tesla", "ee@college.edu");
        manager->addTeam("Civil Titans", "Dr. Archimedes", "civil@college.edu");

        // Seed some sample players
        manager->addPlayer("Alex Hunter", 10, ftms::PlayerPosition::Forward, 1);
        manager->addPlayer("Marcus Smith", 7, ftms::PlayerPosition::Midfielder, 1);
        manager->addPlayer("David Brown", 1, ftms::PlayerPosition::Goalkeeper, 1);

        manager->addPlayer("John Stones", 4, ftms::PlayerPosition::Defender, 2);
        manager->addPlayer("Leo Sterling", 9, ftms::PlayerPosition::Forward, 2);

        manager->addPlayer("Carlos Ray", 11, ftms::PlayerPosition::Forward, 3);
        manager->addPlayer("Sam Wilson", 8, ftms::PlayerPosition::Midfielder, 4);

        // Generate initial fixtures
        manager->generateFixtures();
    }

    ftms::MainWindow window(manager, std::move(referee));
    window.run();

    std::cout << "FTMS closed successfully." << std::endl;
    return 0;
}
