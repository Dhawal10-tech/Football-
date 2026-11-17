
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <bcrypt.h>

#endif

namespace ftms {

enum class TournamentState { Draft, Active, Completed };
enum class Position { Goalkeeper, Defender, Midfielder, Forward };
enum class MatchState { Scheduled, Completed };

struct Player {
  std::string id, name, teamId;
  unsigned jersey{};
  Position position{Position::Midfielder};
};
struct Team {
  std::string id, name, manager, contact;
};
struct Scorer {
  std::string playerId;
  unsigned goals{};
};
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
  unsigned played{}, won{}, drawn{}, lost{}, goalsFor{}, goalsAgainst{},
      points{};
  int goalDifference{};
};
struct PlayerGoals {
  std::string playerId, playerName, teamName;
  unsigned jersey{}, goals{};
};
} // namespace ftms

namespace ftms {
// ABSTRACTION: callers work with a user and an authentication service contract.
// ENCAPSULATION: credentials stay private; only safe identity/permissions
// escape.
class User {
  std::string id_;

protected:
  explicit User(std::string id) : id_(std::move(id)) {}

public:
  virtual ~User() = default;
  const std::string &id() const { return id_; }
  virtual std::string role() const = 0;
  virtual bool mayRecordScores() const = 0;
};
class Referee final : public User { // INHERITANCE
public:
  explicit Referee(std::string id) : User(std::move(id)) {}
  std::string role() const override { return "Referee"; }
  bool mayRecordScores() const override { return true; }
};
class AuthenticationService {
public:
  virtual ~AuthenticationService() = default;
  virtual std::unique_ptr<User> signIn(const std::string &,
                                       const std::string &) = 0;
  virtual bool createReferee(const std::string &, const std::string &,
                             std::string &) = 0;
};
class FileAuthenticationService final : public AuthenticationService {
  std::filesystem::path file_;
  std::string hashPassword(const std::string &, const std::string &) const;
  std::vector<std::pair<std::string, std::string>> accounts() const;

public:
  explicit FileAuthenticationService(std::filesystem::path file)
      : file_(std::move(file)) {}
  std::unique_ptr<User> signIn(const std::string &,
                               const std::string &) override;
  bool createReferee(const std::string &, const std::string &,
                     std::string &) override;
};
// POLYMORPHISM: the login screen depends on the abstract service interface.
class RefereeLogin {
  sf::RenderWindow window_;
  sf::Font font_;
  sf::Clock caretClock_;
  AuthenticationService &auth_;
  bool creating_{}, passwordField_{}, confirmField_{};
  std::size_t focusedInput_{};
  std::string id_, password_, confirm_, message_;
  bool error_{};
  void drawText(const std::string &, float, float, unsigned,
                sf::Color = sf::Color::White);
  std::unique_ptr<User> submit();

public:
  explicit RefereeLogin(AuthenticationService &auth)
      : window_(sf::VideoMode(760, 590), "FTMS | Referee Login"), auth_(auth) {
    window_.setFramerateLimit(60);
    caretClock_.restart();
    font_.loadFromFile(std::string(FTMS_ASSET_DIR) + "/fonts/DejaVuSans.ttf");
  }
  std::unique_ptr<User> run();
};

std::vector<std::pair<std::string, std::string>>
FileAuthenticationService::accounts() const {
  std::vector<std::pair<std::string, std::string>> result;
  std::ifstream in(file_);
  std::string id, hash;
  while (std::getline(in, id, '|') && std::getline(in, hash))
    result.emplace_back(id, hash);
  return result;
}
std::string
FileAuthenticationService::hashPassword(const std::string &password,
                                        const std::string &salt) const {
#ifdef _WIN32
  std::array<UCHAR, 32> key{};
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                  BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
    throw std::runtime_error("Windows SHA-256 provider could not be opened.");
  const NTSTATUS status = BCryptDeriveKeyPBKDF2(
      algorithm, reinterpret_cast<PUCHAR>(const_cast<char *>(password.data())),
      static_cast<ULONG>(password.size()),
      reinterpret_cast<PUCHAR>(const_cast<char *>(salt.data())),
      static_cast<ULONG>(salt.size()), 150000, key.data(),
      static_cast<ULONG>(key.size()), 0);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  if (status < 0)
    throw std::runtime_error("Windows password hashing failed.");
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (auto b : key)
    out << std::setw(2) << static_cast<unsigned>(b);
  return out.str();
#else
  (void)password;
  (void)salt;
  throw std::runtime_error("Referee login currently requires Windows.");
#endif
}
std::unique_ptr<User>
FileAuthenticationService::signIn(const std::string &id,
                                  const std::string &password) {
  auto normalized = [](std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
  };
  for (const auto &[savedId, savedHash] : accounts()) {
    if (normalized(savedId) != normalized(id) || savedHash.size() != 96)
      continue;
    std::string salt;
    for (size_t i = 0; i < 32; i += 2)
      salt.push_back(
          static_cast<char>(std::stoi(savedHash.substr(i, 2), nullptr, 16)));
    if (hashPassword(password, salt) == savedHash.substr(32))
      return std::make_unique<Referee>(savedId);
  }
  return {};
}
bool FileAuthenticationService::createReferee(const std::string &id,
                                              const std::string &password,
                                              std::string &message) {
  if (id.size() < 4 || id.size() > 24 ||
      !std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '-';
      })) {
    message = "ID: use 4-24 letters, numbers, _ or -.";
    return false;
  }
  if (password.size() < 8) {
    message = "Password must contain at least 8 characters.";
    return false;
  }
  auto saved = accounts();
  auto normalized = [](std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
  };
  if (std::any_of(saved.begin(), saved.end(), [&](const auto &a) {
        return normalized(a.first) == normalized(id);
      })) {
    message = "That referee ID already exists.";
    return false;
  }
#ifdef _WIN32
  std::array<UCHAR, 16> salt{};
  if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()),
                      BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
    message = "Could not create secure account.";
    return false;
  }
  std::ostringstream encodedSalt;
  encodedSalt << std::hex << std::setfill('0');
  for (auto b : salt)
    encodedSalt << std::setw(2) << static_cast<unsigned>(b);
  std::string saltText(reinterpret_cast<char *>(salt.data()), salt.size());
  std::ofstream out(file_, std::ios::app);
  if (!out) {
    std::filesystem::create_directories(file_.parent_path());
    out.open(file_, std::ios::app);
  }
  if (!out) {
    message = "Cannot write data/referees.db.";
    return false;
  }
  out << id << '|' << encodedSalt.str() << hashPassword(password, saltText)
      << '\n';
  message = "Referee ID created. Sign in now.";
  return true;
#else
  message = "Referee login currently requires Windows.";
  return false;
#endif
}
void RefereeLogin::drawText(const std::string &text, float x, float y,
                            unsigned size, sf::Color color) {
  if (!font_.getInfo().family.empty()) {
    sf::Text t(text, font_, size);
    t.setPosition(x, y);
    t.setFillColor(color);
    window_.draw(t);
  }
}
std::unique_ptr<User> RefereeLogin::submit() {
  if (creating_) {
    if (password_ != confirm_) {
      message_ = "Passwords do not match.";
      error_ = true;
      return {};
    }
    std::string info;
    if (!auth_.createReferee(id_, password_, info)) {
      message_ = info;
      error_ = true;
      return {};
    }
    creating_ = false;
    password_.clear();
    confirm_.clear();
    message_ = info;
    error_ = false;
    return {};
  }
  auto user = auth_.signIn(id_, password_);
  if (!user || !user->mayRecordScores()) {
    message_ = "Invalid referee ID or password.";
    error_ = true;
    return {};
  }
  return user;
}
std::unique_ptr<User> RefereeLogin::run() {
  while (window_.isOpen()) {
    sf::Event e{};
    while (window_.pollEvent(e)) {
      if (e.type == sf::Event::Closed) {
        window_.close();
        return {};
      }
      if (e.type == sf::Event::KeyPressed &&
          e.key.code == sf::Keyboard::Escape) {
        window_.close();
        return {};
      }
      if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Tab) {
        const std::size_t fieldCount = creating_ ? 3u : 2u;
        focusedInput_ = (focusedInput_ + 1) % fieldCount;
        passwordField_ = focusedInput_ != 0;
        confirmField_ = focusedInput_ == 2;
        caretClock_.restart();
      }
      if (e.type == sf::Event::TextEntered) {
        if (e.text.unicode == 8) {
          if (passwordField_) {
            if (confirmField_ && !confirm_.empty())
              confirm_.pop_back();
            else if (!password_.empty())
              password_.pop_back();
          } else if (!id_.empty())
            id_.pop_back();
        } else if (e.text.unicode >= 32 && e.text.unicode < 127) {
          char c = static_cast<char>(e.text.unicode);
          if (passwordField_) {
            if (creating_ && confirmField_)
              confirm_ += c;
            else
              password_ += c;
          } else
            id_ += c;
        }
        caretClock_.restart();
      }
      if (e.type == sf::Event::KeyPressed &&
          e.key.code == sf::Keyboard::Enter) {
        auto u = submit();
        if (u) {
          window_.close();
          return u;
        }
      }
      if (e.type == sf::Event::MouseButtonPressed &&
          e.mouseButton.button == sf::Mouse::Left) {
        auto p = sf::Vector2f(static_cast<float>(e.mouseButton.x),
                              static_cast<float>(e.mouseButton.y));
        if (sf::FloatRect(210, 425, 340, 52).contains(p)) {
          auto u = submit();
          if (u) {
            window_.close();
            return u;
          }
        }
        if (sf::FloatRect(210, 490, 340, 42).contains(p)) {
          creating_ = !creating_;
          focusedInput_ = 0;
          passwordField_ = false;
          confirmField_ = false;
          message_.clear();
        }
        if (sf::FloatRect(210, 220, 340, 48).contains(p)) {
          focusedInput_ = 0;
          passwordField_ = false;
          confirmField_ = false;
        }
        if (sf::FloatRect(210, 290, 340, 48).contains(p)) {
          focusedInput_ = 1;
          passwordField_ = true;
          confirmField_ = false;
        }
        if (creating_ && sf::FloatRect(210, 360, 340, 48).contains(p)) {
          focusedInput_ = 2;
          passwordField_ = true;
          confirmField_ = true;
        }
        caretClock_.restart();
      }
    }
    window_.clear(sf::Color(15, 25, 23));
    sf::RectangleShape panel({500, 520});
    panel.setPosition(130, 35);
    panel.setFillColor(sf::Color(25, 39, 36));
    window_.draw(panel);
    drawText("FTMS  /  OFFICIAL ACCESS", 175, 65, 15, sf::Color(60, 210, 150));
    drawText(creating_ ? "Create referee account" : "Referee sign in", 175, 105,
             30);
    drawText("Only signed-in referees can update match scores.", 175, 150, 14,
             sf::Color(165, 180, 175));
    const bool cursorOn =
        caretClock_.getElapsedTime().asMilliseconds() % 1000 < 560;
    auto drawLoginInput = [&](float y, const std::string &label,
                              const std::string &value, std::size_t index,
                              bool secret) {
      const bool focused = focusedInput_ == index;
      drawText(label, 210, y - 18, 12, sf::Color(60, 210, 150));
      sf::RectangleShape box({340, 42});
      box.setPosition(210, y);
      box.setFillColor(sf::Color(222, 228, 235));
      box.setOutlineThickness(focused ? 2.f : 1.f);
      box.setOutlineColor(focused ? sf::Color(21, 183, 126)
                                  : sf::Color(72, 88, 103));
      window_.draw(box);
      const std::string visible =
          secret ? std::string(value.size(), '*') : value;
      const std::string placeholder =
          secret ? "Enter password" : "Type your unique ID";
      std::size_t first = 0;
      std::size_t last = visible.size();
      constexpr float textX = 222.f;
      constexpr float textRight = 538.f;
      auto widthOf = [&](std::size_t begin, std::size_t end) {
        sf::Text measured(visible.substr(begin, end - begin), font_, 15);
        return measured.getLocalBounds().width;
      };
      while (first < last && widthOf(first, last) > textRight - textX - 8.f)
        ++first;
      const std::string shown =
          visible.empty() ? placeholder : visible.substr(first, last - first);
      drawText(shown, textX, y + 13, 15,
               visible.empty() ? sf::Color(92, 105, 121)
                               : sf::Color(18, 27, 40));
      if (focused && cursorOn) {
        sf::Text measured(visible.substr(first), font_, 15);
        sf::RectangleShape caret({2.f, 19.f});
        const float caretX = std::min(
            textRight - 3.f, textX + measured.getLocalBounds().width);
        caret.setPosition(caretX, y + 11.f);
        caret.setFillColor(sf::Color::Black);
        window_.draw(caret);
      }
    };
    drawLoginInput(228, "REFEREE ID", id_, 0, false);
    drawLoginInput(303, "PASSWORD", password_, 1, true);
    if (creating_)
      drawLoginInput(373, "CONFIRM PASSWORD", confirm_, 2, true);
    sf::RectangleShape button({340, 48});
    button.setPosition(210, 425);
    button.setFillColor(sf::Color(15, 180, 125));
    window_.draw(button);
    drawText(creating_ ? "CREATE REFEREE ID" : "SIGN IN", 325, 437, 17,
             sf::Color(10, 25, 20));
    drawText(creating_ ? "I already have an account"
                       : "First time? Create referee ID",
             245, 493, 14, sf::Color(60, 210, 150));
    if (!message_.empty())
      drawText(message_, 175, 540, 13,
               error_ ? sf::Color(255, 125, 115) : sf::Color(100, 220, 150));
    window_.display();
  }
  return {};
}

