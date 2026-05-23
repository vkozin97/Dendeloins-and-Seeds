#include "stats.h"

#include <filesystem>
#include <fstream>
#include <iostream>

void printStatsHumanReadable(const SearchStats& stats, bool winner) {
    std::cout << "n = " << stats.n << "\n";
    std::cout << "k = " << stats.k << "\n";
    std::cout << "winner = " << (winner ? "P1" : "P2") << "\n";
    std::cout << "elapsed_ms = " << stats.elapsed_ms << "\n";
    std::cout << "recursive_calls = " << stats.recursive_calls << "\n";
    std::cout << "unique_states = " << stats.unique_states_stored << "\n";
    std::cout << "memo_hits = " << stats.memo_hits << "\n";
    std::cout << "canonicalization_calls = " << stats.canonicalization_calls << "\n";
    std::cout << "terminal_win_hits = " << stats.terminal_win_hits << "\n";
    std::cout << "terminal_loss_hits = " << stats.terminal_loss_hits << "\n";
    std::cout << "plant_moves_considered = " << stats.plant_moves_considered << "\n";
    std::cout << "wind_moves_considered = " << stats.wind_moves_considered << "\n";
    std::cout << "plant_cutoffs = " << stats.plant_cutoffs << "\n";
    std::cout << "wind_cutoffs = " << stats.wind_cutoffs << "\n";
    std::cout << "max_depth = " << stats.max_depth << "\n";
}

void appendStatsCsv(const std::string& path, const SearchStats& stats, bool winner) {
    const bool exists = std::filesystem::exists(path);
    std::ofstream out(path, std::ios::app);
    if (!exists) {
        out << "n,k,winner,elapsed_ms,recursive_calls,unique_states,memo_hits,plant_cutoffs,wind_cutoffs,max_depth\n";
    }
    out << stats.n << ',' << stats.k << ',' << (winner ? "P1" : "P2") << ',' << stats.elapsed_ms << ','
        << stats.recursive_calls << ',' << stats.unique_states_stored << ',' << stats.memo_hits << ','
        << stats.plant_cutoffs << ',' << stats.wind_cutoffs << ',' << stats.max_depth << '\n';
}
