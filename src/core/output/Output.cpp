#include "output/Output.hpp"

#include <fstream>
#include <iostream>

namespace t3
{

    std::vector<std::string> OutputRenderer::renderBoard(const Board &board, const Position &actor, int nextCheckpoint) const
    {
        std::vector<std::string> lines;
        lines.reserve(static_cast<size_t>(board.rows()));

        for (int r = 0; r < board.rows(); ++r)
        {
            std::string line;
            line.reserve(static_cast<size_t>(board.cols()));
            for (int c = 0; c < board.cols(); ++c)
            {
                if (actor.row() == r && actor.col() == c)
                {
                    line.push_back('Z');
                }
                else
                {
                    line.push_back(tileChar(board, r, c, nextCheckpoint));
                }
            }
            lines.push_back(line);
        }
        return lines;
    }

    void OutputRenderer::printBoard(const std::vector<std::string> &lines) const
    {
        for (const auto &line : lines)
        {
            std::cout << line << "\n";
        }
    }

    void OutputRenderer::printSolutionSteps(const Board &board, const std::vector<Position> &positions, const std::vector<int> &checkpoints, const std::string &moves) const
    {
        if (positions.empty())
        {
            return;
        }
        std::cout << "Initial\n";
        printBoard(renderBoard(board, positions[0], checkpoints[0]));
        std::cout << "\n";

        for (size_t i = 1; i < positions.size(); ++i)
        {
            char moveChar = (i - 1 < moves.size()) ? moves[i - 1] : '?';
            std::cout << "Step " << i << " : " << moveChar << "\n";
            printBoard(renderBoard(board, positions[i], checkpoints[i]));
            std::cout << "\n";
        }
    }

    bool OutputRenderer::writeSolutionFile(const std::string &path, const SearchResult &result, double elapsedMs) const
    {
        std::ofstream output(path);
        if (!output)
        {
            return false;
        }

        if (!result.found)
        {
            output << "No solution found.\n";
            output << "Iterations: " << result.iterations << "\n";
            output << "Time (ms): " << elapsedMs << "\n";
            return true;
        }

        output << "Solution moves: " << result.moves << "\n";
        output << "Total cost: " << result.totalCost << "\n";
        output << "Iterations: " << result.iterations << "\n";
        output << "Time (ms): " << elapsedMs << "\n";
        return true;
    }

    bool OutputRenderer::writeIterationFile(const std::string &path, const std::vector<IterationSnapshot> &log) const
    {
        std::ofstream output(path);
        if (!output)
        {
            return false;
        }

        output << "iteration row col next_checkpoint g h f\n";
        for (const auto &snap : log)
        {
            output << snap.iteration << " " << snap.pos.row() << " " << snap.pos.col() << " " << snap.nextCheckpoint
                   << " " << snap.gCost << " " << snap.hCost << " " << snap.fCost << "\n";
        }
        return true;
    }

    char OutputRenderer::tileChar(const Board &board, int row, int col, int nextCheckpoint) const
    {
        const Tile &tile = board.tiles()[row][col];
        switch (tile.type())
        {
        case TileType::Path:
            return '*';
        case TileType::Wall:
            return 'X';
        case TileType::Lava:
            return 'L';
        case TileType::Start:
            return '*';
        case TileType::Goal:
            return 'O';
        case TileType::Checkpoint:
            if (tile.value() < nextCheckpoint)
            {
                return '*';
            }
            return static_cast<char>('0' + tile.value());
        default:
            return '*';
        }
    }

} // namespace t3