class ValidationService {
public:
  static std::string normalize(const std::string &value);
  static void addTeam(const Tournament &, const Team &);
  static void addPlayer(const Tournament &, const Player &);
  static void validateResult(const Tournament &, const Match &, unsigned,
                             unsigned, const std::vector<Scorer> &);
};
class TournamentFormat {
public:
  virtual ~TournamentFormat() = default;
  virtual std::vector<Match> generate(const Tournament &) const = 0;
};
class RoundRobinFormat final : public TournamentFormat {
public:
  std::vector<Match> generate(const Tournament &) const override;
};
class StandingsCalculator {
public:
  static std::vector<Standing> calculate(const Tournament &);
};
class StatisticsManager {
public:
  static std::vector<PlayerGoals> goals(const Tournament &);
};
class TournamentManager {
public:
  static void start(Tournament &);
  static void recordResult(Tournament &, const std::string &, unsigned,
                           unsigned, std::vector<Scorer> = {});
  static void finalize(Tournament &);
  static void reopen(Tournament &);
};
} // namespace ftms
namespace ftms {
class StorageInterface {
public:
  virtual ~StorageInterface() = default;
  virtual std::vector<Tournament> load() const = 0;
  virtual void save(const std::vector<Tournament> &) const = 0;
};
class FileRepository final : public StorageInterface {
  std::filesystem::path path_;

public:
  explicit FileRepository(std::filesystem::path path);
  std::vector<Tournament> load() const override;
  void save(const std::vector<Tournament> &) const override;
};
} // namespace ftms
namespace ftms {
class Application {
  enum class Page {
    Dashboard,
    Tournaments,
    Teams,
    Players,
    Fixtures,
    Matches,
    Standings,
    Statistics,
    History,
    Reports,
    About
  };
  enum class Dialog {
    None,
    Tournament,
    Team,
    Player,
    Score,
    EditTournament,
    EditTeam,
    EditPlayer,
    ConfirmDeleteTournament,
    ConfirmDeleteTeam,
    ConfirmDeletePlayer
  };
  sf::RenderWindow window_;
  sf::Font font_;
  bool fontLoaded_{};
  std::string refereeId_;
  std::string refereeRole_;
  FileRepository repository_;
  std::vector<Tournament> tournaments_;
  size_t active_{};
  Page page_{Page::Dashboard};
  Dialog dialog_{Dialog::None};
  std::vector<std::string> fields_, labels_;
  size_t focusedField_{};
  std::vector<std::size_t> caretPositions_;
  std::size_t searchCaretPosition_{};
  sf::Clock caretClock_;
  std::string message_, loadError_, search_;
  std::string selectedMatchId_;
  bool messageIsError_{}, searchFocused_{};
  int selectedRow_{-1}, scroll_{};
  sf::Vector2i mouse_{};
  void drawText(const std::string &, float, float, unsigned,
                sf::Color = sf::Color::White);
  void drawButton(const std::string &, const sf::FloatRect &,
                  bool enabled = true, bool accent = false);
  void drawCard(const sf::FloatRect &, sf::Color = sf::Color(31, 57, 46));
  void drawMatchCard(const Tournament &, const Match &, const sf::FloatRect &,
                     bool selected = false);
  Tournament *activeTournament();
  const Tournament *activeTournament() const;
  void beginDialog(Dialog, std::vector<std::string>, std::vector<std::string>);
  void submitDialog();
  void saveOrRollback(std::vector<Tournament> before);
  std::string nextId(const std::string &,
                     const std::vector<std::string> &) const;
  void drawSidebar();
  void drawPage();
  void drawDialog();
  void handleClick(sf::Vector2f);
  void showMessage(std::string, bool error = false);
  void exportReport();

public:
  explicit Application(const User &user);
  int run();
};
} // namespace ftms

// ---- Consolidated from src/models.cpp ----
// Domain records are value types. Their private invariants are enforced by the
// services that create and mutate them, keeping the persisted model simple.

// ---- Consolidated from src/services.cpp ----
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace ftms {
std::string ValidationService::normalize(const std::string &s) {
  auto first = s.find_first_not_of(" \t\r\n"),
       last = s.find_last_not_of(" \t\r\n");
  if (first == std::string::npos)
    return {};
  std::string out = s.substr(first, last - first + 1);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}
void ValidationService::addTeam(const Tournament &t, const Team &team) {
  if (normalize(team.id).empty() || normalize(team.name).empty())
    throw std::invalid_argument("Team ID and name are required.");
  for (const auto &x : t.teams) {
    if (x.id == team.id)
      throw std::invalid_argument("Team ID already exists.");
    if (normalize(x.name) == normalize(team.name))
      throw std::invalid_argument(
          "A team with this name already exists in the tournament.");
  }
  if (t.state != TournamentState::Draft || !t.matches.empty())
    throw std::logic_error(
        "Teams can only be changed before fixtures are generated.");
}
void ValidationService::addPlayer(const Tournament &t, const Player &p) {
  if (normalize(p.id).empty() || normalize(p.name).empty() || p.jersey == 0)
    throw std::invalid_argument(
        "Player ID, name and a positive jersey number are required.");
  if (std::none_of(t.teams.begin(), t.teams.end(),
                   [&](const Team &x) { return x.id == p.teamId; }))
    throw std::invalid_argument("Player must belong to a registered team.");
  for (const auto &x : t.players) {
    if (x.id == p.id)
      throw std::invalid_argument("Player ID already exists.");
    if (x.teamId == p.teamId && x.jersey == p.jersey)
      throw std::invalid_argument(
          "Jersey number already exists for this team.");
  }
  if (t.state != TournamentState::Draft || !t.matches.empty())
    throw std::logic_error(
        "Players can only be changed before fixtures are generated.");
}
void ValidationService::validateResult(const Tournament &t, const Match &m,
                                       unsigned h, unsigned a,
                                       const std::vector<Scorer> &scorers) {
  if (t.state != TournamentState::Active)
    throw std::logic_error(
        "Results can only be recorded while the tournament is active.");
  if (h > 1000 || a > 1000)
    throw std::invalid_argument(
        "Scores must be non-negative integers no greater than 1000.");
  if (m.homeTeamId == m.awayTeamId)
    throw std::invalid_argument("A team cannot play itself.");
  std::set<std::string> seen;
  for (auto &s : scorers) {
    auto p = std::find_if(t.players.begin(), t.players.end(),
                          [&](const Player &x) { return x.id == s.playerId; });
    if (p == t.players.end())
      throw std::invalid_argument("Goalscorer is not a registered player.");
    if (p->teamId != m.homeTeamId && p->teamId != m.awayTeamId)
      throw std::invalid_argument(
          "Goalscorer must belong to a team in this match.");
    if (!seen.insert(s.playerId).second)
      throw std::invalid_argument("Use one goalscorer entry per player.");
    if (s.goals == 0 || s.goals > (p->teamId == m.homeTeamId ? h : a))
      throw std::invalid_argument(
          "Goalscorer total exceeds that player's team's match score.");
  }
}
std::vector<Match> RoundRobinFormat::generate(const Tournament &t) const {
  if (t.teams.empty())
    throw std::invalid_argument(
        "Add at least one team before previewing fixtures.");
  if (t.teams.size() == 1)
    return {};
  if (!t.matches.empty())
    throw std::logic_error(
        "Fixtures already exist; regeneration is not allowed.");
  std::vector<std::string> ids;
  for (auto &team : t.teams)
    ids.push_back(team.id);
  if (ids.size() % 2)
    ids.push_back("");
  const auto n = ids.size();
  std::vector<Match> result;
  unsigned round = 1;
  for (size_t r = 0; r + 1 < n; ++r) {
    for (size_t i = 0; i < n / 2; ++i) {
      auto a = ids[i], b = ids[n - 1 - i];
      if (a.empty() || b.empty())
        continue;
      if (r % 2)
        std::swap(a, b);
      result.push_back(Match{"M" + std::to_string(result.size() + 1),
                             a,
                             b,
                             {},
                             round,
                             MatchState::Scheduled,
                             {},
                             {},
                             {}});
    }
    auto last = ids.back();
    ids.pop_back();
    ids.insert(ids.begin() + 1, last);
    ++round;
  }
  const size_t expected = t.teams.size() * (t.teams.size() - 1) / 2;
  if (result.size() != expected)
    throw std::logic_error(
        "Fixture generator produced an unexpected number of matches.");
  return result;
}
std::vector<Standing> StandingsCalculator::calculate(const Tournament &t) {
  std::vector<Standing> table;
  for (auto &team : t.teams)
    table.push_back(Standing{team.id, team.name});
  std::unordered_map<std::string, size_t> ix;
  for (size_t i = 0; i < table.size(); ++i)
    ix[table[i].teamId] = i;
  for (auto &m : t.matches)
    if (m.state == MatchState::Completed && m.homeScore && m.awayScore) {
      if (!ix.contains(m.homeTeamId) || !ix.contains(m.awayTeamId))
        throw std::runtime_error("Match references an unknown team.");
      auto &h = table[ix.at(m.homeTeamId)];
      auto &a = table[ix.at(m.awayTeamId)];
      ++h.played;
      ++a.played;
      h.goalsFor += *m.homeScore;
      h.goalsAgainst += *m.awayScore;
      a.goalsFor += *m.awayScore;
      a.goalsAgainst += *m.homeScore;
      if (*m.homeScore > *m.awayScore) {
        ++h.won;
        ++a.lost;
        h.points += 3;
      } else if (*m.homeScore < *m.awayScore) {
        ++a.won;
        ++h.lost;
        a.points += 3;
      } else {
        ++h.drawn;
        ++a.drawn;
        ++h.points;
        ++a.points;
      }
    }
  for (auto &s : table)
    s.goalDifference =
        static_cast<int>(s.goalsFor) - static_cast<int>(s.goalsAgainst);
  std::sort(table.begin(), table.end(),
            [](const Standing &a, const Standing &b) {
              if (a.points != b.points)
                return a.points > b.points;
              if (a.goalDifference != b.goalDifference)
                return a.goalDifference > b.goalDifference;
              if (a.goalsFor != b.goalsFor)
                return a.goalsFor > b.goalsFor;
              return a.teamId < b.teamId;
            });
  return table;
}
std::vector<PlayerGoals> StatisticsManager::goals(const Tournament &t) {
  std::unordered_map<std::string, unsigned> totals;
  for (auto &m : t.matches)
    if (m.state == MatchState::Completed)
      for (auto &s : m.scorers)
        totals[s.playerId] += s.goals;
  std::vector<PlayerGoals> result;
  for (auto &p : t.players) {
    auto team = std::find_if(t.teams.begin(), t.teams.end(),
                             [&](const Team &x) { return x.id == p.teamId; });
    result.push_back({p.id, p.name,
                      team == t.teams.end() ? "[missing team]" : team->name,
                      p.jersey, totals[p.id]});
  }
  std::sort(result.begin(), result.end(), [](auto &a, auto &b) {
    if (a.goals != b.goals)
      return a.goals > b.goals;
    return a.playerId < b.playerId;
  });
  return result;
}
void TournamentManager::start(Tournament &t) {
  if (t.state != TournamentState::Draft)
    throw std::logic_error("Only a draft tournament can be started.");
  if (t.teams.size() < 2)
    throw std::logic_error(
        "At least two teams are required to start a tournament.");
  RoundRobinFormat format;
  auto matches = format.generate(t);
  t.matches = std::move(matches);
  t.state = TournamentState::Active;
}
void TournamentManager::recordResult(Tournament &t, const std::string &id,
                                     unsigned h, unsigned a,
                                     std::vector<Scorer> scorers) {
  if (t.state != TournamentState::Active)
    throw std::logic_error("Tournament is not active.");
  auto it = std::find_if(t.matches.begin(), t.matches.end(),
                         [&](const Match &m) { return m.id == id; });
  if (it == t.matches.end())
    throw std::invalid_argument("Match was not found.");
  ValidationService::validateResult(t, *it, h, a, scorers);
  it->homeScore = h;
  it->awayScore = a;
  it->scorers = std::move(scorers);
  it->state = MatchState::Completed;
}
void TournamentManager::finalize(Tournament &t) {
  if (t.state != TournamentState::Active || t.matches.empty())
    throw std::logic_error("An active tournament with fixtures is required.");
  if (std::any_of(t.matches.begin(), t.matches.end(),
                  [](auto &m) { return m.state != MatchState::Completed; }))
    throw std::logic_error(
        "All fixtures must be completed before finalization.");
  auto s = StandingsCalculator::calculate(t);
  const bool tied = s.size() >= 2 && s[0].points == s[1].points &&
                    s[0].goalDifference == s[1].goalDifference &&
                    s[0].goalsFor == s[1].goalsFor;
  if (s.size() < 2 || tied)
    throw std::logic_error("The champion is tied under points, goal difference "
                           "and goals scored; resolve the tie explicitly.");
  t.state = TournamentState::Completed;
}
void TournamentManager::reopen(Tournament &t) {
  if (t.state != TournamentState::Completed)
    throw std::logic_error("Only a completed tournament can be reopened.");
  t.state = TournamentState::Active;
}
} // namespace ftms

