#include "security/Authentication.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ftms {
namespace {
constexpr ULONGLONG kPbkdf2Iterations = 150000;
constexpr ULONG kSaltBytes = 16;
constexpr ULONG kHashBytes = 32;

std::string normalizeId(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::vector<unsigned char> decodeHex(const std::string& text) {
    if (text.size() % 2 != 0) throw std::runtime_error("Invalid referee account database.");
    std::vector<unsigned char> bytes;
    bytes.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const auto byte = static_cast<unsigned int>(std::stoul(text.substr(i, 2), nullptr, 16));
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

std::string encodeHex(const std::vector<unsigned char>& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char byte : bytes) out << std::setw(2) << static_cast<unsigned int>(byte);
    return out.str();
}

std::vector<unsigned char> deriveHash(const std::string& password,
                                     const std::vector<unsigned char>& salt) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    const NTSTATUS opened = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                                         nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (opened < 0) throw std::runtime_error("Could not initialize password hashing.");

    std::vector<unsigned char> hash(kHashBytes);
    auto* passwordBytes = reinterpret_cast<PUCHAR>(const_cast<char*>(password.data()));
    const NTSTATUS derived = BCryptDeriveKeyPBKDF2(
        algorithm, passwordBytes, static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt.data()), static_cast<ULONG>(salt.size()),
        kPbkdf2Iterations, hash.data(), static_cast<ULONG>(hash.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (derived < 0) throw std::runtime_error("Could not hash the referee password.");
    return hash;
}

std::vector<unsigned char> newSalt() {
    std::vector<unsigned char> salt(kSaltBytes);
    if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
        throw std::runtime_error("Could not create secure referee account credentials.");
    return salt;
}
} // namespace

User::User(std::string userId, std::vector<unsigned char> salt,
           std::vector<unsigned char> passwordHash)
    : userId_(std::move(userId)), salt_(std::move(salt)), passwordHash_(std::move(passwordHash)) {}

bool User::verifyPassword(const std::string& password) const {
    const auto candidate = deriveHash(password, salt_);
    if (candidate.size() != passwordHash_.size()) return false;
    unsigned char difference = 0;
    for (std::size_t i = 0; i < candidate.size(); ++i)
        difference |= static_cast<unsigned char>(candidate[i] ^ passwordHash_[i]);
    return difference == 0;
}

Referee::Referee(std::string userId, std::vector<unsigned char> salt,
                 std::vector<unsigned char> passwordHash)
    : User(std::move(userId), std::move(salt), std::move(passwordHash)) {}

std::unique_ptr<User> Referee::clone() const { return std::make_unique<Referee>(*this); }

FileAuthenticationService::FileAuthenticationService(std::string databasePath)
    : databasePath_(std::move(databasePath)) { load(); }

void FileAuthenticationService::load() {
    std::ifstream input(databasePath_);
    if (!input) return;
    std::string id, saltHex, hashHex;
    while (input >> std::quoted(id) >> saltHex >> hashHex) {
        auto salt = decodeHex(saltHex);
        auto hash = decodeHex(hashHex);
        if (salt.size() != kSaltBytes || hash.size() != kHashBytes)
            throw std::runtime_error("Referee account database contains an invalid credential record.");
        users_.push_back(std::make_unique<Referee>(std::move(id), std::move(salt), std::move(hash)));
    }
    if (!input.eof()) throw std::runtime_error("Referee account database could not be read.");
}

std::unique_ptr<User> FileAuthenticationService::authenticate(const std::string& userId,
                                                               const std::string& password) const {
    const auto key = normalizeId(userId);
    for (const auto& user : users_) {
        if (normalizeId(user->userId()) == key && user->verifyPassword(password))
            return user->clone();
    }
    return nullptr;
}

void FileAuthenticationService::registerReferee(const std::string& userId,
                                                 const std::string& password) {
    if (userId.size() < 4 || userId.size() > 24 ||
        !std::all_of(userId.begin(), userId.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        }))
        throw std::invalid_argument("Referee ID must be 4–24 characters using letters, numbers, _ or -.");
    if (password.size() < 8)
        throw std::invalid_argument("Password must contain at least 8 characters.");
    if (std::any_of(users_.begin(), users_.end(), [&](const auto& user) {
            return normalizeId(user->userId()) == normalizeId(userId);
        }))
        throw std::invalid_argument("That referee ID is already registered. Choose a unique ID.");

    auto salt = newSalt();
    auto hash = deriveHash(password, salt);
    Referee user(userId, std::move(salt), std::move(hash));
    appendAccount(user);
    users_.push_back(user.clone());
}

void FileAuthenticationService::appendAccount(const Referee& user) {
    const auto parent = std::filesystem::path(databasePath_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    std::ofstream output(databasePath_, std::ios::app);
    if (!output) throw std::runtime_error("Could not save referee account data.");
    output << std::quoted(user.userId()) << ' ' << encodeHex(user.salt_) << ' '
           << encodeHex(user.passwordHash_) << '\n';
    output.flush();
    if (!output) throw std::runtime_error("Could not finish saving referee account data.");
}

} // namespace ftms
