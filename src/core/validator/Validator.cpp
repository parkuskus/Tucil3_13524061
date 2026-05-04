#include "validator/Validator.hpp"

#include <cctype>
#include <map>

namespace t3
{

    namespace
    {

        bool isAllowedChar(char ch)
        {
            return ch == '*' || ch == 'X' || ch == 'L' || ch == 'Z' || ch == 'O' || std::isdigit(static_cast<unsigned char>(ch));
        }

    } // namespace

    Board Validator::validate(const RawInput &raw) const
    {
        Board board;
        if (raw.rows() <= 0 || raw.cols() <= 0)
        {
            throw ValidationException("Board dimensions must be positive.");
        }

        if (static_cast<int>(raw.mapLines().size()) != raw.rows())
        {
            throw ValidationException("Board rows count does not match N.");
        }

        if (static_cast<int>(raw.costs().size()) != raw.rows())
        {
            throw ValidationException("Cost rows count does not match N.");
        }

        for (int r = 0; r < raw.rows(); ++r)
        {
            if (static_cast<int>(raw.mapLines()[r].size()) != raw.cols())
            {
                throw ValidationException("Board row length mismatch at row " + std::to_string(r) + ".");
            }
            if (static_cast<int>(raw.costs()[r].size()) != raw.cols())
            {
                throw ValidationException("Cost row length mismatch at row " + std::to_string(r) + ".");
            }
        }

        int startCount = 0;
        int goalCount = 0;
        int maxDigit = -1;
        std::map<int, Position> digitPositions;

        for (int r = 0; r < raw.rows(); ++r)
        {
            for (int c = 0; c < raw.cols(); ++c)
            {
                char ch = raw.mapLines()[r][c];
                if (!isAllowedChar(ch))
                {
                    throw ValidationException(std::string("Invalid tile character: ") + ch + " at (" + std::to_string(r) + ", " + std::to_string(c) + ").");
                }

                if (ch == 'Z')
                {
                    ++startCount;
                    board.setStart(Position(r, c));
                }
                else if (ch == 'O')
                {
                    ++goalCount;
                    board.setGoal(Position(r, c));
                }
                else if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    int value = ch - '0';
                    if (digitPositions.count(value) > 0)
                    {
                        throw ValidationException("Duplicate checkpoint digit: " + std::to_string(value) + ".");
                    }
                    else
                    {
                        digitPositions[value] = Position(r, c);
                    }
                    if (value > maxDigit)
                    {
                        maxDigit = value;
                    }
                }
            }
        }

        if (startCount != 1)
        {
            throw ValidationException("Exactly one start tile (Z) is required.");
        }
        if (goalCount != 1)
        {
            throw ValidationException("Exactly one goal tile (O) is required.");
        }

        if (maxDigit >= 0)
        {
            for (int v = 0; v <= maxDigit; ++v)
            {
                if (digitPositions.count(v) == 0)
                {
                    throw ValidationException("Checkpoint sequence must be contiguous from 0 to " + std::to_string(maxDigit) + ". Missing " + std::to_string(v) + ".");
                }
            }
        }

        board.init(raw.rows(), raw.cols());
        board.setMaxCheckpoint(maxDigit);
        if (maxDigit >= 0)
        {
            std::vector<Position> checkpoints(static_cast<size_t>(maxDigit + 1));
            for (int v = 0; v <= maxDigit; ++v)
            {
                checkpoints[static_cast<size_t>(v)] = digitPositions[v];
            }
            board.setCheckpoints(checkpoints);
        }

        for (int r = 0; r < raw.rows(); ++r)
        {
            for (int c = 0; c < raw.cols(); ++c)
            {
                board.setCost(r, c, raw.costs()[r][c]);
                char ch = raw.mapLines()[r][c];
                Tile tile;
                if (ch == '*')
                {
                    tile.setType(TileType::Path);
                }
                else if (ch == 'X')
                {
                    tile.setType(TileType::Wall);
                }
                else if (ch == 'L')
                {
                    tile.setType(TileType::Lava);
                }
                else if (ch == 'Z')
                {
                    tile.setType(TileType::Start);
                }
                else if (ch == 'O')
                {
                    tile.setType(TileType::Goal);
                }
                else if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    tile.setType(TileType::Checkpoint);
                    tile.setValue(ch - '0');
                }
                board.setTile(r, c, tile);
            }
        }

        return board;
    }

} // namespace t3