// ---- Consolidated from src/storage.cpp ----
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace ftms {
namespace {
void check(bool ok, const std::filesystem::path &p) {
  if (!ok)
    throw std::runtime_error("Invalid or corrupt tournament data in " +
                             p.string());
}
} // namespace
FileRepository::FileRepository(std::filesystem::path p) : path_(std::move(p)) {}
std::vector<Tournament> FileRepository::load() const {
  std::ifstream in(path_);
  if (!in) {
    if (!std::filesystem::exists(path_))
      return {};
    throw std::runtime_error("Could not open tournament data file: " +
                             path_.string());
  }
  std::string line;
  check(static_cast<bool>(std::getline(in, line)) && line == "FTMS 1", path_);
  std::vector<Tournament> ts;
  std::string tag;
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    std::istringstream row(line);
    row >> tag;
    if (tag == "T") {
      Tournament t;
      int st;
      row >> std::quoted(t.id) >> std::quoted(t.name) >>
          std::quoted(t.season) >> std::quoted(t.description) >> st;
      check(static_cast<bool>(row) && st >= 0 && st <= 2, path_);
      t.state = static_cast<TournamentState>(st);
      ts.push_back(std::move(t));
    } else if (tag == "E") {
      std::string id;
      row >> std::quoted(id);
      check(static_cast<bool>(row) && !ts.empty() && ts.back().id == id, path_);
    } else {
      check(!ts.empty(), path_);
      auto &t = ts.back();
      if (tag == "TEAM") {
        Team x;
        row >> std::quoted(x.id) >> std::quoted(x.name) >>
            std::quoted(x.manager) >> std::quoted(x.contact);
        check(static_cast<bool>(row), path_);
        t.teams.push_back(std::move(x));
      } else if (tag == "PLAYER") {
        Player x;
        int pos;
        row >> std::quoted(x.id) >> std::quoted(x.name) >>
            std::quoted(x.teamId) >> x.jersey >> pos;
        check(static_cast<bool>(row) && pos >= 0 && pos <= 3, path_);
        x.position = static_cast<Position>(pos);
        t.players.push_back(std::move(x));
      } else if (tag == "MATCH") {
        Match x;
        int state, hh, ha;
        unsigned hs = 0, as = 0;
        row >> std::quoted(x.id) >> std::quoted(x.homeTeamId) >>
            std::quoted(x.awayTeamId) >> std::quoted(x.dateTime) >> x.round >>
            state >> hh >> hs >> ha >> as;
        check(static_cast<bool>(row) && state >= 0 && state <= 1 && hh >= 0 &&
                  hh <= 1 && ha >= 0 && ha <= 1,
              path_);
        x.state = static_cast<MatchState>(state);
        if (hh)
          x.homeScore = hs;
        if (ha)
          x.awayScore = as;
        t.matches.push_back(std::move(x));
      } else if (tag == "SCORER") {
        std::string mid;
        Scorer x;
        row >> std::quoted(mid) >> std::quoted(x.playerId) >> x.goals;
        check(static_cast<bool>(row), path_);
        auto m = std::find_if(t.matches.begin(), t.matches.end(),
                              [&](const Match &v) { return v.id == mid; });
        check(m != t.matches.end(), path_);
        m->scorers.push_back(std::move(x));
      } else
        check(false, path_);
    }
  }
  check(in.eof(), path_);
  std::set<std::string> tids;
  for (const auto &t : ts) {
    check(!t.id.empty() && tids.insert(t.id).second, path_);
    std::set<std::string> teams, players, matches;
    for (const auto &x : t.teams)
      check(!x.id.empty() && teams.insert(x.id).second, path_);
    for (const auto &x : t.players)
      check(!x.id.empty() && players.insert(x.id).second &&
                teams.contains(x.teamId),
            path_);
    for (const auto &m : t.matches)
      check(!m.id.empty() && matches.insert(m.id).second &&
                teams.contains(m.homeTeamId) && teams.contains(m.awayTeamId) &&
                m.homeTeamId != m.awayTeamId &&
                (m.homeScore.has_value() == m.awayScore.has_value()) &&
                ((m.state == MatchState::Completed) == m.homeScore.has_value()),
            path_);
    for (const auto &m : t.matches) {
      std::set<std::string> scorerIds;
      for (const auto &s : m.scorers) {
        auto p =
            std::find_if(t.players.begin(), t.players.end(),
                         [&](const Player &v) { return v.id == s.playerId; });
        check(p != t.players.end() && scorerIds.insert(s.playerId).second &&
                  s.goals > 0 && m.state == MatchState::Completed &&
                  (p->teamId == m.homeTeamId || p->teamId == m.awayTeamId) &&
                  s.goals <=
                      (p->teamId == m.homeTeamId ? *m.homeScore : *m.awayScore),
              path_);
      }
    }
  }
  return ts;
}
void FileRepository::save(const std::vector<Tournament> &ts) const {
  if (!path_.parent_path().empty())
    std::filesystem::create_directories(path_.parent_path());
  auto temp = path_;
  temp += ".tmp";
  auto backup = path_;
  backup += ".bak";
  {
    std::ofstream out(temp, std::ios::trunc);
    if (!out)
      throw std::runtime_error("Could not create temporary data file.");
    out << "FTMS 1\n";
    for (const auto &t : ts) {
      out << "T " << std::quoted(t.id) << ' ' << std::quoted(t.name) << ' '
          << std::quoted(t.season) << ' ' << std::quoted(t.description) << ' '
          << static_cast<int>(t.state) << '\n';
      for (const auto &x : t.teams)
        out << "TEAM " << std::quoted(x.id) << ' ' << std::quoted(x.name) << ' '
            << std::quoted(x.manager) << ' ' << std::quoted(x.contact) << '\n';
      for (const auto &x : t.players)
        out << "PLAYER " << std::quoted(x.id) << ' ' << std::quoted(x.name)
            << ' ' << std::quoted(x.teamId) << ' ' << x.jersey << ' '
            << static_cast<int>(x.position) << '\n';
      for (const auto &m : t.matches) {
        out << "MATCH " << std::quoted(m.id) << ' ' << std::quoted(m.homeTeamId)
            << ' ' << std::quoted(m.awayTeamId) << ' '
            << std::quoted(m.dateTime) << ' ' << m.round << ' '
            << static_cast<int>(m.state) << ' ' << (m.homeScore ? 1 : 0) << ' '
            << m.homeScore.value_or(0) << ' ' << (m.awayScore ? 1 : 0) << ' '
            << m.awayScore.value_or(0) << '\n';
        for (const auto &s : m.scorers)
          out << "SCORER " << std::quoted(m.id) << ' '
              << std::quoted(s.playerId) << ' ' << s.goals << '\n';
      }
      out << "E " << std::quoted(t.id) << '\n';
    }
    out.flush();
    if (!out)
      throw std::runtime_error(
          "Failed writing tournament data; existing data was preserved.");
  }
  bool moved = false;
  try {
    if (std::filesystem::exists(path_)) {
      std::error_code ec;
      std::filesystem::remove(backup, ec);
      std::filesystem::rename(path_, backup);
      moved = true;
    }
    std::filesystem::rename(temp, path_);
    if (moved) {
      std::error_code ec;
      std::filesystem::remove(backup, ec);
    }
  } catch (...) {
    if (moved && !std::filesystem::exists(path_)) {
      std::error_code ec;
      std::filesystem::rename(backup, path_, ec);
    }
    throw;
  }
}
} // namespace ftms

// ---- Consolidated from src/ui/application.cpp ----
#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace ftms {
namespace {
const sf::Color kBg(10, 16, 27), kSidebar(12, 20, 31), kPanel(20, 30, 44),
    kPanelRaised(25, 38, 54), kInput(222, 228, 235), kBorder(43, 58, 76),
    kMuted(151, 165, 183), kText(238, 243, 249), kAccent(21, 183, 126),
    kAccentLight(70, 220, 163), kAccentWash(20, 58, 48), kGold(245, 177, 68),
    kRed(225, 86, 83);
void drawRoundedRect(sf::RenderTarget &target, const sf::FloatRect &bounds,
                     sf::Color fill, sf::Color outline = sf::Color::Transparent,
                     float outlineWidth = 0.f, float radius = 11.f,
                     bool shadow = false) {
  const float width = std::max(0.f, bounds.width),
              height = std::max(0.f, bounds.height),
              r = std::min({radius, width / 2.f, height / 2.f});
  const std::array<sf::Vector2f, 4> centers = {
      {{width - r, r}, {width - r, height - r}, {r, height - r}, {r, r}}};
  const std::array<float, 4> starts = {-90.f, 0.f, 90.f, 180.f};
  sf::ConvexShape shape;
  shape.setPointCount(28);
  for (std::size_t corner = 0; corner < centers.size(); ++corner) {
    for (std::size_t step = 0; step < 7; ++step) {
      const float angle = (starts[corner] + 90.f * step / 6.f) * 0.0174532925f;
      shape.setPoint(corner * 7 + step,
                     {centers[corner].x + r * std::cos(angle),
                      centers[corner].y + r * std::sin(angle)});
    }
  }
  shape.setPosition(bounds.left, bounds.top);
  if (shadow) {
    shape.setPosition(bounds.left, bounds.top + 4.f);
    shape.setFillColor(sf::Color(0, 0, 0, 38));
    shape.setOutlineThickness(0.f);
    target.draw(shape);
    shape.setPosition(bounds.left, bounds.top);
  }
  shape.setFillColor(fill);
  shape.setOutlineColor(outline);
  shape.setOutlineThickness(outlineWidth);
  target.draw(shape);
}
std::string stateName(TournamentState s) {
  switch (s) {
  case TournamentState::Draft:
    return "Draft";
  case TournamentState::Active:
    return "Active";
  case TournamentState::Completed:
    return "Completed";
  }
  return "Unknown";
}
std::string positionName(Position p) {
  switch (p) {
  case Position::Goalkeeper:
    return "Goalkeeper";
  case Position::Defender:
    return "Defender";
  case Position::Midfielder:
    return "Midfielder";
  case Position::Forward:
    return "Forward";
  }
  return "Unknown";
}
std::string teamName(const Tournament &t, const std::string &id) {
  auto i = std::find_if(t.teams.begin(), t.teams.end(),
                        [&](const Team &x) { return x.id == id; });
  return i == t.teams.end() ? "Unknown team" : i->name;
}
std::string matchLabel(const Tournament &t, const Match &m) {
  std::ostringstream s;
  s << "R" << m.round << "  " << teamName(t, m.homeTeamId) << "  ";
  if (m.homeScore && m.awayScore)
    s << *m.homeScore << " - " << *m.awayScore;
  else
    s << "vs";
  s << "  " << teamName(t, m.awayTeamId);
  return s.str();
}
std::string csv(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '\"')
      out += '\"';
    out += c;
  }
  return out + '\"';
}
unsigned parseUnsigned(const std::string &value, unsigned maximum,
                       const std::string &label) {
  if (value.empty())
    throw std::invalid_argument(label + " is required.");
  unsigned long long n = 0;
  for (unsigned char c : value) {
    if (!std::isdigit(c))
      throw std::invalid_argument(label + " must contain digits only.");
    n = n * 10 + (c - '0');
    if (n > maximum)
      throw std::invalid_argument(label + " exceeds the allowed maximum.");
  }
  return static_cast<unsigned>(n);
}
} // namespace

