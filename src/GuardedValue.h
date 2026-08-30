#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

// Lightweight runtime tamper-evident integer storage. It is intentionally NOT
// advertised as cryptography: its purpose is to avoid leaving obvious plain
// economy integers in memory and to detect simple single-address edits.
class GuardedInt {
public:
    GuardedInt() { Set(0); }
    GuardedInt(int value) { Set(value); }

    GuardedInt& operator=(int value) { Set(value); return *this; }
    GuardedInt& operator+=(int delta) { Set(Get() + delta); return *this; }
    GuardedInt& operator-=(int delta) { Set(Get() - delta); return *this; }

    operator int() const { return Get(); }
    int Get() const {
        if (!IntegrityOK()) { tampered = true; return 0; }
        return static_cast<int>(encoded ^ mask);
    }
    void Set(int value) {
        mask = NextMask();
        encoded = static_cast<std::uint32_t>(value) ^ mask;
        mirror = ~encoded;
        tag = MakeTag(encoded, mask);
        tampered = false;
    }
    bool IntegrityOK() const { return mirror == ~encoded && tag == MakeTag(encoded, mask); }
    bool TamperDetected() const { return tampered || !IntegrityOK(); }

private:
    std::uint32_t encoded = 0;
    std::uint32_t mask = 0;
    std::uint32_t mirror = 0;
    std::uint32_t tag = 0;
    mutable bool tampered = false;

    static std::uint32_t Rotl(std::uint32_t x, int r) { return (x << r) | (x >> (32-r)); }
    static std::uint32_t MakeTag(std::uint32_t e, std::uint32_t m) {
        std::uint32_t x = e ^ Rotl(m, 11) ^ 0xA7F31C59u;
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return x;
    }
    static std::uint32_t NextMask() {
        static std::atomic<std::uint32_t> state{[](){
            auto t = static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::uint32_t s = static_cast<std::uint32_t>(t ^ (t >> 32) ^ 0x9E3779B9u);
            return s ? s : 0xC001D00Du;
        }()};
        std::uint32_t old = state.load(std::memory_order_relaxed);
        for (;;) {
            std::uint32_t x = old;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            if (x == 0) x = 0x6D2B79F5u;
            if (state.compare_exchange_weak(old, x, std::memory_order_relaxed)) return x;
        }
    }
};
