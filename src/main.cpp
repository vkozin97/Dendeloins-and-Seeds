#include <iostream>
#include <string>

#include "solver.h"

int main(int argc, char** argv) {
    int n = -1;
    int k = -1;
    bool batch_small = false;
    bool use_memo = true;
    bool use_symmetry = true;
    bool use_move_ordering = true;
    std::string csv_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--n" && i + 1 < argc) n = std::stoi(argv[++i]);
        else if (arg == "--k" && i + 1 < argc) k = std::stoi(argv[++i]);
        else if (arg == "--batch-small") batch_small = true;
        else if (arg == "--no-memo") use_memo = false;
        else if (arg == "--no-symmetry") use_symmetry = false;
        else if (arg == "--no-move-ordering") use_move_ordering = false;
        else if (arg == "--stats-csv" && i + 1 < argc) csv_path = argv[++i];
    }

    auto run_one = [&](int rn, int rk) {
        Solver solver({rn, rk, use_memo, use_symmetry, use_move_ordering});
        SolveResult result = solver.solve();
        printStatsHumanReadable(result.stats, result.first_wins);
        if (!csv_path.empty()) appendStatsCsv(csv_path, result.stats, result.first_wins);
    };

    if (batch_small) {
        for (int bn = 1; bn <= 4; ++bn) {
            for (int bk = 1; bk <= std::min(8, bn * bn); ++bk) {
                run_one(bn, bk);
            }
        }
        return 0;
    }

    if (n <= 0 || k <= 0) {
        std::cerr << "Usage: --n <size> --k <rounds> [--batch-small] [--stats-csv <path>]\n";
        return 1;
    }

    run_one(n, k);
    return 0;
}