Application::Application(const User &user)
    : window_(sf::VideoMode(1180, 760), "Football Tournament Management System",
              sf::Style::Default, sf::ContextSettings(0, 0, 4)),
      refereeId_(user.id()), refereeRole_(user.role()),
      repository_(std::filesystem::path(FTMS_DATA_DIR) / "tournaments.ftms") {
  window_.setFramerateLimit(60);
  fontLoaded_ =
      font_.loadFromFile(std::string(FTMS_ASSET_DIR) + "/fonts/DejaVuSans.ttf");
  try {
    tournaments_ = repository_.load();
    if (!tournaments_.empty())
      active_ = 0;
  } catch (const std::exception &e) {
    loadError_ = e.what();
  }
}
Tournament *Application::activeTournament() {
  return tournaments_.empty() || active_ >= tournaments_.size()
             ? nullptr
             : &tournaments_[active_];
}
const Tournament *Application::activeTournament() const {
  return tournaments_.empty() || active_ >= tournaments_.size()
             ? nullptr
             : &tournaments_[active_];
}
void Application::drawText(const std::string &s, float x, float y,
                           unsigned size, sf::Color c) {
  if (!fontLoaded_)
    return;
  sf::Text t(s, font_, size);
  t.setFillColor(c);
  t.setPosition(x, y);
  window_.draw(t);
}
void Application::drawButton(const std::string &s, const sf::FloatRect &r,
                             bool enabled, bool accent) {
  const bool hovered = enabled && r.contains(static_cast<float>(mouse_.x),
                                             static_cast<float>(mouse_.y));
  const sf::Color fill = !enabled ? kPanelRaised
                         : accent
                             ? (hovered ? sf::Color(31, 205, 145) : kAccent)
                             : (hovered ? sf::Color(35, 52, 70) : kPanelRaised);
  drawRoundedRect(window_, r, fill, accent && enabled ? kAccentLight : kBorder,
                  1.f, 8.f, accent && enabled);
  sf::Text label(s, font_, 14);
  label.setFillColor(accent && enabled ? sf::Color(7, 28, 24)
                     : enabled         ? kText
                                       : kMuted);
  const auto bounds = label.getLocalBounds();
  label.setPosition(r.left + (r.width - bounds.width) / 2.f - bounds.left,
                    r.top + (r.height - bounds.height) / 2.f - bounds.top);
  window_.draw(label);
}
void Application::drawCard(const sf::FloatRect &r, sf::Color c) {
  drawRoundedRect(window_, r, c, kBorder, 1.f, 12.f, true);
}
void Application::drawMatchCard(const Tournament &t, const Match &m,
                                const sf::FloatRect &r, bool selected) {
  const bool completed =
      m.state == MatchState::Completed && m.homeScore && m.awayScore;
  const bool compact = r.height < 56.f;
  drawCard(r, selected ? kAccentWash : kPanel);
  const float roundWidth = compact ? 38.f : 48.f;
  const float roundHeight = compact ? 22.f : 26.f;
  sf::FloatRect roundBadge(r.left + (compact ? 8.f : 12.f),
                           r.top + (r.height - roundHeight) / 2.f,
                           roundWidth, roundHeight);
  drawRoundedRect(window_, roundBadge, kPanelRaised, kBorder, 1.f, 7.f);
  std::ostringstream roundText;
  roundText << 'R' << std::setw(2) << std::setfill('0') << m.round;
  drawText(roundText.str(), roundBadge.left + (compact ? 5.f : 8.f),
           roundBadge.top + (compact ? 4.f : 5.f), compact ? 10 : 12,
           kAccentLight);

  const float center = r.left + r.width * (compact ? 0.50f : 0.52f);
  const float scoreWidth = compact ? 34.f : 40.f;
  const float scoreGap = compact ? 5.f : 9.f;
  const float scoreLeft = center - scoreWidth - scoreGap / 2.f;
  const float scoreRight = center + scoreGap / 2.f;
  const float teamLeft = r.left + (compact ? 52.f : 72.f);
  const float gap = compact ? 8.f : 18.f;
  const float homeWidth = std::max(36.f, scoreLeft - teamLeft - gap);
  const float awayLeft = scoreRight + scoreWidth + gap;
  const float statusWidth = compact ? 0.f : 104.f;
  const float awayWidth =
      std::max(36.f, r.left + r.width - statusWidth -
                         (compact ? 12.f : 20.f) - awayLeft);
  sf::Text home(teamName(t, m.homeTeamId), font_, compact ? 12 : 15);
  home.setFillColor(kText);
  while (home.getLocalBounds().width > homeWidth &&
         home.getCharacterSize() > (compact ? 9 : 11))
    home.setCharacterSize(home.getCharacterSize() - 1);
  home.setPosition(teamLeft + homeWidth - home.getLocalBounds().width -
                       home.getLocalBounds().left,
                   r.top + (r.height - home.getLocalBounds().height) / 2.f -
                       home.getLocalBounds().top);
  window_.draw(home);

  const float scoreBoxHeight = compact ? 28.f : 36.f;
  const float scoreTop = r.top + (r.height - scoreBoxHeight) / 2.f;
  if (completed) {
    drawRoundedRect(window_, {scoreLeft, scoreTop, scoreWidth, scoreBoxHeight},
                    kPanelRaised, kBorder, 1.f, 7.f);
    drawRoundedRect(window_, {scoreRight, scoreTop, scoreWidth, scoreBoxHeight},
                    kPanelRaised, kBorder, 1.f, 7.f);
    drawText(std::to_string(*m.homeScore), scoreLeft + (compact ? 12.f : 15.f),
             scoreTop + (compact ? 6.f : 9.f), compact ? 13 : 16,
             kAccentLight);
    drawText(std::to_string(*m.awayScore), scoreRight + (compact ? 12.f : 15.f),
             scoreTop + (compact ? 6.f : 9.f), compact ? 13 : 16,
             kAccentLight);
  } else {
    drawRoundedRect(window_, {center - (compact ? 20.f : 24.f),
                              scoreTop + (compact ? 2.f : 4.f),
                              compact ? 40.f : 48.f,
                              compact ? 24.f : 28.f},
                    kAccentWash, kBorder, 1.f, 10.f);
    drawText("VS", center - 8.f, scoreTop + (compact ? 7.f : 10.f),
             compact ? 10 : 12, kAccentLight);
  }

  sf::Text away(teamName(t, m.awayTeamId), font_, compact ? 12 : 15);
  away.setFillColor(kText);
  while (away.getLocalBounds().width > awayWidth &&
         away.getCharacterSize() > (compact ? 9 : 11))
    away.setCharacterSize(away.getCharacterSize() - 1);
  away.setPosition(awayLeft,
                   r.top + (r.height - away.getLocalBounds().height) / 2.f -
                       away.getLocalBounds().top);
  window_.draw(away);

  if (compact)
    return;
  const std::string status = completed ? "COMPLETED" : "UPCOMING";
  const sf::Color statusColor = completed ? kAccentLight : kGold;
  sf::FloatRect statusBadge(r.left + r.width - statusWidth - 12.f,
                            r.top + (r.height - 26.f) / 2.f, statusWidth, 26.f);
  drawRoundedRect(window_, statusBadge,
                  completed ? sf::Color(19, 53, 45) : sf::Color(54, 43, 27),
                  sf::Color::Transparent, 0.f, 8.f);
  drawText(status, statusBadge.left + 12.f, statusBadge.top + 6.f, 10,
           statusColor);
}
void Application::showMessage(std::string s, bool error) {
  message_ = std::move(s);
  messageIsError_ = error;
}
std::string
Application::nextId(const std::string &prefix,
                    const std::vector<std::string> &existing) const {
  unsigned max = 0;
  for (const auto &id : existing)
    if (id.rfind(prefix, 0) == 0) {
      try {
        max = std::max(
            max, static_cast<unsigned>(std::stoul(id.substr(prefix.size()))));
      } catch (...) {
      }
    }
  std::ostringstream out;
  out << prefix << std::setw(5) << std::setfill('0') << max + 1;
  return out.str();
}
void Application::saveOrRollback(std::vector<Tournament> before) {
  try {
    if (!loadError_.empty())
      throw std::runtime_error(
          "Saved data could not be loaded; refusing to overwrite it.");
    repository_.save(tournaments_);
  } catch (const std::exception &e) {
    tournaments_ = std::move(before);
    active_ =
        tournaments_.empty() ? 0 : std::min(active_, tournaments_.size() - 1);
    throw std::runtime_error(
        std::string("Save failed; changes were rolled back: ") + e.what());
  }
}
void Application::beginDialog(Dialog d, std::vector<std::string> labels,
                              std::vector<std::string> fields) {
  dialog_ = d;
  labels_ = std::move(labels);
  fields_ = std::move(fields);
  focusedField_ = 0;
  caretPositions_.clear();
  for (const auto &field : fields_)
    caretPositions_.push_back(field.size());
  caretClock_.restart();
}

void Application::submitDialog() {
  try {
    if (dialog_ == Dialog::ConfirmDeleteTournament ||
        dialog_ == Dialog::ConfirmDeleteTeam ||
        dialog_ == Dialog::ConfirmDeletePlayer) {
      auto before = tournaments_;
      if (dialog_ == Dialog::ConfirmDeleteTournament) {
        if (!activeTournament())
          throw std::logic_error("Select a tournament first.");
        tournaments_.erase(tournaments_.begin() +
                           static_cast<std::ptrdiff_t>(active_));
        active_ = tournaments_.empty()
                      ? 0
                      : std::min(active_, tournaments_.size() - 1);
      } else if (dialog_ == Dialog::ConfirmDeleteTeam) {
        auto *t = activeTournament();
        if (!t || selectedRow_ < 0 ||
            static_cast<size_t>(selectedRow_) >= t->teams.size())
          throw std::logic_error("Select a team first.");
        auto team = t->teams[static_cast<size_t>(selectedRow_)];
        if (!t->matches.empty())
          throw std::logic_error(
              "Teams referenced by fixtures cannot be deleted.");
        if (std::any_of(t->players.begin(), t->players.end(),
                        [&](const Player &p) { return p.teamId == team.id; }))
          throw std::logic_error(
              "Remove this team's players before deleting it.");
        t->teams.erase(t->teams.begin() + selectedRow_);
      } else {
        auto *t = activeTournament();
        if (!t || selectedRow_ < 0 ||
            static_cast<size_t>(selectedRow_) >= t->players.size())
          throw std::logic_error("Select a player first.");
        auto p = t->players[static_cast<size_t>(selectedRow_)];
        for (const auto &m : t->matches)
          for (const auto &s : m.scorers)
            if (s.playerId == p.id)
              throw std::logic_error(
                  "A player credited with match goals cannot be deleted.");
        t->players.erase(t->players.begin() + selectedRow_);
      }
      saveOrRollback(std::move(before));
      showMessage("Record deleted.");
      dialog_ = Dialog::None;
      return;
    }
    auto before = tournaments_;
    if (dialog_ == Dialog::Tournament) {
      if (ValidationService::normalize(fields_[0]).empty() ||
          ValidationService::normalize(fields_[1]).empty())
        throw std::invalid_argument("Tournament name and season are required.");
      for (const auto &other : tournaments_)
        if (ValidationService::normalize(other.name) ==
            ValidationService::normalize(fields_[0]))
          throw std::invalid_argument("Tournament name already exists.");
      std::vector<std::string> ids;
      for (auto &t : tournaments_)
        ids.push_back(t.id);
      Tournament t;
      t.id = nextId("TOUR-", ids);
      t.name = fields_[0];
      t.season = fields_[1];
      if (fields_.size() > 2)
        t.description = fields_[2];
      tournaments_.push_back(std::move(t));
      active_ = tournaments_.size() - 1;
    } else if (dialog_ == Dialog::EditTournament) {
      auto *t = activeTournament();
      if (!t || fields_[0].empty() || fields_[1].empty())
        throw std::invalid_argument("Tournament name and season are required.");
      for (const auto &other : tournaments_)
        if (other.id != t->id && ValidationService::normalize(other.name) ==
                                     ValidationService::normalize(fields_[0]))
          throw std::invalid_argument("Tournament name already exists.");
      t->name = fields_[0];
      t->season = fields_[1];
      t->description = fields_[2];
    } else if (dialog_ == Dialog::Team) {
      auto *t = activeTournament();
      if (!t)
        throw std::logic_error("Create a tournament first.");
      Team team;
      team.name = fields_[0];
      if (fields_.size() > 1)
        team.manager = fields_[1];
      team.id = nextId("TEAM-", [&] {
        std::vector<std::string> v;
        for (auto &x : t->teams)
          v.push_back(x.id);
        return v;
      }());
      ValidationService::addTeam(*t, team);
      t->teams.push_back(std::move(team));
    } else if (dialog_ == Dialog::EditTeam) {
      auto *t = activeTournament();
      if (!t || selectedRow_ < 0 ||
          static_cast<size_t>(selectedRow_) >= t->teams.size())
        throw std::logic_error("Select a team first.");
      if (t->state != TournamentState::Draft || !t->matches.empty())
        throw std::logic_error(
            "Team details are locked after fixtures are generated.");
      Team revised = t->teams[static_cast<size_t>(selectedRow_)];
      revised.name = fields_[0];
      revised.manager = fields_[1];
      revised.contact = fields_[2];
      t->teams.erase(t->teams.begin() + selectedRow_);
      try {
        ValidationService::addTeam(*t, revised);
      } catch (...) {
        t->teams.insert(t->teams.begin() + selectedRow_, std::move(revised));
        throw;
      }
      t->teams.insert(t->teams.begin() + selectedRow_, std::move(revised));
    } else if (dialog_ == Dialog::Player) {
      auto *t = activeTournament();
      if (!t)
        throw std::logic_error("Create a tournament first.");
      if (t->teams.empty())
        throw std::logic_error("Add a team before registering players.");
      const auto teamIndex = parseUnsigned(
          fields_[1], static_cast<unsigned>(t->teams.size()), "Team number");
      if (teamIndex == 0)
        throw std::invalid_argument(
            "Team number must match one shown in the form.");
      const auto jersey = parseUnsigned(fields_[2], 99, "Jersey number");
      if (jersey == 0)
        throw std::invalid_argument("Jersey number must be between 1 and 99.");
      unsigned pos = 0;
      auto ptxt = ValidationService::normalize(fields_[3]);
      if (ptxt == "defender" || ptxt == "def")
        pos = 1;
      else if (ptxt == "midfielder" || ptxt == "mid")
        pos = 2;
      else if (ptxt == "forward" || ptxt == "fwd")
        pos = 3;
      else if (ptxt != "goalkeeper" && ptxt != "gk")
        throw std::invalid_argument(
            "Position must be Goalkeeper, Defender, Midfielder, or Forward.");
      Player p;
      p.id = nextId("PLAYER-", [&] {
        std::vector<std::string> v;
        for (auto &x : t->players)
          v.push_back(x.id);
        return v;
      }());
      p.name = fields_[0];
      p.teamId = t->teams[teamIndex - 1].id;
      p.jersey = jersey;
      p.position = static_cast<Position>(pos);
      ValidationService::addPlayer(*t, p);
      t->players.push_back(std::move(p));
    } else if (dialog_ == Dialog::EditPlayer) {
      auto *t = activeTournament();
      if (!t || selectedRow_ < 0 ||
          static_cast<size_t>(selectedRow_) >= t->players.size())
        throw std::logic_error("Select a player first.");
      if (t->state != TournamentState::Draft || !t->matches.empty())
        throw std::logic_error(
            "Player details are locked after fixtures are generated.");
      const auto teamIndex = parseUnsigned(
          fields_[1], static_cast<unsigned>(t->teams.size()), "Team number");
      if (teamIndex == 0)
        throw std::invalid_argument(
            "Team number must match one shown in the form.");
      const auto jersey = parseUnsigned(fields_[2], 99, "Jersey number");
      if (jersey == 0)
        throw std::invalid_argument("Jersey number must be between 1 and 99.");
      unsigned pos = 0;
      auto ptxt = ValidationService::normalize(fields_[3]);
      if (ptxt == "defender" || ptxt == "def")
        pos = 1;
      else if (ptxt == "midfielder" || ptxt == "mid")
        pos = 2;
      else if (ptxt == "forward" || ptxt == "fwd")
        pos = 3;
      else if (ptxt != "goalkeeper" && ptxt != "gk")
        throw std::invalid_argument(
            "Position must be Goalkeeper, Defender, Midfielder, or Forward.");
      Player revised = t->players[static_cast<size_t>(selectedRow_)];
      revised.name = fields_[0];
      revised.teamId = t->teams[teamIndex - 1].id;
      revised.jersey = jersey;
      revised.position = static_cast<Position>(pos);
      t->players.erase(t->players.begin() + selectedRow_);
      try {
        ValidationService::addPlayer(*t, revised);
      } catch (...) {
        t->players.insert(t->players.begin() + selectedRow_,
                          std::move(revised));
        throw;
      }
      t->players.insert(t->players.begin() + selectedRow_, std::move(revised));
    } else if (dialog_ == Dialog::Score) {
      auto *t = activeTournament();
      if (!t)
        throw std::logic_error("Select a tournament first.");
      const auto h = parseUnsigned(fields_[0], 1000, "Home score"),
                 a = parseUnsigned(fields_[1], 1000, "Away score");
      std::vector<Scorer> scorers;
      std::istringstream list(fields_.size() > 2 ? fields_[2] : "");
      std::string item;
      while (std::getline(list, item, ',')) {
        const auto cleanItem = ValidationService::normalize(item);
        if (cleanItem.empty())
          continue;
        const auto nameSeparator = item.find('=');
        const bool enteredByName = nameSeparator != std::string::npos;
        const auto sep = enteredByName ? nameSeparator : item.find(':');
        if (sep == std::string::npos)
          throw std::invalid_argument(
              "Enter each scorer as Player Name=Goals, separated by commas.");
        auto trim = [](std::string text) {
          const auto first = text.find_first_not_of(" \t\r\n");
          if (first == std::string::npos)
            return std::string{};
          const auto last = text.find_last_not_of(" \t\r\n");
          return text.substr(first, last - first + 1);
        };
        const auto playerKey = trim(item.substr(0, sep));
        const auto goals = parseUnsigned(
            trim(item.substr(sep + 1)), 1000, "Scorer goals");
        if (!enteredByName) {
          scorers.push_back({playerKey, goals});
          continue;
        }
        const auto normalizedName = ValidationService::normalize(playerKey);
        std::vector<const Player *> matches;
        for (const auto &player : t->players)
          if (ValidationService::normalize(player.name) == normalizedName)
            matches.push_back(&player);
        if (matches.empty())
          throw std::invalid_argument("Player name '" + normalizedName +
                                      "' was not found.");
        if (matches.size() > 1)
          throw std::invalid_argument(
              "More than one player has that name; enter their player ID as "
              "PLAYER-ID:GOALS.");
        scorers.push_back({matches.front()->id, goals});
      }
      TournamentManager::recordResult(*t, selectedMatchId_, h, a,
                                      std::move(scorers));
    }
    saveOrRollback(std::move(before));
    showMessage("Saved successfully.");
    dialog_ = Dialog::None;
  } catch (const std::exception &e) {
    showMessage(e.what(), true);
  }
}

