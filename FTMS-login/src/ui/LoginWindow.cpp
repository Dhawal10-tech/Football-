#include "ui/LoginWindow.hpp"
#include "ui/UITheme.hpp"

#include <windows.h>

#include <algorithm>
#include <stdexcept>

namespace ftms {
namespace {
const sf::Color kLoginBg(18, 24, 22);
const sf::Color kPanel(26, 36, 33);
const sf::Color kMuted(148, 163, 184);
const sf::Color kGreen(16, 185, 129);
const sf::Color kRed(239, 100, 90);
}

LoginWindow::LoginWindow(IAuthenticationService& authentication)
    : authentication_(authentication),
      window_(sf::VideoMode(820, 620), "FTMS | Referee Login",
              sf::Style::Default) {
    ShowWindow(window_.getSystemHandle(), SW_MAXIMIZE);
    window_.setFramerateLimit(60);
    UITheme::loadFonts();
    font_ = UITheme::regularFont;
}

std::unique_ptr<User> LoginWindow::run() {
    while (window_.isOpen()) {
        sf::Event event{};
        while (window_.pollEvent(event)) handle(event);
        draw();
    }
    return std::move(signedInUser_);
}

void LoginWindow::handle(const sf::Event& event) {
    if (event.type == sf::Event::Closed) window_.close();
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const float x = static_cast<float>(event.mouseButton.x);
        const float y = static_cast<float>(event.mouseButton.y);
        if (sf::FloatRect(170, 210, 480, 44).contains(x, y)) focusedField_ = 0;
        else if (sf::FloatRect(170, 290, 480, 44).contains(x, y)) focusedField_ = 1;
        else if (registerMode_ && sf::FloatRect(170, 370, 480, 44).contains(x, y)) focusedField_ = 2;
        const float buttonY = registerMode_ ? 445.f : 390.f;
        if (sf::FloatRect(170, buttonY, 220, 46).contains(x, y)) submit();
        else if (sf::FloatRect(405, buttonY, 245, 46).contains(x, y)) {
            registerMode_ = !registerMode_;
            focusedField_ = 0;
            message_.clear();
        }
    }
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) window_.close();
        if (event.key.code == sf::Keyboard::Tab) {
            const std::size_t fieldCount = registerMode_ ? 3 : 2;
            focusedField_ = (focusedField_ + 1) % fieldCount;
        }
        if (event.key.code == sf::Keyboard::Enter) submit();
        if (event.key.code == sf::Keyboard::BackSpace) {
            std::string* fields[] = {&userId_, &password_, &confirmPassword_};
            auto& value = *fields[std::min<std::size_t>(focusedField_, 2)];
            if (!value.empty()) value.pop_back();
        }
    }
    if (event.type == sf::Event::TextEntered && event.text.unicode >= 32 && event.text.unicode < 127) {
        std::string* fields[] = {&userId_, &password_, &confirmPassword_};
        auto& value = *fields[std::min<std::size_t>(focusedField_, 2)];
        const std::size_t limit = focusedField_ == 0 ? 24 : 64;
        if (value.size() < limit) value += static_cast<char>(event.text.unicode);
    }
}

void LoginWindow::submit() {
    try {
        if (registerMode_) {
            if (password_ != confirmPassword_)
                throw std::invalid_argument("The two passwords do not match.");
            authentication_.registerReferee(userId_, password_);
            registerMode_ = false;
            password_.clear();
            confirmPassword_.clear();
            focusedField_ = 1;
            message_ = "Referee account created. Sign in with your ID and password.";
            messageIsError_ = false;
            return;
        }
        signedInUser_ = authentication_.authenticate(userId_, password_);
        if (!signedInUser_) {
            message_ = "Referee ID or password is incorrect.";
            messageIsError_ = true;
            password_.clear();
            focusedField_ = 1;
            return;
        }
        if (!signedInUser_->canRecordScores()) {
            signedInUser_.reset();
            throw std::runtime_error("This account is not allowed to enter match scores.");
        }
        window_.close();
    } catch (const std::exception& error) {
        message_ = error.what();
        messageIsError_ = true;
    }
}

void LoginWindow::drawText(const std::string& text, float x, float y, unsigned size,
                           sf::Color color, bool bold) {
    sf::Text item(text, font_, size);
    item.setFillColor(color);
    if (bold) item.setStyle(sf::Text::Bold);
    item.setPosition(x, y);
    window_.draw(item);
}

void LoginWindow::draw() {
    window_.clear(kLoginBg);
    sf::RectangleShape panel({660.f, 520.f});
    panel.setPosition(80.f, 42.f);
    panel.setFillColor(kPanel);
    panel.setOutlineColor(UITheme::BorderColor);
    panel.setOutlineThickness(1.f);
    window_.draw(panel);

    drawText("FTMS 2026", 170, 78, 14, kGreen, true);
    drawText(registerMode_ ? "Create referee account" : "Referee sign in", 170, 108, 28, sf::Color::White, true);
    drawText(registerMode_ ? "Create a unique referee ID to supervise match results." :
                              "Sign in with your referee ID before entering match scores.",
             170, 151, 13, kMuted);

    const auto input = [&](float y, const std::string& label, const std::string& value,
                           std::size_t index, bool secret) {
        drawText(label, 170, y - 21, 12, kMuted);
        sf::RectangleShape box({480.f, 44.f});
        box.setPosition(170.f, y);
        box.setFillColor(kLoginBg);
        box.setOutlineColor(focusedField_ == index ? kGreen : UITheme::BorderColor);
        box.setOutlineThickness(1.f);
        window_.draw(box);
        const std::string shown = secret ? std::string(value.size(), '*') : value;
        drawText(shown.empty() ? (secret ? "Enter password" : "Enter referee ID") : shown,
                 184, y + 11, 14, shown.empty() ? kMuted : sf::Color::White);
    };
    input(210, "Referee ID", userId_, 0, false);
    input(290, "Password", password_, 1, true);
    if (registerMode_) input(370, "Confirm password", confirmPassword_, 2, true);

    const float buttonY = registerMode_ ? 445.f : 390.f;
    sf::RectangleShape primary({220.f, 46.f});
    primary.setPosition(170.f, buttonY);
    primary.setFillColor(kGreen);
    window_.draw(primary);
    drawText(registerMode_ ? "CREATE ACCOUNT" : "SIGN IN", 193, buttonY + 13, 13, kLoginBg, true);

    sf::RectangleShape secondary({245.f, 46.f});
    secondary.setPosition(405.f, buttonY);
    secondary.setFillColor(sf::Color(42, 58, 51));
    secondary.setOutlineColor(UITheme::BorderColor);
    secondary.setOutlineThickness(1.f);
    window_.draw(secondary);
    drawText(registerMode_ ? "BACK TO SIGN IN" : "CREATE REFEREE ID", 423, buttonY + 13, 12, sf::Color::White);

    if (!message_.empty()) drawText(message_, 170, 512, 12, messageIsError_ ? kRed : kGreen);
    else drawText("Use TAB to move between fields. Passwords must be at least 8 characters.", 170, 512, 11, kMuted);
    drawText("Referee accounts are saved on this computer; passwords are stored as salted hashes.",
             170, 541, 10, kMuted);
    window_.display();
}

} // namespace ftms
