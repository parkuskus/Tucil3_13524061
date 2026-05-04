#include "rules/Rules.hpp"

namespace t3
{

    bool Rules::isInside(const Board &board, int row, int col) const
    {
        return row >= 0 && col >= 0 && row < board.rows() && col < board.cols();
    }

    Position Rules::step(Position pos, MoveDir dir) const
    {
        if (dir == MoveDir::Up)
        {
            pos.setRow(pos.row() - 1);
        }
        else if (dir == MoveDir::Down)
        {
            pos.setRow(pos.row() + 1);
        }
        else if (dir == MoveDir::Left)
        {
            pos.setCol(pos.col() - 1);
        }
        else if (dir == MoveDir::Right)
        {
            pos.setCol(pos.col() + 1);
        }
        return pos;
    }

    int Rules::tileCost(const Board &board, int row, int col) const
    {
        return board.costs()[row][col];
    }

    MoveOutcome Rules::applyMove(const Board &board, const Position &start, int checkpointIndex, MoveDir dir) const
    {
        MoveOutcome result;
        result.end = start;
        result.nextCheckpoint = checkpointIndex;

        Position current = start;
        bool moved = false;

        while (true)
        {
            Position next = step(current, dir);
            if (!isInside(board, next.row(), next.col()))
            {
                result.outOfBounds = true;
                return result;
            }

            const Tile &tile = board.tiles()[next.row()][next.col()];
            if (tile.type() == TileType::Wall)
            {
                result.hitWall = true;
                result.end = current;
                result.valid = moved;
                return result;
            }

            moved = true;
            result.moveCost += tileCost(board, next.row(), next.col());
            current = next;

            if (tile.type() == TileType::Lava)
            {
                result.hitLava = true;
                return result;
            }

            if (tile.type() == TileType::Checkpoint)
            {
                if (tile.value() == checkpointIndex)
                {
                    result.nextCheckpoint = checkpointIndex + 1;
                }
                else if (tile.value() > checkpointIndex)
                {
                    return result;
                }
            }
        }
    }

} // namespace t3