void Application::drawSidebar() {
  const auto h = static_cast<float>(window_.getSize().y);
  sf::RectangleShape side({218, h});
  side.setFillColor(kSidebar);
  window_.draw(side);
  sf::RectangleShape edge({1.f, h});
  edge.setPosition(217.f, 0.f);
  edge.setFillColor(kBorder);
  window_.draw(edge);
  drawRoundedRect(window_, {20, 20, 38, 38}, kAccent, sf::Color::Transparent,
                  0.f, 10.f, true);
  drawText("F", 32, 25, 24, sf::Color(8, 26, 22));
  drawText("FTMS", 68, 20, 27, kText);
  drawText("FOOTBALL MANAGER", 68, 50, 9, kMuted);
  drawRoundedRect(window_, {16, 76, 186, 22}, kAccentWash, kBorder, 1.f, 8.f);
  drawText(refereeRole_ + "  /  " + refereeId_, 26, 81, 10, kAccentLight);
  const std::vector<std::tuple<Page, std::string, std::string>> nav = {
      {Page::Dashboard, "DB", "Dashboard"},
      {Page::Tournaments, "TM", "Tournaments"},
      {Page::Teams, "TE", "Teams"},
      {Page::Players, "PL", "Players"},
      {Page::Fixtures, "FX", "Fixtures"},
      {Page::Matches, "SC", "Matches & scores"},
      {Page::Standings, "PT", "Points table"},
      {Page::Statistics, "ST", "Player statistics"},
      {Page::History, "HI", "History"},
      {Page::Reports, "RP", "Reports"},
      {Page::About, "AB", "Settings / about"}};
  float y = 104;
  drawText("WORKSPACE", 24, 108, 9, kMuted);
  y = 126;
  for (const auto &[p, icon, title] : nav) {
    sf::FloatRect r(12, y, 194, 37);
    if (p == page_) {
      drawRoundedRect(window_, r, kAccentWash, kBorder, 1.f, 9.f);
      sf::RectangleShape marker({3.f, 21.f});
      marker.setPosition(12.f, y + 8.f);
      marker.setFillColor(kAccentLight);
      window_.draw(marker);
    }
    drawRoundedRect(window_, {23, y + 6, 27, 25},
                    p == page_ ? kAccent : kPanelRaised, sf::Color::Transparent,
                    0.f, 7.f);
    drawText(icon, 28, y + 12, 9, p == page_ ? sf::Color(8, 26, 22) : kMuted);
    drawText(title, 60, y + 9, 13, p == page_ ? kAccentLight : kText);
    y += 43;
  }
  auto *t = activeTournament();
  const float cardY = std::max(y + 8.f, h - 138.f);
  drawRoundedRect(window_, {12, cardY, 194, 112}, kPanel, kBorder, 1.f, 11.f,
                  true);
  drawText("ACTIVE TOURNAMENT", 24, cardY + 14, 9, kMuted);
  drawText(t ? t->name : "No tournament yet", 24, cardY + 39, 14,
           t ? kText : kMuted);
  drawRoundedRect(window_, {22, cardY + 69, 88, 24},
                  t && t->state == TournamentState::Active ? kAccentWash
                                                           : kPanelRaised,
                  kBorder, 1.f, 8.f);
  drawText(t ? stateName(t->state) : "Get started", 32, cardY + 75, 10,
           t && t->state == TournamentState::Active ? kAccentLight : kMuted);
}

