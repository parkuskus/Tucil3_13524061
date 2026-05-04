#pragma once

#include <vector>

namespace t3
{

    class Position
    {
    public:
        Position() = default;
        Position(int row, int col);

        int row() const;
        int col() const;
        void setRow(int row);
        void setCol(int col);

    private:
        int row_ = 0;
        int col_ = 0;
    };

    enum class TileType
    {
        Path,
        Wall,
        Lava,
        Start,
        Goal,
        Checkpoint
    };

    class Tile
    {
    public:
        Tile() = default;
        Tile(TileType type, int value = -1);

        TileType type() const;
        int value() const;
        void setType(TileType type);
        void setValue(int value);

    private:
        TileType type_ = TileType::Path;
        int value_ = -1;
    };

    class Board
    {
    public:
        Board() = default;

        void init(int rows, int cols);

        int rows() const;
        int cols() const;
        const std::vector<std::vector<Tile>> &tiles() const;
        const std::vector<std::vector<int>> &costs() const;
        const Position &start() const;
        const Position &goal() const;
        int maxCheckpoint() const;
        const std::vector<Position> &checkpoints() const;

        void setStart(const Position &start);
        void setGoal(const Position &goal);
        void setMaxCheckpoint(int maxCheckpoint);
        void setCheckpoints(const std::vector<Position> &checkpoints);
        void setTile(int row, int col, const Tile &tile);
        void setCost(int row, int col, int cost);

    private:
        int rows_ = 0;
        int cols_ = 0;
        std::vector<std::vector<Tile>> tiles_;
        std::vector<std::vector<int>> costs_;
        Position start_;
        Position goal_;
        int maxCheckpoint_ = -1;
        std::vector<Position> checkpoints_;
    };

} // namespace t3
