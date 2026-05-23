#pragma once
#include <optional>
#include <string>
#include <vector>
#include "canonical.h"
#include "memo_file.h"
#include "solver.h"

enum class Phase { P1_TO_PLANT, P2_TO_BLOW };
enum class PlayerResult { P1, P2, Unknown };

struct MoveEvaluation {
    std::string move_label;
    PlayerResult result = PlayerResult::Unknown;
    std::vector<int> refuting_dirs;
    std::vector<int> unknown_dirs;
};

class SolverOracle {
public:
    explicit SolverOracle(SolverConfig cfg);
    bool loadMemo(const std::string& path, std::string& err);
    bool saveMemo(const std::string& path, bool complete=true, uint8_t root_result=255) const;

    PlayerResult evaluateP1ToMoveState(const State& state) const;
    PlayerResult solveP1ToMoveState(const State& state);

    PlayerResult evaluateP2ToBlowState(const State& state) const;
    PlayerResult solveP2ToBlowState(const State& state);

    std::vector<MoveEvaluation> listP1Moves(const State& state) const;
    std::vector<MoveEvaluation> listP2Moves(const State& state) const;

    const Geometry& geometry() const { return geometry_; }
    const SolverConfig& config() const { return cfg_; }
private:
    PackedKey canonicalPacked(const State& s) const;
    SolverConfig cfg_;
    Geometry geometry_;
    Solver solver_;
};