void Application::drawPage() {
  const auto width = static_cast<float>(window_.getSize().x),
             height = static_cast<float>(window_.getSize().y);
  const float left = 250, content = width - left - 30;
  auto *t = activeTournament();
  std::string title = "Dashboard", subtitle = "Tournament workspace";
  switch (page_) {
  case Page::Dashboard:
    break;
  case Page::Tournaments:
    title = "Tournaments";
    subtitle = "Create, select, start or close a season";
    break;
  case Page::Teams:
    title = "Teams";
    subtitle = "Register and review participating teams";
    break;
  case Page::Players:
    title = "Players";
    subtitle = "Squad registration and jersey details";
    break;
  case Page::Fixtures:
    title = "Fixtures";
    subtitle = "Single round-robin schedule";
    break;
  case Page::Matches:
    title = "Matches & scores";
    subtitle = "Select a fixture to enter or correct its result";
    break;
  case Page::Standings:
    title = "Points table";
    subtitle = "Derived from completed match results";
    break;
  case Page::Statistics:
    title = "Player statistics";
    subtitle = "Goals are counted only when explicitly recorded";
    break;
  case Page::History:
    title = "Tournament history";
    subtitle = "Select a saved season to inspect its records";
    break;
  case Page::Reports:
    title = "Reports";
    subtitle = "Export a CSV summary from saved tournament records";
    break;
  case Page::About:
    title = "Settings & about";
    subtitle = "Local storage, rules and tie-break policy";
    break;
  }
  drawText(title, left, 24, 29, kText);
  drawText(subtitle, left, 67, 13, kMuted);
  sf::RectangleShape divider({content, 1.f});
  divider.setPosition(left, 96.f);
  divider.setFillColor(kBorder);
  window_.draw(divider);
  if (auto *current = activeTournament()) {
    drawRoundedRect(window_, {width - 210.f, 29.f, 178.f, 34.f}, kPanel,
                    kBorder, 1.f, 10.f);
    drawText(current->season + "  /  " + stateName(current->state),
             width - 195.f, 39.f, 11,
             current->state == TournamentState::Active ? kAccentLight : kMuted);
  }
  if (!loadError_.empty()) {
    drawCard({left, 97, content, 42}, sf::Color(67, 36, 40));
    drawText("Data load warning: " + loadError_, left + 12, 108, 13);
  }
  auto button = [&](const std::string &s, float x, float y, float w,
                    bool enabled = true, bool accent = false) {
    drawButton(s, {x, y, w, 38}, enabled, accent);
  };
  const float top = 112;
  if (page_ == Page::Dashboard) {
    if (!t) {
      drawCard({left, top, content, 300}, kPanel);
      drawText("YOUR TOURNAMENT, AT A GLANCE", left + 28, top + 30, 11,
               kAccentLight);
      drawText("Welcome to FTMS", left + 28, top + 62, 30, kText);
      drawText("Set up your season, register the clubs and let FTMS build the "
               "match schedule.",
               left + 28, top + 111, 15, kMuted);
      button("Create tournament", left + 26, top + 132, 190, true, true);
    } else {
      const auto complete = static_cast<unsigned>(std::count_if(
          t->matches.begin(), t->matches.end(),
          [](const Match &m) { return m.state == MatchState::Completed; }));
      const auto pending = static_cast<unsigned>(t->matches.size()) - complete;
      const auto stats = StatisticsManager::goals(*t);
      const auto table = StandingsCalculator::calculate(*t);
      const float gap = 14, cardW = (content - 3 * gap) / 4;
      std::array<std::pair<std::string, std::string>, 4> cards = {
          {{"REGISTERED TEAMS", std::to_string(t->teams.size())},
           {"REGISTERED PLAYERS", std::to_string(t->players.size())},
           {"MATCHES PLAYED", std::to_string(complete)},
           {"UPCOMING FIXTURES", std::to_string(pending)}}};
      drawCard({left, top, content, 102}, kPanel);
      drawText("SEASON OVERVIEW", left + 20, top + 15, 10, kAccentLight);
      drawText(t->name, left + 20, top + 38, 22, kText);
      drawText("Season " + t->season + "   |   " + stateName(t->state) +
                   "   |   " + std::to_string(t->matches.size()) +
                   " total matches",
               left + 22, top + 72, 12, kMuted);
      drawRoundedRect(window_, {left + content - 118, top + 32, 96, 30},
                      t->state == TournamentState::Active ? kAccentWash
                                                          : kPanelRaised,
                      kBorder, 1.f, 9.f);
      drawText(stateName(t->state), left + content - 105, top + 41, 11,
               t->state == TournamentState::Active ? kAccentLight : kMuted);
      const float statsTop = top + 116;
      for (size_t i = 0; i < cards.size(); ++i) {
        float x = left + i * (cardW + gap);
        drawCard({x, statsTop, cardW, 94}, kPanel);
        sf::RectangleShape accentLine({3.f, 54.f});
        accentLine.setPosition(x + 15.f, statsTop + 20.f);
        accentLine.setFillColor(i == 3 ? kGold : kAccent);
        window_.draw(accentLine);
        drawText(cards[i].first, x + 29, statsTop + 17, 10, kMuted);
        drawText(cards[i].second, x + 29, statsTop + 41, 24,
                 i == 3 && pending ? kGold : kText);
      }
      const float panelsTop = top + 228;
      const float panelW = (content - 18.f) / 2.f;
      drawCard({left, panelsTop, panelW, 282}, kPanel);
      drawText("UPCOMING FIXTURES", left + 20, panelsTop + 19, 11,
               kAccentLight);
      drawText("Next on the schedule", left + 20, panelsTop + 40, 18, kText);
      float y = panelsTop + 75;
      unsigned count = 0;
      for (const auto &m : t->matches)
        if (m.state == MatchState::Scheduled && count++ < 4) {
          drawMatchCard(*t, m, {left + 14.f, y, panelW - 28.f, 42.f});
          y += 48;
        }
      if (count == 0)
        drawText("No pending fixtures.", left + 22, y + 5, 13, kMuted);
      float rx = left + panelW + 18.f;
      drawCard({rx, panelsTop, panelW, 282}, kPanel);
      drawText("LEAGUE TABLE", rx + 20, panelsTop + 19, 11, kAccentLight);
      drawText("Current leaders", rx + 20, panelsTop + 40, 18, kText);
      for (size_t i = 0; i < table.size() && i < 4; ++i) {
        const float rowY = panelsTop + 76.f + 42.f * i;
        drawRoundedRect(window_, {rx + 14, rowY, panelW - 28, 34},
                        i == 0 ? kAccentWash : kPanelRaised, kBorder, 1.f, 8.f);
        drawText(std::to_string(i + 1), rx + 27, rowY + 9, 12,
                 i == 0 ? kGold : kMuted);
        drawText(table[i].teamName, rx + 58, rowY + 9, 12, kText);
        drawText(std::to_string(table[i].points) + " PTS", rx + panelW - 95,
                 rowY + 9, 10, i == 0 ? kAccentLight : kMuted);
      }
      if (!stats.empty() && stats.front().goals > 0) {
        std::string leaders;
        for (const auto &player : stats) {
          if (player.goals != stats.front().goals)
            break;
          if (!leaders.empty())
            leaders += ", ";
          leaders += player.playerName;
        }
        drawText("Top scorer: " + leaders + " (" +
                     std::to_string(stats.front().goals) + ")",
                 rx + 20, panelsTop + 260, 12, kGold);
      } else
        drawText("No goalscorer records yet.", rx + 20, panelsTop + 260, 12,
                 kMuted);
      auto recent = std::find_if(
          t->matches.rbegin(), t->matches.rend(),
          [](const Match &m) { return m.state == MatchState::Completed; });
      drawCard({left, panelsTop + 296, content, 64}, kPanelRaised);
      drawText("LAST RESULT", left + 18, panelsTop + 307, 9, kMuted);
      drawText(recent == t->matches.rend() ? "No completed matches yet"
                                           : matchLabel(*t, *recent),
               left + 18, panelsTop + 326, 13, kText);
      drawText("Season " + t->season + "  |  " + std::to_string(pending) +
                   " fixtures to play",
               left + content - 260, panelsTop + 327, 11, kMuted);
    }
  } else if (page_ == Page::Tournaments || page_ == Page::History) {
    if (page_ == Page::Tournaments) {
      button("New tournament", left, top, 164, true, true);
      button("Start tournament", left + 174, top, 168,
             t && t->state == TournamentState::Draft && t->teams.size() >= 2,
             true);
      button("Complete", left + 352, top, 124,
             t && t->state == TournamentState::Active && !t->matches.empty());
      button("Reopen", left + 486, top, 100,
             t && t->state == TournamentState::Completed);
      button("Edit", left + 596, top, 90,
             t && t->state == TournamentState::Draft);
      button("Delete", left + 696, top, 90, t != nullptr);
    } else
      drawText("Completed seasons are kept in the list below.", left, top + 7,
               14, kMuted);
    float y = top + 55 - static_cast<float>(scroll_);
    for (size_t i = 0; i < tournaments_.size(); ++i) {
      auto &item = tournaments_[i];
      if (page_ == Page::History && item.state != TournamentState::Completed)
        continue;
      if (!search_.empty() &&
          ValidationService::normalize(item.name + " " + item.season)
                  .find(ValidationService::normalize(search_)) ==
              std::string::npos)
        continue;
      sf::FloatRect r(left, y, content, 68);
      drawCard(r, i == active_ ? kAccentWash : kPanel);
      drawText(item.name + "  /  " + item.season, left + 14, y + 6, 15);
      drawText(stateName(item.state) + "   |   " +
                   std::to_string(item.teams.size()) + " teams   |   " +
                   std::to_string(item.matches.size()) + " matches",
               left + 14, y + 29, 12, kMuted);
      if (page_ == Page::History) {
        auto table = StandingsCalculator::calculate(item);
        std::string champion =
            table.empty()
                ? "Champion: undetermined"
                : (table.size() > 1 && table[0].points == table[1].points &&
                           table[0].goalDifference == table[1].goalDifference &&
                           table[0].goalsFor == table[1].goalsFor
                       ? "Champion: unresolved tie"
                       : "Champion: " + table.front().teamName);
        drawText(champion, left + 14, y + 49, 11, kAccentLight);
      }
      y += 76;
    }
  } else if (page_ == Page::Teams) {
    button("Add team", left, top, 115,
           t && t->state == TournamentState::Draft && t->matches.empty(), true);
    button("Edit selected", left + 126, top, 140,
           t && t->state == TournamentState::Draft && t->matches.empty() &&
               selectedRow_ >= 0);
    button("Delete selected", left + 276, top, 160,
           t && t->state == TournamentState::Draft && t->matches.empty() &&
               selectedRow_ >= 0);
    if (!t) {
      drawText("Select or create a tournament first.", left, top + 66, 15,
               kMuted);
    } else {
      float y = top + 57 - static_cast<float>(scroll_);
      for (size_t i = 0; i < t->teams.size(); ++i) {
        auto &x = t->teams[i];
        if (!search_.empty() &&
            ValidationService::normalize(x.id + " " + x.name + " " + x.manager)
                    .find(ValidationService::normalize(search_)) ==
                std::string::npos)
          continue;
        sf::FloatRect r(left, y, content, 55);
        drawCard(r, static_cast<int>(i) == selectedRow_ ? kAccentWash : kPanel);
        drawText(x.name, left + 14, y + 7, 15);
        drawText(x.id + "    " +
                     (x.manager.empty() ? "No manager recorded" : x.manager),
                 left + 14, y + 31, 12, kMuted);
        y += 62;
      }
    }
  } else if (page_ == Page::Players) {
    button("Register player", left, top, 150,
           t && t->state == TournamentState::Draft && t->matches.empty() &&
               !t->teams.empty(),
           true);
    button("Edit selected", left + 160, top, 140,
           t && t->state == TournamentState::Draft && t->matches.empty() &&
               selectedRow_ >= 0);
    button("Delete selected", left + 310, top, 160,
           t && t->state == TournamentState::Draft && t->matches.empty() &&
               selectedRow_ >= 0);
    if (t) {
      float y = top + 57 - static_cast<float>(scroll_);
      for (size_t i = 0; i < t->players.size(); ++i) {
        auto &p = t->players[i];
        if (!search_.empty() &&
            ValidationService::normalize(p.id + " " + p.name + " " +
                                         teamName(*t, p.teamId))
                    .find(ValidationService::normalize(search_)) ==
                std::string::npos)
          continue;
        sf::FloatRect r(left, y, content, 55);
        drawCard(r, static_cast<int>(i) == selectedRow_ ? kAccentWash : kPanel);
        drawText("#" + std::to_string(p.jersey) + "  " + p.name, left + 14,
                 y + 7, 15);
        drawText(p.id + "    " + teamName(*t, p.teamId) + "    " +
                     positionName(p.position),
                 left + 14, y + 31, 12, kMuted);
        y += 62;
      }
    }
  } else if (page_ == Page::Fixtures || page_ == Page::Matches) {
    if (page_ == Page::Fixtures)
      button("Generate & start fixtures", left, top, 230,
             t && t->state == TournamentState::Draft && t->teams.size() >= 2,
             true);
    if (page_ == Page::Fixtures && t && t->state == TournamentState::Draft)
      drawText(t->teams.size() < 2
                   ? "Register at least two teams to generate the schedule."
                   : "Round robin  |  " + std::to_string(t->teams.size()) +
                         " teams  |  " +
                         std::to_string(t->teams.size() *
                                        (t->teams.size() - 1) / 2) +
                         " fixtures",
               left + 248, top + 12, 12, kMuted);
    if (!t)
      drawText("Select or create a tournament first.", left, top + 60, 15,
               kMuted);
    else {
      float y = top + (page_ == Page::Fixtures ? 56 : 4) -
                static_cast<float>(scroll_);
      for (size_t i = 0; i < t->matches.size(); ++i) {
        auto &m = t->matches[i];
        if (!search_.empty() &&
            ValidationService::normalize(m.id + " " +
                                         teamName(*t, m.homeTeamId) + " " +
                                         teamName(*t, m.awayTeamId))
                    .find(ValidationService::normalize(search_)) ==
                std::string::npos)
          continue;
        sf::FloatRect r(left, y, content, 72);
        drawMatchCard(*t, m, r, static_cast<int>(i) == selectedRow_);
        drawText(m.id + (m.dateTime.empty() ? "" : "   |   " + m.dateTime),
                 left + 76, y + 53, 9, kMuted);
        y += 80;
      }
    }
  } else if (page_ == Page::Standings) {
    if (t) {
      auto rows = StandingsCalculator::calculate(*t);
      drawCard({left, top, content, 44}, kPanelRaised);
      constexpr float posWidth = 54.f, statWidth = 66.f;
      const float statsLeft = left + content - 8.f * statWidth;
      const float teamLeft = left + posWidth;
      drawText("POS", left + 16, top + 15, 10, kMuted);
      drawText("CLUB", teamLeft + 8, top + 15, 10, kMuted);
      const std::array<std::string, 8> headers = {"P",  "W",  "D",  "L",
                                                  "GF", "GA", "GD", "PTS"};
      for (std::size_t c = 0; c < headers.size(); ++c)
        drawText(headers[c], statsLeft + c * statWidth + 14, top + 15, 10,
                 c == 7 ? kAccentLight : kMuted);
      float y = top + 54 - static_cast<float>(scroll_);
      for (size_t i = 0; i < rows.size(); ++i) {
        auto &s = rows[i];
        if (!search_.empty() && ValidationService::normalize(s.teamName)
                                        .find(ValidationService::normalize(
                                            search_)) == std::string::npos)
          continue;
        drawCard({left, y, content, 44},
                 i == 0 && s.played ? kAccentWash : kPanel);
        if (i == 0 && s.played) {
          sf::RectangleShape marker({3.f, 24.f});
          marker.setPosition(left + 1.f, y + 10.f);
          marker.setFillColor(kGold);
          window_.draw(marker);
        }
        drawText(std::to_string(i + 1), left + 17, y + 15, 12,
                 i == 0 && s.played ? kGold : kMuted);
        drawText(s.teamName, teamLeft + 8, y + 15, 12, kText);
        const std::array<std::string, 8> values = {
            std::to_string(s.played),         std::to_string(s.won),
            std::to_string(s.drawn),          std::to_string(s.lost),
            std::to_string(s.goalsFor),       std::to_string(s.goalsAgainst),
            std::to_string(s.goalDifference), std::to_string(s.points)};
        for (std::size_t c = 0; c < values.size(); ++c)
          drawText(values[c], statsLeft + c * statWidth + 14, y + 15, 12,
                   c == 7 ? kAccentLight : kText);
        y += 50;
      }
      drawText("Tie policy: points, goal difference, goals scored. Equal "
               "leaders remain unresolved; team ID is display order only.",
               left,
               top + std::min(500.f, static_cast<float>(rows.size() * 47 + 65)),
               12, kMuted);
    }
  } else if (page_ == Page::Statistics) {
    if (t) {
      auto stats = StatisticsManager::goals(*t);
      drawText("PLAYER ID       PLAYER                         TEAM            "
               "          #       GOALS",
               left, top + 10, 13, kMuted);
      float y = top + 40 - static_cast<float>(scroll_);
      size_t shown = 0;
      for (const auto &p : stats) {
        if (!search_.empty() &&
            ValidationService::normalize(p.playerId + " " + p.playerName + " " +
                                         p.teamName + " " +
                                         std::to_string(p.jersey))
                    .find(ValidationService::normalize(search_)) ==
                std::string::npos)
          continue;
        drawCard({left, y, content, 45}, kPanel);
        std::ostringstream line;
        line << std::left << std::setw(16) << p.playerId << std::setw(30)
             << p.playerName << std::setw(26) << p.teamName << std::setw(7)
             << p.jersey << p.goals;
        drawText(line.str(), left + 12, y + 13, 13);
        y += 50;
        ++shown;
      }
      if (stats.empty())
        drawText("No player scorer records yet.", left, top + 60, 14, kMuted);
      else if (shown == 0)
        drawText("No players match this search.", left, top + 60, 14, kMuted);
    }
  } else if (page_ == Page::Reports) {
    drawCard({left, top, content, 130});
    drawText("Tournament summary export", left + 18, top + 18, 18);
    drawText(
        "Creates a CSV standings and match report for the selected tournament.",
        left + 18, top + 52, 14, kMuted);
    button("Export CSV report", left + 18, top + 82, 180, t != nullptr, true);
  } else if (page_ == Page::About) {
    drawCard({left, top, content, 340});
    drawText("Football Tournament Management System", left + 20, top + 20, 20);
    drawText("C++20  /  SFML 2.6.2  /  local versioned file storage", left + 20,
             top + 57, 15, kMuted);
    drawText(
        "Data file: data/tournaments.ftms (relative to the launch directory)",
        left + 20, top + 92, 14, kMuted);
    drawText("Standings: 3 points per win, 1 per draw. Sorted by points, goal "
             "difference, then goals scored.",
             left + 20, top + 130, 14, kMuted);
    drawText("A stable team ID sorts display order only. A tied leader cannot "
             "be finalized as champion.",
             left + 20, top + 162, 14, kMuted);
    drawText("Keyboard: Escape closes dialogs; Tab changes form field; Enter "
             "submits.",
             left + 20, top + 206, 14, kMuted);
    drawText(fontLoaded_ ? "Bundled font loaded."
                         : "Font missing: add DejaVuSans.ttf under "
                           "assets/fonts/ to display text.",
             left + 20, top + 254, 14, fontLoaded_ ? kAccentLight : kGold);
  }
  if (!message_.empty()) {
    sf::Color c = messageIsError_ ? sf::Color(67, 36, 40) : kAccentWash;
    drawCard({left, height - 55, content, 38}, c);
    drawText(message_, left + 12, height - 45, 13);
  }
}

