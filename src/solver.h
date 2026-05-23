#pragma once

#include <unordered_map>

#include "canonical.h"
#include "stats.h"

struct SolverConfig {
    int n;
    int k;
    bool use_memo = true;
    bool use_symmetry = true;
    bool use_move_ordering = true;
};

struct SolveResult {
    bool first_wins;
    SearchStats stats;
};

class Solver {
public:
    explicit Solver(SolverConfig config);
    SolveResult solve();

private:
    bool canFirstWin(const State& state, int depth);

    SolverConfig config_;
    Geometry geometry_;
    SearchStats stats_;
    std::unordered_map<StateKey, bool, StateKeyHash> memo_;
};
