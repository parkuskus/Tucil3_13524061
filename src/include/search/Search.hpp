#pragma once

#include <string>
#include <vector>

#include "board/Board.hpp"
#include "heuristic/Heuristic.hpp"
#include "rules/Rules.hpp"

namespace t3
{

    enum class SearchAlgorithm
    {
        UCS,
        GBFS,
        AStar
    };

    struct SearchConfig
    {
        SearchAlgorithm algorithm = SearchAlgorithm::UCS;
        HeuristicType heuristic = HeuristicType::RemainingPlusManhattan;
        bool logIterations = false;
    };

    struct IterationSnapshot
    {
        int iteration = 0;
        Position pos;
        int nextCheckpoint = 0;
        int gCost = 0;
        int hCost = 0;
        int fCost = 0;
    };

    struct SearchResult
    {
        bool found = false;
        int totalCost = 0;
        int iterations = 0;
        std::string moves;
        std::vector<Position> pathPositions;
        std::vector<int> pathCheckpoints;
        std::vector<IterationSnapshot> iterationLog;
    };

    class Search
    {
    public:
        SearchResult solve(const Board &board, const Rules &rules, const SearchConfig &config) const;

    private:
        char dirToChar(MoveDir dir) const;
        bool isGoalState(const Board &board, const Position &pos, int nextCheckpoint) const;
    };

} // namespace t3