void Application::drawDialog() {
  if (dialog_ == Dialog::None)
    return;
  const float w = static_cast<float>(window_.getSize().x),
              h = static_cast<float>(window_.getSize().y),
              boxW = std::min(660.f, w - 60),
              boxH = std::min(560.f,
                              168.f + static_cast<float>(fields_.size()) * 66);
  sf::RectangleShape shade({w, h});
  shade.setFillColor(sf::Color(0, 0, 0, 165));
  window_.draw(shade);
  float x = (w - boxW) / 2, y = (h - boxH) / 2;
  drawCard({x, y, boxW, boxH}, kPanel);
  std::string title = dialog_ == Dialog::Tournament       ? "Create tournament"
                      : dialog_ == Dialog::EditTournament ? "Edit tournament"
                      : dialog_ == Dialog::Team           ? "Add team"
                      : dialog_ == Dialog::EditTeam       ? "Edit team"
                      : dialog_ == Dialog::Player         ? "Register player"
                      : dialog_ == Dialog::EditPlayer     ? "Edit player"
                      : dialog_ == Dialog::Score ? "Enter or correct result"
                                                 : "Confirm deletion";
  drawText(title, x + 26, y + 22, 22, kText);
  drawText("Complete each field, then save your changes.", x + 26, y + 54, 12,
           kMuted);
  if (dialog_ == Dialog::ConfirmDeleteTeam ||
      dialog_ == Dialog::ConfirmDeletePlayer ||
      dialog_ == Dialog::ConfirmDeleteTournament) {
    drawText("This action cannot be undone. Continue?", x + 22, y + 74, 15,
             kMuted);
    drawButton("Cancel", {x + boxW - 230, y + boxH - 58, 98, 38});
    drawButton("Delete", {x + boxW - 120, y + boxH - 58, 98, 38}, true, true);
    return;
  }
  float fy = y + 78;
  const bool cursorOn =
      caretClock_.getElapsedTime().asMilliseconds() % 1000 < 560;
  for (size_t i = 0; i < fields_.size(); ++i) {
    const bool focused = i == focusedField_;
    drawText(labels_[i], x + 26, fy, 11, focused ? kAccentLight : kMuted);
    sf::FloatRect r(x + 22, fy + 19, boxW - 44, 40);
    drawRoundedRect(window_, r, focused ? sf::Color(235, 240, 245) : kInput,
                    focused ? kAccent : kBorder, focused ? 1.5f : 1.f, 8.f);

    const std::string &value = fields_[i];
    const std::size_t caret = i < caretPositions_.size()
                                  ? std::min(caretPositions_[i], value.size())
                                  : value.size();
    const float available = r.width - 24.f;
    std::size_t first = 0;
    auto textWidth = [&](std::size_t begin, std::size_t end) {
      sf::Text measure(value.substr(begin, end - begin), font_, 13);
      return measure.getLocalBounds().width;
    };
    while (first < caret && textWidth(first, caret) > available - 18.f)
      ++first;
    std::size_t last = caret;
    while (last < value.size() && textWidth(first, last + 1) <= available)
      ++last;
    while (last < value.size() && textWidth(first, last + 1) <= available)
      ++last;
    drawText(value.empty() ? "Enter " + labels_[i]
                           : value.substr(first, last - first),
             r.left + 12, r.top + 11, 13,
             value.empty() ? sf::Color(92, 105, 121) : sf::Color(18, 27, 40));
    if (focused && cursorOn && !value.empty()) {
      sf::Text prefix(value.substr(first, caret - first), font_, 13);
      const float cursorX = r.left + 12.f + prefix.getLocalBounds().width;
      sf::RectangleShape caretBar({2.f, 19.f});
      caretBar.setPosition(cursorX, r.top + 10.f);
      caretBar.setFillColor(sf::Color::Black);
      window_.draw(caretBar);
    } else if (focused && cursorOn) {
      sf::RectangleShape caretBar({2.f, 19.f});
      caretBar.setPosition(r.left + 12.f, r.top + 10.f);
      caretBar.setFillColor(sf::Color::Black);
      window_.draw(caretBar);
    }
    fy += 66;
  }
  if (dialog_ == Dialog::Player || dialog_ == Dialog::EditPlayer) {
    auto *t = activeTournament();
    if (t) {
      std::string teams = "Teams: ";
      for (size_t i = 0; i < t->teams.size(); ++i)
        teams += std::to_string(i + 1) + "=" + t->teams[i].name + "  ";
      drawText(teams, x + 26, fy - 2, 10, kMuted);
    }
  }
  if (dialog_ == Dialog::Score) {
    auto *t = activeTournament();
    if (t) {
      auto match = std::find_if(t->matches.begin(), t->matches.end(),
                                [&](const Match &m) {
                                  return m.id == selectedMatchId_;
                                });
      std::string hint = "Type player name=goals, e.g. Messi=2. Players: ";
      if (match != t->matches.end()) {
        bool first = true;
        for (const auto &player : t->players) {
          if (player.teamId != match->homeTeamId &&
              player.teamId != match->awayTeamId)
            continue;
          if (!first)
            hint += ", ";
          hint += player.name;
          first = false;
        }
        if (first)
          hint += "none registered for these teams";
      }
      if (hint.size() > 112)
        hint = hint.substr(0, 109) + "...";
      drawText(hint, x + 26, fy - 2, 10, kMuted);
    }
  }
  drawText("TAB  next field      ENTER  save      ESC  cancel", x + 26,
           y + boxH - 42, 10, kMuted);
  drawButton("Cancel", {x + boxW - 230, y + boxH - 58, 98, 38});
  drawButton("Save changes", {x + boxW - 120, y + boxH - 58, 98, 38}, true,
             true);
}

void Application::handleClick(sf::Vector2f p) {
  if (dialog_ != Dialog::None) {
    const float w = static_cast<float>(window_.getSize().x),
                h = static_cast<float>(window_.getSize().y),
                bw = std::min(660.f, w - 60),
                bh = std::min(560.f,
                              168.f + static_cast<float>(fields_.size()) * 66),
                x = (w - bw) / 2, y = (h - bh) / 2;
    if (sf::FloatRect(x + bw - 230, y + bh - 60, 100, 44).contains(p)) {
      dialog_ = Dialog::None;
      return;
    }
    if (sf::FloatRect(x + bw - 120, y + bh - 60, 100, 44).contains(p)) {
      if (dialog_ == Dialog::ConfirmDeleteTeam ||
          dialog_ == Dialog::ConfirmDeletePlayer ||
          dialog_ == Dialog::ConfirmDeleteTournament) {
        submitDialog();
      } else
        submitDialog();
      return;
    }
    if (dialog_ != Dialog::ConfirmDeleteTeam &&
        dialog_ != Dialog::ConfirmDeletePlayer &&
        dialog_ != Dialog::ConfirmDeleteTournament) {
      float fy = y + 78;
      for (size_t i = 0; i < fields_.size(); ++i) {
        const sf::FloatRect fieldBounds(x + 22, fy + 19, bw - 44, 40);
        if (fieldBounds.contains(p)) {
          focusedField_ = i;
          if (caretPositions_.size() < fields_.size())
            caretPositions_.resize(fields_.size());
          sf::Text measure;
          measure.setFont(font_);
          measure.setCharacterSize(13);
          const float targetX = p.x - (fieldBounds.left + 12.f);
          float closest = 100000.f;
          std::size_t nearest = 0;
          for (std::size_t at = 0; at <= fields_[i].size(); ++at) {
            measure.setString(fields_[i].substr(0, at));
            const float xAt = measure.getLocalBounds().width;
            if (std::abs(xAt - targetX) < closest) {
              closest = std::abs(xAt - targetX);
              nearest = at;
            }
          }
          caretPositions_[i] = nearest;
          caretClock_.restart();
          return;
        }
        fy += 66;
      }
    }
    return;
  }
  const std::vector<Page> navPages = {
      Page::Dashboard, Page::Tournaments, Page::Teams,     Page::Players,
      Page::Fixtures,  Page::Matches,     Page::Standings, Page::Statistics,
      Page::History,   Page::Reports,     Page::About};
  if (p.x < 218.f && p.y >= 126.f) {
    const auto index = static_cast<size_t>((p.y - 126.f) / 43.f);
    if (index < navPages.size()) {
      page_ = navPages[index];
      selectedRow_ = -1;
      scroll_ = 0;
      return;
    }
  }
  if (page_ == Page::Dashboard) {
    const float left = 250;
    if (!activeTournament() &&
        sf::FloatRect(left + 26, 244, 210, 48).contains(p)) {
      beginDialog(Dialog::Tournament,
                  {"Tournament name", "Season / year", "Description"},
                  {"", "", ""});
      return;
    }
  }
  const float left = 250, top = 112;
  auto *t = activeTournament();
  if (page_ == Page::Tournaments || page_ == Page::History) {
    if (page_ == Page::Tournaments &&
        sf::FloatRect(left, top, 164, 38).contains(p)) {
      beginDialog(Dialog::Tournament,
                  {"Tournament name", "Season / year", "Description"},
                  {"", "", ""});
      return;
    }
    if (page_ == Page::Tournaments && t &&
        sf::FloatRect(left + 174, top, 168, 38).contains(p) &&
        t->state == TournamentState::Draft && t->teams.size() >= 2) {
      auto before = tournaments_;
      try {
        TournamentManager::start(*t);
        saveOrRollback(std::move(before));
        showMessage("Fixtures generated and tournament started.");
      } catch (const std::exception &e) {
        showMessage(e.what(), true);
      }
      return;
    }
    if (page_ == Page::Tournaments && t &&
        sf::FloatRect(left + 352, top, 124, 38).contains(p) &&
        t->state == TournamentState::Active) {
      auto before = tournaments_;
      try {
        TournamentManager::finalize(*t);
        saveOrRollback(std::move(before));
        showMessage("Tournament completed; final standings saved.");
      } catch (const std::exception &e) {
        showMessage(e.what(), true);
      }
      return;
    }
    if (page_ == Page::Tournaments && t &&
        sf::FloatRect(left + 486, top, 100, 38).contains(p) &&
        t->state == TournamentState::Completed) {
      auto before = tournaments_;
      try {
        TournamentManager::reopen(*t);
        saveOrRollback(std::move(before));
        showMessage("Tournament reopened for corrections.");
      } catch (const std::exception &e) {
        showMessage(e.what(), true);
      }
      return;
    }
    if (page_ == Page::Tournaments && t &&
        sf::FloatRect(left + 596, top, 90, 38).contains(p) &&
        t->state == TournamentState::Draft) {
      beginDialog(Dialog::EditTournament,
                  {"Tournament name", "Season / year", "Description"},
                  {t->name, t->season, t->description});
      return;
    }
    if (page_ == Page::Tournaments && t &&
        sf::FloatRect(left + 696, top, 90, 38).contains(p)) {
      beginDialog(Dialog::ConfirmDeleteTournament, {}, {});
      return;
    }
    float y = top + 55 - static_cast<float>(scroll_);
    for (size_t i = 0; i < tournaments_.size(); ++i) {
      auto &item = tournaments_[i];
      if (page_ == Page::History && item.state != TournamentState::Completed)
        continue;
      if (!search_.empty() &&
          ValidationService::normalize(item.name + " " + item.season)
                  .find(ValidationService::normalize(search_)) ==
              std::string::npos)
        continue;
      if (sf::FloatRect(left, y,
                        static_cast<float>(window_.getSize().x) - left - 30, 68)
              .contains(p)) {
        active_ = i;
        selectedRow_ = -1;
        return;
      }
      y += 76;
    }
  } else if (page_ == Page::Teams && t) {
    if (sf::FloatRect(left, top, 115, 38).contains(p)) {
      beginDialog(Dialog::Team,
                  {"Team name", "Manager (optional)", "Contact (optional)"},
                  {"", "", ""});
      return;
    }
    if (sf::FloatRect(left + 126, top, 140, 38).contains(p) &&
        selectedRow_ >= 0) {
      auto &x = t->teams[static_cast<size_t>(selectedRow_)];
      beginDialog(Dialog::EditTeam,
                  {"Team name", "Manager (optional)", "Contact (optional)"},
                  {x.name, x.manager, x.contact});
      return;
    }
    if (sf::FloatRect(left + 276, top, 160, 38).contains(p) &&
        selectedRow_ >= 0) {
      beginDialog(Dialog::ConfirmDeleteTeam, {}, {});
      return;
    }
    float y = top + 57 - static_cast<float>(scroll_);
    for (size_t i = 0; i < t->teams.size(); ++i) {
      auto &x = t->teams[i];
      if (!search_.empty() &&
          ValidationService::normalize(x.id + " " + x.name + " " + x.manager)
                  .find(ValidationService::normalize(search_)) ==
              std::string::npos)
        continue;
      if (sf::FloatRect(left, y,
                        static_cast<float>(window_.getSize().x) - left - 30, 55)
              .contains(p)) {
        selectedRow_ = static_cast<int>(i);
        return;
      }
      y += 62;
    }
  } else if (page_ == Page::Players && t) {
    if (sf::FloatRect(left, top, 150, 38).contains(p)) {
      beginDialog(Dialog::Player,
                  {"Player name", "Team number", "Jersey number", "Position"},
                  {"", "1", "", "Forward"});
      return;
    }
    if (sf::FloatRect(left + 160, top, 140, 38).contains(p) &&
        selectedRow_ >= 0) {
      auto &pl = t->players[static_cast<size_t>(selectedRow_)];
      auto ti = std::find_if(t->teams.begin(), t->teams.end(),
                             [&](const Team &x) { return x.id == pl.teamId; });
      std::string teamNo =
          ti == t->teams.end()
              ? "1"
              : std::to_string(std::distance(t->teams.begin(), ti) + 1);
      beginDialog(Dialog::EditPlayer,
                  {"Player name", "Team number", "Jersey number", "Position"},
                  {pl.name, teamNo, std::to_string(pl.jersey),
                   positionName(pl.position)});
      return;
    }
    if (sf::FloatRect(left + 310, top, 160, 38).contains(p) &&
        selectedRow_ >= 0) {
      beginDialog(Dialog::ConfirmDeletePlayer, {}, {});
      return;
    }
    float y = top + 57 - static_cast<float>(scroll_);
    for (size_t i = 0; i < t->players.size(); ++i) {
      auto &pl = t->players[i];
      if (!search_.empty() &&
          ValidationService::normalize(pl.id + " " + pl.name + " " +
                                       teamName(*t, pl.teamId))
                  .find(ValidationService::normalize(search_)) ==
              std::string::npos)
        continue;
      if (sf::FloatRect(left, y,
                        static_cast<float>(window_.getSize().x) - left - 30, 55)
              .contains(p)) {
        selectedRow_ = static_cast<int>(i);
        return;
      }
      y += 62;
    }
  } else if (page_ == Page::Fixtures && t &&
             sf::FloatRect(left, top, 230, 38).contains(p) &&
             t->state == TournamentState::Draft && t->teams.size() >= 2) {
    auto before = tournaments_;
    try {
      TournamentManager::start(*t);
      saveOrRollback(std::move(before));
      showMessage("Fixtures generated and tournament started.");
    } catch (const std::exception &e) {
      showMessage(e.what(), true);
    }
    return;
  } else if (page_ == Page::Matches && t) {
    float y = top + 4 - static_cast<float>(scroll_);
    for (size_t i = 0; i < t->matches.size(); ++i) {
      auto &m = t->matches[i];
      if (!search_.empty() &&
          ValidationService::normalize(m.id + " " + teamName(*t, m.homeTeamId) +
                                       " " + teamName(*t, m.awayTeamId))
                  .find(ValidationService::normalize(search_)) ==
              std::string::npos)
        continue;
      if (sf::FloatRect(left, y,
                        static_cast<float>(window_.getSize().x) - left - 30, 72)
              .contains(p) &&
          t->state == TournamentState::Active) {
        selectedMatchId_ = m.id;
        std::vector<std::string> values = {
            m.homeScore ? std::to_string(*m.homeScore) : "",
            m.awayScore ? std::to_string(*m.awayScore) : "", ""};
        for (const auto &s : m.scorers) {
          if (!values[2].empty())
            values[2] += ", ";
          const auto player = std::find_if(
              t->players.begin(), t->players.end(),
              [&](const Player &p) { return p.id == s.playerId; });
          values[2] += player == t->players.end() ? s.playerId : player->name;
          values[2] += "=" + std::to_string(s.goals);
        }
        beginDialog(Dialog::Score,
                    {"Home score", "Away score",
                     "Goalscorers (name=goals, comma separated; optional)"},
                    values);
        return;
      }
      y += 80;
    }
  } else if (page_ == Page::Reports && t &&
             sf::FloatRect(left, top + 82, 180, 38).contains(p)) {
    exportReport();
    return;
  }
}

