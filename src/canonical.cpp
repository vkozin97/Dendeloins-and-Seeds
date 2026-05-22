#include "canonical.h"

bool StateKey::operator==(const StateKey& other) const {
    return dandelions == other.dandelions && occupied == other.occupied && used_dirs == other.used_dirs;
}

std::size_t StateKeyHash::operator()(const StateKey& k) const {
    std::size_t h = std::hash<uint64_t>{}(k.occupied);
    h ^= (std::hash<uint64_t>{}(k.dandelions) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    h ^= (std::hash<uint8_t>{}(k.used_dirs) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    return h;
}

StateKey makeKey(const State& state) { return {state.dandelions, state.occupied, state.used_dirs}; }

State transformState(const State& state, int sym, const Geometry& geometry) {
    State out{};
    uint64_t d = state.dandelions;
    while (d) {
        int cell = std::countr_zero(d);
        d &= (d - 1);
        out.dandelions |= (1ULL << geometry.transformCell(sym, cell));
    }
    uint64_t o = state.occupied;
    while (o) {
        int cell = std::countr_zero(o);
        o &= (o - 1);
        out.occupied |= (1ULL << geometry.transformCell(sym, cell));
    }
    uint8_t used = 0;
    for (int dir = 0; dir < 8; ++dir) {
        if (state.used_dirs & (1u << dir)) {
            used |= static_cast<uint8_t>(1u << geometry.transformDir(sym, dir));
        }
    }
    out.used_dirs = used;
    return out;
}

CanonicalResult canonicalize(const State& state, const Geometry& geometry) {
    CanonicalResult best{makeKey(state), 0};
    for (int sym = 1; sym < 8; ++sym) {
        const State transformed = transformState(state, sym, geometry);
        const StateKey key = makeKey(transformed);
        if (key.occupied < best.key.occupied ||
            (key.occupied == best.key.occupied && key.dandelions < best.key.dandelions) ||
            (key.occupied == best.key.occupied && key.dandelions == best.key.dandelions && key.used_dirs < best.key.used_dirs)) {
            best = {key, sym};
        }
    }
    return best;
}
