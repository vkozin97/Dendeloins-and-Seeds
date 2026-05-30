#include <filesystem>
#include <cstring>
#include <iostream>
#include <string>

#include "memo_file.h"
#include "solver.h"

int main(int argc, char** argv) {
    int n = -1, k = -1; bool batch_small=false, use_memo=true, use_symmetry=true, use_move_ordering=true; std::string csv_path, save_memo, load_memo; uint64_t max_states=0; double time_limit=0.0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--n" && i + 1 < argc) n = std::stoi(argv[++i]);
        else if (arg == "--k" && i + 1 < argc) k = std::stoi(argv[++i]);
        else if (arg == "--batch-small") batch_small = true;
        else if (arg == "--no-memo") use_memo = false;
        else if (arg == "--no-symmetry") use_symmetry = false;
        else if (arg == "--no-move-ordering") use_move_ordering = false;
        else if (arg == "--stats-csv" && i + 1 < argc) csv_path = argv[++i];
        else if (arg == "--save-memo" && i + 1 < argc) save_memo = argv[++i];
        else if (arg == "--load-memo" && i + 1 < argc) load_memo = argv[++i];
        else if (arg == "--max-states" && i + 1 < argc) max_states = std::stoull(argv[++i]);
        else if (arg == "--time-limit-sec" && i + 1 < argc) time_limit = std::stod(argv[++i]);
    }

    auto run_one = [&](int rn, int rk) {
        Solver solver({rn, rk, use_memo, use_symmetry, use_move_ordering, max_states, time_limit});
        if(!load_memo.empty() && std::filesystem::exists(load_memo)){
            MemoFileHeader h{}; MemoTable t(rn==6); std::string err;
            if(loadMemoFile(load_memo,h,t,err) && h.n==rn && h.k==rk){ solver.memo() = std::move(t); std::cout<<"Loaded memo: "<<load_memo<<"\n"; }
            else std::cerr<<"Failed to load memo: "<<err<<"\n";
        }
        SolveResult result = solver.solve();
        printStatsHumanReadable(result.stats, result.first_wins);
        if (!csv_path.empty()) appendStatsCsv(csv_path, result.stats, result.first_wins);
        if(!save_memo.empty()){
            MemoFileHeader h{}; std::memcpy(h.magic,"DSMEM01",8); h.file_version=1; h.n=rn; h.k=rk; h.rules_version=1; h.direction_scheme=1; h.key_format=(rn==6?2:1); h.canonical_keys=use_symmetry?1:0; h.db_scope=1; h.complete=result.complete?1:0; h.rules_hash=0xD011AULL; h.hash_seed=0x726f6f7448617368ULL; h.hash_version=1; h.bucket_capacity=solver.memo().capacity(); h.entries_count=solver.memo().size(); h.root_result=result.first_wins?1:0;
            if(saveMemoFile(save_memo,h,solver.memo())) std::cout<<"Saved memo: "<<save_memo<<"\n"; else std::cerr<<"Failed to save memo\n";
        }
    };

    if (batch_small) { for (int bn = 1; bn <= 4; ++bn) for (int bk = 1; bk <= std::min(8, bn * bn); ++bk) run_one(bn, bk); return 0; }
    if (n <= 0 || k <= 0) { std::cerr << "Usage: --n <size> --k <rounds> [--save-memo path] [--load-memo path] [--max-states N] [--time-limit-sec T]\n"; return 1; }
    run_one(n, k); return 0;
}
