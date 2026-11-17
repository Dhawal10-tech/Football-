#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ftms {

// Abstract user contract: each role declares its own permissions and display name.
class User {
    friend class FileAuthenticationService;
    std::string userId_;
    std::vector<unsigned char> salt_;
    std::vector<unsigned char> passwordHash_;

protected:
    User(std::string userId, std::vector<unsigned char> salt,
         std::vector<unsigned char> passwordHash);
    bool verifyPassword(const std::string& password) const;

public:
    virtual ~User() = default;
    const std::string& userId() const noexcept { return userId_; }
    virtual std::string roleName() const = 0;
    virtual bool canRecordScores() const noexcept = 0;
    virtual std::unique_ptr<User> clone() const = 0;
};

// Inheritance: Referee is a concrete User role with score-entry permission.
class Referee final : public User {
public:
    Referee(std::string userId, std::vector<unsigned char> salt,
            std::vector<unsigned char> passwordHash);
    std::string roleName() const override { return "Referee"; }
    bool canRecordScores() const noexcept override { return true; }
    std::unique_ptr<User> clone() const override;
};

// Abstraction: the login screen depends on this interface, not the file format.
class IAuthenticationService {
public:
    virtual ~IAuthenticationService() = default;
    virtual std::unique_ptr<User> authenticate(const std::string& userId,
                                               const std::string& password) const = 0;
    virtual void registerReferee(const std::string& userId,
                                 const std::string& password) = 0;
};

// Encapsulates local account storage and unique-ID validation.
class FileAuthenticationService final : public IAuthenticationService {
    std::string databasePath_;
    std::vector<std::unique_ptr<User>> users_;
    void load();
    void appendAccount(const Referee& user);

public:
    explicit FileAuthenticationService(std::string databasePath);
    std::unique_ptr<User> authenticate(const std::string& userId,
                                       const std::string& password) const override;
    void registerReferee(const std::string& userId,
                         const std::string& password) override;
};

} // namespace ftms
