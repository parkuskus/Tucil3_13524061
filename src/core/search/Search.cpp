#include "search/Search.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_map>

namespace t3
{

    namespace
    {

        struct StateKey
        {
            int row = 0;
            int col = 0;
            int nextCheckpoint = 0;

            bool operator==(const StateKey &other) const
            {
                return row == other.row && col == other.col && nextCheckpoint == other.nextCheckpoint;
            }
        };

        struct StateKeyHash
        {
            std::size_t operator()(const StateKey &key) const
            {
                std::size_t h1 = static_cast<std::size_t>(key.row);
                std::size_t h2 = static_cast<std::size_t>(key.col);
                std::size_t h3 = static_cast<std::size_t>(key.nextCheckpoint);
                return (h1 * 1315423911u) ^ (h2 * 2654435761u) ^ (h3 * 97531u);
            }
        };

        struct Node
        {
            Position pos;
            int nextCheckpoint = 0;
            int gCost = 0;
            int hCost = 0;
            int parentIndex = -1;
            MoveDir move = MoveDir::Up;
        };

        struct QueueItem
        {
            int nodeIndex = 0;
            int priority = 0;
            int gCost = 0;
            int order = 0;
        };

        struct QueueCompare
        {
            bool operator()(const QueueItem &a, const QueueItem &b) const
            {
                if (a.priority != b.priority)
                {
                    return a.priority > b.priority;
                }
                if (a.gCost != b.gCost)
                {
                    return a.gCost > b.gCost;
                }
                return a.order > b.order;
            }
        };

        int computePriority(SearchAlgorithm algorithm, int gCost, int hCost)
        {
            if (algorithm == SearchAlgorithm::UCS)
            {
                return gCost;
            }
            if (algorithm == SearchAlgorithm::GBFS)
            {
                return hCost;
            }
            return gCost + hCost;
        }

        StateKey makeKey(const Position &pos, int nextCheckpoint)
        {
            StateKey key;
            key.row = pos.row();
            key.col = pos.col();
            key.nextCheckpoint = nextCheckpoint;
            return key;
        }

        bool samePos(const Position &a, const Position &b)
        {
            return a.row() == b.row() && a.col() == b.col();
        }

    } // namespace

