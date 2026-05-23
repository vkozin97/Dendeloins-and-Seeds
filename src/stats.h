#pragma once

#include <cstdint>
#include <string>

struct SearchStats {
    int n = 0;
    int k = 0;

    uint64_t recursive_calls = 0;
    uint64_t memo_hits = 0;
    uint64_t unique_states_stored = 0;
    uint64_t canonicalization_calls = 0;

    uint64_t terminal_win_hits = 0;
    uint64_t terminal_loss_hits = 0;

    uint64_t plant_moves_considered = 0;
    uint64_t wind_moves_considered = 0;

    uint64_t plant_cutoffs = 0;
    uint64_t wind_cutoffs = 0;

    int max_depth = 0;
    long long elapsed_ms = 0;
};

void printStatsHumanReadable(const SearchStats& stats, bool winner);
void appendStatsCsv(const std::string& path, const SearchStats& stats, bool winner);
