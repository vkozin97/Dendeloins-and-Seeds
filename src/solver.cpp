#include "solver.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <vector>

Solver::Solver(SolverConfig config) : config_(config), geometry_(config.n), memo_(config.n==6) { stats_.n=config.n; stats_.k=config.k; }
SolveResult Solver::solve(){ return solveFromState(State{}); }
SolveResult Solver::solveFromState(const State& root){ start_=std::chrono::steady_clock::now(); stopped_=false; bool fw=canFirstWin(root,0); auto end=std::chrono::steady_clock::now(); stats_.elapsed_ms=std::chrono::duration_cast<std::chrono::milliseconds>(end-start_).count(); stats_.unique_states_stored=memo_.size(); return {fw,stats_,!stopped_}; }
bool Solver::shouldStop() const { if(config_.max_states && memo_.size()>=config_.max_states) return true; if(config_.time_limit_sec>0){ auto d=std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count(); if(d>=config_.time_limit_sec) return true;} return false; }

bool Solver::canFirstWin(const State& original, int depth) {
    if(shouldStop()){ const_cast<Solver*>(this)->stopped_=true; return false; }
    stats_.recursive_calls++; stats_.max_depth=std::max(stats_.max_depth, depth); State state=original; if(config_.use_symmetry){ stats_.canonicalization_calls++; auto c=canonicalize(state, geometry_); state={c.key.dandelions,c.key.occupied,c.key.used_dirs}; }
    PackedKey pkey=packStateKey(state,config_.n);
    if(config_.use_memo){ auto lr=memo_.find(pkey); if(lr!=LookupResult::NotFound){ stats_.memo_hits++; return lr==LookupResult::FoundP1; } }
    if (isTerminalWin(state, geometry_)) { stats_.terminal_win_hits++; if(config_.use_memo) memo_.insert(pkey,true); return true; }
    if (isTerminalLoss(state, config_.k)) { stats_.terminal_loss_hits++; if(config_.use_memo) memo_.insert(pkey,false); return false; }
    std::vector<int> plants; uint64_t pmask=plantableMask(state, geometry_); while(pmask){int c=std::countr_zero(pmask); pmask&=pmask-1; plants.push_back(c);} std::vector<int> dirs0; for(int d=0;d<8;++d) if(!(state.used_dirs&(1u<<d))) dirs0.push_back(d);
    for(int plant: plants){ stats_.plant_moves_considered++; State ap=applyPlant(state,plant); bool wins_all=true; for(int dir:dirs0){ stats_.wind_moves_considered++; State nx=applyWind(ap,dir,geometry_); if(!canFirstWin(nx,depth+1)){ wins_all=false; stats_.wind_cutoffs++; break; } } if(wins_all){ stats_.plant_cutoffs++; if(config_.use_memo) memo_.insert(pkey,true); return true; } }
    if(config_.use_memo) memo_.insert(pkey,false); return false;
}
