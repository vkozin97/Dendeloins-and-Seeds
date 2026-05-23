#include "solver.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <vector>

Solver::Solver(SolverConfig config) : config_(config), geometry_(config.n) {
    stats_.n = config.n;
    stats_.k = config.k;
}

SolveResult Solver::solve() {
    auto begin = std::chrono::steady_clock::now();
    const bool first_wins = canFirstWin(State{}, 0);
    auto end = std::chrono::steady_clock::now();
    stats_.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    stats_.unique_states_stored = memo_.size();
    return {first_wins, stats_};
}

bool Solver::canFirstWin(const State& original, int depth) {
    stats_.recursive_calls++;
    stats_.max_depth = std::max(stats_.max_depth, depth);

    State state = original;
    StateKey key{};
    if (config_.use_symmetry) {
        stats_.canonicalization_calls++;
        const auto c = canonicalize(state, geometry_);
        key = c.key;
        state = {key.dandelions, key.occupied, key.used_dirs};
    } else {
        key = makeKey(state);
    }

    if (config_.use_memo) {
        if (auto it = memo_.find(key); it != memo_.end()) {
            stats_.memo_hits++;
            return it->second;
        }
    }

    if (isTerminalWin(state, geometry_)) {
        stats_.terminal_win_hits++;
        if (config_.use_memo) memo_[key] = true;
        return true;
    }
    if (isTerminalLoss(state, config_.k)) {
        stats_.terminal_loss_hits++;
        if (config_.use_memo) memo_[key] = false;
        return false;
    }

    std::vector<int> plants;
    uint64_t pmask = plantableMask(state, geometry_);
    while (pmask) { int c = std::countr_zero(pmask); pmask &= pmask - 1; plants.push_back(c);}    

    std::vector<int> available_dirs;
    for (int d=0; d<8; ++d) if (!(state.used_dirs & (1u<<d))) available_dirs.push_back(d);

    if (config_.use_move_ordering) {
        std::sort(plants.begin(), plants.end(), [&](int a, int b){
            State sa = applyPlant(state, a), sb = applyPlant(state, b);
            int mina=1e9, minb=1e9;
            for (int d: available_dirs) {
                mina = std::min(mina, std::popcount(blowMask(sa, d, geometry_) & ~state.occupied));
                minb = std::min(minb, std::popcount(blowMask(sb, d, geometry_) & ~state.occupied));
            }
            return mina > minb;
        });
    }

    for (int plant : plants) {
        stats_.plant_moves_considered++;
        State after_plant = applyPlant(state, plant);

        std::vector<int> dirs = available_dirs;
        if (config_.use_move_ordering) {
            std::sort(dirs.begin(), dirs.end(), [&](int a, int b){
                int sa = std::popcount(blowMask(after_plant, a, geometry_) & ~after_plant.occupied);
                int sb = std::popcount(blowMask(after_plant, b, geometry_) & ~after_plant.occupied);
                return sa < sb;
            });
        }

        bool wins_all = true;
        for (int dir : dirs) {
            stats_.wind_moves_considered++;
            State next = applyWind(after_plant, dir, geometry_);
            if (!canFirstWin(next, depth + 1)) {
                wins_all = false;
                stats_.wind_cutoffs++;
                break;
            }
        }

        if (wins_all) {
            stats_.plant_cutoffs++;
            if (config_.use_memo) memo_[key] = true;
            return true;
        }
    }

    if (config_.use_memo) memo_[key] = false;
    return false;
}
