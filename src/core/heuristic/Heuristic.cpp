#include "heuristic/Heuristic.hpp"

#include <algorithm>
#include <cstdlib>

namespace t3
{

    int Heuristic::estimate(const Board &board, const Position &pos, int nextCheckpoint, HeuristicType type) const
    {
        if (type == HeuristicType::RemainingPlusManhattan)
        {
            Position target = resolveTarget(board, nextCheckpoint);
            int remaining = remainingCheckpoints(board, nextCheckpoint);
            return remaining + manhattan(pos, target);
        }
        return 0;
    }

    int Heuristic::manhattan(const Position &a, const Position &b) const
    {
        return std::abs(a.row() - b.row()) + std::abs(a.col() - b.col());
    }

    Position Heuristic::resolveTarget(const Board &board, int nextCheckpoint) const
    {
        if (nextCheckpoint <= board.maxCheckpoint() && nextCheckpoint >= 0)
        {
            return board.checkpoints()[static_cast<size_t>(nextCheckpoint)];
        }
        return board.goal();
    }

    int Heuristic::remainingCheckpoints(const Board &board, int nextCheckpoint) const
    {
        if (board.maxCheckpoint() < 0)
        {
            return 0;
        }
        int remaining = (board.maxCheckpoint() + 1) - nextCheckpoint;
        return std::max(0, remaining);
    }

} // namespace t3
