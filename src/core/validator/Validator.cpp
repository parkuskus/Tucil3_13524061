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

    Board &ValidationResult::board() { return board_; }
    const Board &ValidationResult::board() const { return board_; }
    const std::vector<std::string> &ValidationResult::errors() const { return errors_; }
    bool ValidationResult::ok() const { return errors_.empty(); }
    void ValidationResult::addError(const std::string &error) { errors_.push_back(error); }

    ValidationResult Validator::validate(const RawInput &raw) const
    {
        ValidationResult result;
        if (raw.rows() <= 0 || raw.cols() <= 0)
        {
            result.addError("Board dimensions must be positive.");
            return result;
        }

        if (static_cast<int>(raw.mapLines().size()) != raw.rows())
        {
            result.addError("Board rows count does not match N.");
            return result;
        }

        if (static_cast<int>(raw.costs().size()) != raw.rows())
        {
            result.addError("Cost rows count does not match N.");
            return result;
        }

        for (int r = 0; r < raw.rows(); ++r)
        {
            if (static_cast<int>(raw.mapLines()[r].size()) != raw.cols())
            {
                result.addError("Board row length mismatch at row " + std::to_string(r) + ".");
            }
            if (static_cast<int>(raw.costs()[r].size()) != raw.cols())
            {
                result.addError("Cost row length mismatch at row " + std::to_string(r) + ".");
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
                    result.addError(std::string("Invalid tile character: ") + ch + " at (" + std::to_string(r) + ", " + std::to_string(c) + ").");
                    continue;
                }

                if (ch == 'Z')
                {
                    ++startCount;
                    result.board().setStart(Position(r, c));
                }
                else if (ch == 'O')
                {
                    ++goalCount;
                    result.board().setGoal(Position(r, c));
                }
                else if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    int value = ch - '0';
                    if (digitPositions.count(value) > 0)
                    {
                        result.addError("Duplicate checkpoint digit: " + std::to_string(value) + ".");
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
            result.addError("Exactly one start tile (Z) is required.");
        }
        if (goalCount != 1)
        {
            result.addError("Exactly one goal tile (O) is required.");
        }

        if (maxDigit >= 0)
        {
            for (int v = 0; v <= maxDigit; ++v)
            {
                if (digitPositions.count(v) == 0)
                {
                    result.addError("Checkpoint sequence must be contiguous from 0 to " + std::to_string(maxDigit) + ". Missing " + std::to_string(v) + ".");
                    break;
                }
            }
        }

        if (!result.ok())
        {
            return result;
        }

        result.board().init(raw.rows(), raw.cols());
        result.board().setMaxCheckpoint(maxDigit);
        if (maxDigit >= 0)
        {
            std::vector<Position> checkpoints(static_cast<size_t>(maxDigit + 1));
            for (int v = 0; v <= maxDigit; ++v)
            {
                checkpoints[static_cast<size_t>(v)] = digitPositions[v];
            }
            result.board().setCheckpoints(checkpoints);
        }

        for (int r = 0; r < raw.rows(); ++r)
        {
            for (int c = 0; c < raw.cols(); ++c)
            {
                result.board().setCost(r, c, raw.costs()[r][c]);
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
                result.board().setTile(r, c, tile);
            }
        }

        return result;
    }

} // namespace t3
