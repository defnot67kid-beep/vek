#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SecurityHash {
using Sha256Digest = std::array<std::uint8_t, 32>;

Sha256Digest Sha256(const std::uint8_t* data, std::size_t size);
Sha256Digest Sha256(std::string_view text);
Sha256Digest HmacSha256(const std::vector<std::uint8_t>& key, std::string_view data);
std::string Hex(const Sha256Digest& digest);
bool ConstantTimeEqual(const Sha256Digest& a, const Sha256Digest& b);
bool ParseHex64(std::string_view hex, std::array<std::uint8_t, 64>& out);
}