void Application::exportReport() {
  try {
    auto *t = activeTournament();
    if (!t)
      throw std::logic_error("Select a tournament first.");
    std::filesystem::create_directories("reports");
    std::string safe = t->id;
    for (auto &c : safe)
      if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
        c = '_';
    auto path = std::filesystem::path("reports") / (safe + ".csv");
    std::ofstream out(path);
    if (!out)
      throw std::runtime_error("Could not create report file.");
    out << "Tournament," << csv(t->name) << "\nSeason," << csv(t->season)
        << "\nStatus," << stateName(t->state)
        << "\n\nPosition,Team,P,W,D,L,GF,GA,GD,PTS\n";
    auto rows = StandingsCalculator::calculate(*t);
    for (size_t i = 0; i < rows.size(); ++i) {
      const auto &s = rows[i];
      out << i + 1 << ',' << csv(s.teamName) << ',' << s.played << ',' << s.won
          << ',' << s.drawn << ',' << s.lost << ',' << s.goalsFor << ','
          << s.goalsAgainst << ',' << s.goalDifference << ',' << s.points
          << '\n';
    }
    out << "\nPlayer ID,Player,Team,Jersey,Position,Goals\n";
    for (const auto &p : StatisticsManager::goals(*t)) {
      auto player =
          std::find_if(t->players.begin(), t->players.end(),
                       [&](const Player &x) { return x.id == p.playerId; });
      if (player != t->players.end())
        out << csv(p.playerId) << ',' << csv(p.playerName) << ','
            << csv(p.teamName) << ',' << p.jersey << ','
            << positionName(player->position) << ',' << p.goals << '\n';
    }
    out << "\nMatch,Round,Home,Home score,Away score,Away,Status\n";
    for (const auto &m : t->matches)
      out << csv(m.id) << ',' << m.round << ','
          << csv(teamName(*t, m.homeTeamId)) << ','
          << (m.homeScore ? std::to_string(*m.homeScore) : "") << ','
          << (m.awayScore ? std::to_string(*m.awayScore) : "") << ','
          << csv(teamName(*t, m.awayTeamId)) << ','
          << (m.state == MatchState::Completed ? "Completed" : "Scheduled")
          << '\n';
    if (!out)
      throw std::runtime_error("Failed while writing the report.");
    showMessage("CSV exported to " + path.string());
  } catch (const std::exception &e) {
    showMessage(e.what(), true);
  }
}

int Application::run() {
  while (window_.isOpen()) {
    sf::Event e{};
    while (window_.pollEvent(e)) {
      if (e.type == sf::Event::Closed)
        window_.close();
      if (e.type == sf::Event::Resized)
        window_.setView(
            sf::View(sf::FloatRect(0, 0, static_cast<float>(e.size.width),
                                   static_cast<float>(e.size.height))));
      if (e.type == sf::Event::MouseMoved)
        mouse_ = sf::Mouse::getPosition(window_);
      if (e.type == sf::Event::MouseButtonPressed &&
          e.mouseButton.button == sf::Mouse::Left) {
        mouse_ = sf::Mouse::getPosition(window_);
        handleClick(sf::Vector2f(static_cast<float>(mouse_.x),
                                 static_cast<float>(mouse_.y)));
      }
      if (e.type == sf::Event::MouseWheelScrolled) {
        scroll_ =
            std::max(0, scroll_ + (e.mouseWheelScroll.delta < 0 ? 120 : -120));
      }
      if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Escape && dialog_ != Dialog::None) {
          dialog_ = Dialog::None;
          message_.clear();
        } else if (dialog_ != Dialog::None && e.key.code == sf::Keyboard::Tab &&
                   !fields_.empty()) {
          focusedField_ = (focusedField_ + 1) % fields_.size();
          if (focusedField_ < caretPositions_.size())
            caretPositions_[focusedField_] = fields_[focusedField_].size();
          caretClock_.restart();
        } else if (dialog_ != Dialog::None &&
                   (e.key.code == sf::Keyboard::Left ||
                    e.key.code == sf::Keyboard::Right ||
                    e.key.code == sf::Keyboard::Home ||
                    e.key.code == sf::Keyboard::End ||
                    e.key.code == sf::Keyboard::Delete) &&
                   !fields_.empty()) {
          auto &position = caretPositions_[focusedField_];
          auto &value = fields_[focusedField_];
          if (e.key.code == sf::Keyboard::Left && position > 0)
            --position;
          else if (e.key.code == sf::Keyboard::Right && position < value.size())
            ++position;
          else if (e.key.code == sf::Keyboard::Home)
            position = 0;
          else if (e.key.code == sf::Keyboard::End)
            position = value.size();
          else if (e.key.code == sf::Keyboard::Delete &&
                   position < value.size())
            value.erase(position, 1);
          caretClock_.restart();
        } else if (dialog_ == Dialog::None && searchFocused_ &&
                   e.key.code == sf::Keyboard::Enter) {
          searchFocused_ = false;
        } else if (dialog_ == Dialog::None && searchFocused_ &&
                   (e.key.code == sf::Keyboard::Left ||
                    e.key.code == sf::Keyboard::Right ||
                    e.key.code == sf::Keyboard::Home ||
                    e.key.code == sf::Keyboard::End ||
                    e.key.code == sf::Keyboard::Delete)) {
          if (e.key.code == sf::Keyboard::Left && searchCaretPosition_ > 0)
            --searchCaretPosition_;
          else if (e.key.code == sf::Keyboard::Right &&
                   searchCaretPosition_ < search_.size())
            ++searchCaretPosition_;
          else if (e.key.code == sf::Keyboard::Home)
            searchCaretPosition_ = 0;
          else if (e.key.code == sf::Keyboard::End)
            searchCaretPosition_ = search_.size();
          else if (e.key.code == sf::Keyboard::Delete &&
                   searchCaretPosition_ < search_.size())
            search_.erase(searchCaretPosition_, 1);
          caretClock_.restart();
        } else if (dialog_ != Dialog::None &&
                   e.key.code == sf::Keyboard::Enter) {
          submitDialog();
        } else if (dialog_ == Dialog::None &&
                   e.key.code == sf::Keyboard::Slash) {
          searchFocused_ = true;
          search_.clear();
          searchCaretPosition_ = 0;
          caretClock_.restart();
        } else if (dialog_ == Dialog::None &&
                   e.key.code == sf::Keyboard::Enter &&
                   page_ == Page::Tournaments && activeTournament() &&
                   activeTournament()->state == TournamentState::Draft &&
                   activeTournament()->teams.size() >= 2) {
          auto before = tournaments_;
          try {
            TournamentManager::start(*activeTournament());
            saveOrRollback(std::move(before));
            showMessage("Fixtures generated and tournament started.");
          } catch (const std::exception &x) {
            showMessage(x.what(), true);
          }
        }
      }
      if (e.type == sf::Event::TextEntered) {
        if (dialog_ != Dialog::None && !fields_.empty()) {
          auto &v = fields_[focusedField_];
          auto &position = caretPositions_[focusedField_];
          if (e.text.unicode == 8) {
            if (position > 0) {
              v.erase(position - 1, 1);
              --position;
            }
          } else if (e.text.unicode >= 32 && e.text.unicode < 127 &&
                     v.size() < 120)
            v.insert(v.begin() + static_cast<std::ptrdiff_t>(position++),
                     static_cast<char>(e.text.unicode));
          caretClock_.restart();
        } else if (searchFocused_) {
          if (e.text.unicode == 8) {
            if (searchCaretPosition_ > 0) {
              search_.erase(searchCaretPosition_ - 1, 1);
              --searchCaretPosition_;
            }
          } else if (e.text.unicode >= 32 && e.text.unicode < 127 &&
                     search_.size() < 60)
            search_.insert(search_.begin() + static_cast<std::ptrdiff_t>(
                                                 searchCaretPosition_++),
                           static_cast<char>(e.text.unicode));
          caretClock_.restart();
        }
      }
    }
    window_.clear(kBg);
    drawSidebar();
    drawPage();
    drawDialog();
    if (searchFocused_) {
      const auto h = static_cast<float>(window_.getSize().y);
      const sf::FloatRect searchBox(250, h - 100, 520, 42);
      drawRoundedRect(window_, searchBox, kInput, kAccent, 1.5f, 9.f, true);
      drawText("SEARCH", 266, h - 119, 9, kAccentLight);
      const float textX = searchBox.left + 14.f;
      drawText(search_, textX, h - 88, 13, sf::Color(18, 27, 40));
      if (caretClock_.getElapsedTime().asMilliseconds() % 1000 < 560) {
        sf::Text prefix(search_.substr(0, searchCaretPosition_), font_, 13);
        sf::RectangleShape caretBar({2.f, 18.f});
        caretBar.setPosition(textX + prefix.getLocalBounds().width, h - 89);
        caretBar.setFillColor(sf::Color::Black);
        window_.draw(caretBar);
      }
      drawText("ENTER to finish", searchBox.left + searchBox.width + 12.f,
               h - 87, 10, kMuted);
    }
    window_.display();
  }
  return 0;
}
} // namespace ftms

// ---- Application entry point (formerly src/main.cpp) ----
int main() {
  ftms::FileAuthenticationService authentication(
      std::filesystem::path(FTMS_DATA_DIR) / "referees.db");
  ftms::RefereeLogin login(authentication);
  auto referee = login.run();
  if (!referee)
    return 0;
  ftms::Application app(*referee);
  return app.run();
}
