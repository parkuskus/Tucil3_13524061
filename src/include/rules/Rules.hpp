#pragma once

#include <string>

#include "board/Board.hpp"
#include "Exception/Exception.hpp"

namespace t3
{

    enum class MoveDir
    {
        Up,
        Down,
        Left,
        Right
    };

    struct MoveOutcome
    {
        Position end;
        bool hitWall = false;
        bool outOfBounds = false;
        bool hitLava = false;
        bool valid = false;
        int nextCheckpoint = 0;
        int moveCost = 0;
    };

    class Rules
    {
    public:
        MoveOutcome applyMove(const Board &board, const Position &start, int checkpointIndex, MoveDir dir) const;

    private:
        bool isInside(const Board &board, int row, int col) const;
        Position step(Position pos, MoveDir dir) const;
        int tileCost(const Board &board, int row, int col) const;
    };

} // namespace t3
