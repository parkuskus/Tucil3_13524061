#pragma once

#include <string>
#include <vector>

#include "board/Board.hpp"
#include "search/Search.hpp"

namespace t3
{

    class OutputRenderer
    {
    public:
        std::vector<std::string> renderBoard(const Board &board, const Position &actor, int nextCheckpoint) const;
        void printBoard(const std::vector<std::string> &lines) const;
        void printSolutionSteps(const Board &board, const std::vector<Position> &positions, const std::vector<int> &checkpoints, const std::string &moves) const;

        bool writeSolutionFile(const std::string &path, const Board &board, const std::vector<Position> &positions, const std::vector<int> &checkpoints, const std::string &moves, const SearchResult &result, double elapsedMs) const;
        bool writeIterationFile(const std::string &path, const std::vector<IterationSnapshot> &log) const;

    private:
        char tileChar(const Board &board, int row, int col, int nextCheckpoint) const;
    };

} // namespace t3
