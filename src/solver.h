#pragma once

#include <chrono>

#include "canonical.h"
#include "stats.h"
#include "memo_table.h"

struct SolveLimits { uint64_t max_states = 0; double time_limit_sec = 0; };

struct SolverConfig {
    int n; int k; bool use_memo = true; bool use_symmetry = true; bool use_move_ordering = true; uint64_t max_states = 0; double time_limit_sec = 0;
};

struct SolveResult { bool first_wins; SearchStats stats; bool complete = true; };

class Solver {
public:
    explicit Solver(SolverConfig config);
    SolveResult solve();
    SolveResult solveFromState(const State& root);
    MemoTable& memo() { return memo_; }
    const MemoTable& memo() const { return memo_; }
private:
    bool canFirstWin(const State& state, int depth);
    bool shouldStop() const;

    SolverConfig config_; Geometry geometry_; SearchStats stats_; MemoTable memo_; bool stopped_ = false; std::chrono::steady_clock::time_point start_;
};
