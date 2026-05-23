#pragma once
#include <cstdint>
#include <stdexcept>
#include "state.h"

struct Key80 { uint64_t lo; uint16_t hi; };

struct PackedKey {
    bool is80 = false;
    uint64_t lo = 0;
    uint16_t hi = 0;
};

inline PackedKey packStateKey(const State& s, int n) {
    const int m = n * n;
    if (n <= 5) {
        uint64_t key = (s.occupied & ((1ULL << m) - 1ULL)) |
                       ((s.dandelions & ((1ULL << m) - 1ULL)) << m) |
                       (static_cast<uint64_t>(s.used_dirs) << (2 * m));
        return {false, key, 0};
    }
    if (n == 6) {
        PackedKey k{}; k.is80 = true;
        k.lo = (s.occupied & ((1ULL << 36) - 1ULL)) |
               ((s.dandelions & ((1ULL << 28) - 1ULL)) << 36);
        k.hi = static_cast<uint16_t>(((s.dandelions >> 28) & 0xFF) | (static_cast<uint16_t>(s.used_dirs) << 8));
        return k;
    }
    throw std::runtime_error("unsupported optimized key format for n > 6");
}

inline bool equalPacked(const PackedKey& a, const PackedKey& b){return a.is80==b.is80 && a.lo==b.lo && (!a.is80 || a.hi==b.hi);} 

inline uint64_t splitmix64(uint64_t x){ x+=0x9e3779b97f4a7c15ULL; x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL; x=(x^(x>>27))*0x94d049bb133111ebULL; return x^(x>>31);} 

inline uint64_t hashPacked(const PackedKey& k, uint64_t seed=0x726f6f7448617368ULL){
    if (!k.is80) return splitmix64(k.lo ^ seed);
    uint64_t h = splitmix64(k.lo ^ seed);
    h ^= splitmix64(static_cast<uint64_t>(k.hi) + 0x9e3779b97f4a7c15ULL);
    return splitmix64(h);
}
