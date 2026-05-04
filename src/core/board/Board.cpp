#include "board/Board.hpp"

namespace t3
{

    Position::Position(int row, int col) : row_(row), col_(col) {}

    int Position::row() const { return row_; }
    int Position::col() const { return col_; }
    void Position::setRow(int row) { row_ = row; }
    void Position::setCol(int col) { col_ = col; }

    Tile::Tile(TileType type, int value) : type_(type), value_(value) {}

    TileType Tile::type() const { return type_; }
    int Tile::value() const { return value_; }
    void Tile::setType(TileType type) { type_ = type; }
    void Tile::setValue(int value) { value_ = value; }

    void Board::init(int rows, int cols)
    {
        rows_ = rows;
        cols_ = cols;
        tiles_.assign(rows, std::vector<Tile>(cols));
        costs_.assign(rows, std::vector<int>(cols, 0));
        checkpoints_.clear();
        maxCheckpoint_ = -1;
    }

    int Board::rows() const { return rows_; }
    int Board::cols() const { return cols_; }
    const std::vector<std::vector<Tile>> &Board::tiles() const { return tiles_; }
    const std::vector<std::vector<int>> &Board::costs() const { return costs_; }
    const Position &Board::start() const { return start_; }
    const Position &Board::goal() const { return goal_; }
    int Board::maxCheckpoint() const { return maxCheckpoint_; }
    const std::vector<Position> &Board::checkpoints() const { return checkpoints_; }

    void Board::setStart(const Position &start) { start_ = start; }
    void Board::setGoal(const Position &goal) { goal_ = goal; }
    void Board::setMaxCheckpoint(int maxCheckpoint) { maxCheckpoint_ = maxCheckpoint; }
    void Board::setCheckpoints(const std::vector<Position> &checkpoints) { checkpoints_ = checkpoints; }

    void Board::setTile(int row, int col, const Tile &tile)
    {
        if (row < 0 || col < 0 || row >= rows_ || col >= cols_)
        {
            return;
        }
        tiles_[row][col] = tile;
    }

    void Board::setCost(int row, int col, int cost)
    {
        if (row < 0 || col < 0 || row >= rows_ || col >= cols_)
        {
            return;
        }
        costs_[row][col] = cost;
    }

} // namespace t3
