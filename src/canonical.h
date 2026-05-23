#pragma once

#include <cstdint>
#include <functional>

#include "state.h"

struct StateKey {
    uint64_t dandelions;
    uint64_t occupied;
    uint8_t used_dirs;

    bool operator==(const StateKey& other) const;
};

struct StateKeyHash {
    std::size_t operator()(const StateKey& k) const;
};

struct CanonicalResult {
    StateKey key;
    int symmetry;
};

StateKey makeKey(const State& state);
State transformState(const State& state, int sym, const Geometry& geometry);
CanonicalResult canonicalize(const State& state, const Geometry& geometry);
