#pragma once

#include <cstdint>

#include "geometry.h"

struct State {
    uint64_t dandelions = 0;
    uint64_t occupied = 0;
    uint8_t used_dirs = 0;
};

bool isValidState(const State& state, const Geometry& geometry);
bool isTerminalWin(const State& state, const Geometry& geometry);
bool isTerminalLoss(const State& state, int k);
int roundIndex(const State& state);
uint64_t plantableMask(const State& state, const Geometry& geometry);
State applyPlant(const State& state, int cell_id);
uint64_t blowMask(const State& state, int dir, const Geometry& geometry);
State applyWind(const State& state, int dir, const Geometry& geometry);
