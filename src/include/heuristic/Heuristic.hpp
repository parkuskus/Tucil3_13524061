#pragma once

#include "board/Board.hpp"

namespace t3
{

    enum class HeuristicType
    {
        ManhattanToTarget,
        RemainingPlusManhattan,
        WeightedRemainingToGoal
    };

    class Heuristic
    {
    public:
        int estimate(const Board &board, const Position &pos, int nextCheckpoint, HeuristicType type) const;

    private:
        int manhattan(const Position &a, const Position &b) const;
        Position resolveTarget(const Board &board, int nextCheckpoint) const;
        int remainingCheckpoints(const Board &board, int nextCheckpoint) const;
        int weightedRemainingToGoal(const Board &board, const Position &pos, int nextCheckpoint) const;
    };

} // namespace t3
