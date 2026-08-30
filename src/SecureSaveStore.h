#pragma once
#include <filesystem>
#include <string>

class SecureSaveStore {
public:
    static bool Write(const std::filesystem::path& path, const std::string& payload, std::string* error = nullptr);
    static bool Read(const std::filesystem::path& path, std::string& payload, std::string* error = nullptr);
};
