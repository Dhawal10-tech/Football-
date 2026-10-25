#pragma once
#include "ftms/domain.hpp"
#include <filesystem>
#include <vector>
namespace ftms {
class StorageInterface {
public:
    virtual ~StorageInterface() = default;
    virtual std::vector<Tournament> load() const = 0;
    virtual void save(const std::vector<Tournament>&) const = 0;
};
class FileRepository final : public StorageInterface {
    std::filesystem::path path_;
public:
    explicit FileRepository(std::filesystem::path path);
    std::vector<Tournament> load() const override;
    void save(const std::vector<Tournament>&) const override;
};
}
