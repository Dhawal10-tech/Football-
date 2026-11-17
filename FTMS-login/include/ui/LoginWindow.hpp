#pragma once

#include "security/Authentication.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

namespace ftms {

class LoginWindow {
    IAuthenticationService& authentication_;
    sf::RenderWindow window_;
    sf::Font font_;
    bool registerMode_{};
    std::size_t focusedField_{};
    std::string userId_, password_, confirmPassword_, message_;
    bool messageIsError_{};
    std::unique_ptr<User> signedInUser_;

    void handle(const sf::Event& event);
    void submit();
    void drawText(const std::string& text, float x, float y, unsigned size,
                  sf::Color color, bool bold = false);
    void draw();

public:
    explicit LoginWindow(IAuthenticationService& authentication);
    std::unique_ptr<User> run();
};

} // namespace ftms
