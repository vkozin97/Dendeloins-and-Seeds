#include "state.h"

#include <bit>

bool isValidState(const State& state, const Geometry& geometry) {
    const uint64_t full = geometry.fullBoardMask();
    if ((state.dandelions & ~state.occupied) != 0) return false;
    if ((state.dandelions & ~full) != 0) return false;
    if ((state.occupied & ~full) != 0) return false;
    return true;
}

bool isTerminalWin(const State& state, const Geometry& geometry) {
    return state.occupied == geometry.fullBoardMask();
}

bool isTerminalLoss(const State& state, int k) { return roundIndex(state) >= k; }

int roundIndex(const State& state) { return std::popcount(state.used_dirs); }

uint64_t plantableMask(const State& state, const Geometry& geometry) {
    return geometry.fullBoardMask() & ~state.dandelions;
}

State applyPlant(const State& state, int cell_id) {
    State next = state;
    const uint64_t bit = 1ULL << cell_id;
    next.dandelions |= bit;
    next.occupied |= bit;
    return next;
}

uint64_t blowMask(const State& state, int dir, const Geometry& geometry) {
    uint64_t d = state.dandelions;
    uint64_t out = 0;
    while (d) {
        const int cell = std::countr_zero(d);
        d &= (d - 1);
        out |= geometry.ray(cell, dir);
    }
    return out;
}

State applyWind(const State& state, int dir, const Geometry& geometry) {
    State next = state;
    next.occupied |= blowMask(state, dir, geometry);
    next.used_dirs |= static_cast<uint8_t>(1u << dir);
    return next;
}