    SearchResult Search::solve(const Board &board, const Rules &rules, const SearchConfig &config) const
    {
        SearchResult result;
        Heuristic heuristic;
        std::vector<Node> nodes;
        nodes.reserve(1024);

        std::priority_queue<QueueItem, std::vector<QueueItem>, QueueCompare> frontier;
        std::unordered_map<StateKey, int, StateKeyHash> bestCost;
        int orderCounter = 0;

        Node startNode;
        startNode.pos = board.start();
        startNode.nextCheckpoint = 0;
        startNode.gCost = 0;
        startNode.hCost = heuristic.estimate(board, startNode.pos, startNode.nextCheckpoint, config.heuristic);
        nodes.push_back(startNode);

        int startPriority = computePriority(config.algorithm, startNode.gCost, startNode.hCost);
        frontier.push(QueueItem{0, startPriority, startNode.gCost, orderCounter++});
        bestCost[makeKey(startNode.pos, startNode.nextCheckpoint)] = startNode.gCost;

        const MoveDir directions[] = {MoveDir::Up, MoveDir::Down, MoveDir::Left, MoveDir::Right};

        while (!frontier.empty())
        {
            QueueItem currentItem = frontier.top();
            frontier.pop();

            const Node &current = nodes[static_cast<size_t>(currentItem.nodeIndex)];
            StateKey currentKey = makeKey(current.pos, current.nextCheckpoint);

            auto bestIt = bestCost.find(currentKey);
            if (bestIt != bestCost.end() && current.gCost > bestIt->second)
            {
                continue;
            }

            ++result.iterations;
            if (config.logIterations)
            {
                IterationSnapshot snap;
                snap.iteration = result.iterations;
                snap.pos = current.pos;
                snap.nextCheckpoint = current.nextCheckpoint;
                snap.gCost = current.gCost;
                snap.hCost = current.hCost;
                snap.fCost = computePriority(config.algorithm, current.gCost, current.hCost);
                result.iterationLog.push_back(snap);
            }

            if (isGoalState(board, current.pos, current.nextCheckpoint))
            {
                result.found = true;
                result.totalCost = current.gCost;

                std::vector<int> indexPath;
                int index = currentItem.nodeIndex;
                while (index >= 0)
                {
                    indexPath.push_back(index);
                    index = nodes[static_cast<size_t>(index)].parentIndex;
                }
                std::reverse(indexPath.begin(), indexPath.end());

                result.pathPositions.reserve(indexPath.size());
                result.pathCheckpoints.reserve(indexPath.size());

                for (size_t i = 0; i < indexPath.size(); ++i)
                {
                    const Node &node = nodes[static_cast<size_t>(indexPath[i])];
                    result.pathPositions.push_back(node.pos);
                    result.pathCheckpoints.push_back(node.nextCheckpoint);
                    if (i > 0)
                    {
                        result.moves.push_back(dirToChar(node.move));
                    }
                }
                return result;
            }

            for (MoveDir dir : directions)
            {
                MoveOutcome outcome = rules.applyMove(board, current.pos, current.nextCheckpoint, dir);
                if (!outcome.valid || outcome.hitLava || outcome.outOfBounds)
                {
                    continue;
                }

                if (isGoalState(board, outcome.end, outcome.nextCheckpoint))
                {
                    Node goalNode;
                    goalNode.pos = outcome.end;
                    goalNode.nextCheckpoint = outcome.nextCheckpoint;
                    goalNode.gCost = current.gCost + outcome.moveCost;
                    goalNode.hCost = 0;
                    goalNode.parentIndex = currentItem.nodeIndex;
                    goalNode.move = dir;
                    nodes.push_back(goalNode);

                    int goalIndex = static_cast<int>(nodes.size() - 1);
                    frontier.push(QueueItem{goalIndex, 0, goalNode.gCost, orderCounter++});
                    bestCost[makeKey(goalNode.pos, goalNode.nextCheckpoint)] = goalNode.gCost;
                    continue;
                }

                if (samePos(outcome.end, board.goal()) && outcome.nextCheckpoint <= board.maxCheckpoint())
                {
                    continue;
                }

                Node nextNode;
                nextNode.pos = outcome.end;
                nextNode.nextCheckpoint = outcome.nextCheckpoint;
                nextNode.gCost = current.gCost + outcome.moveCost;
                nextNode.hCost = heuristic.estimate(board, nextNode.pos, nextNode.nextCheckpoint, config.heuristic);
                nextNode.parentIndex = currentItem.nodeIndex;
                nextNode.move = dir;

                StateKey nextKey = makeKey(nextNode.pos, nextNode.nextCheckpoint);
                auto existing = bestCost.find(nextKey);
                if (existing != bestCost.end() && nextNode.gCost >= existing->second)
                {
                    continue;
                }
                bestCost[nextKey] = nextNode.gCost;

                nodes.push_back(nextNode);
                int nextIndex = static_cast<int>(nodes.size() - 1);
                int priority = computePriority(config.algorithm, nextNode.gCost, nextNode.hCost);
                frontier.push(QueueItem{nextIndex, priority, nextNode.gCost, orderCounter++});
            }
        }

        return result;
    }

    char Search::dirToChar(MoveDir dir) const
    {
        if (dir == MoveDir::Up)
        {
            return 'U';
        }
        if (dir == MoveDir::Down)
        {
            return 'D';
        }
        if (dir == MoveDir::Left)
        {
            return 'L';
        }
        return 'R';
    }

    bool Search::isGoalState(const Board &board, const Position &pos, int nextCheckpoint) const
    {
        if (pos.row() != board.goal().row() || pos.col() != board.goal().col())
        {
            return false;
        }
        return nextCheckpoint > board.maxCheckpoint();
    }

} // namespace t3
