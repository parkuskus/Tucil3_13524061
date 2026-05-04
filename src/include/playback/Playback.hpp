#pragma once

#include <vector>

#include "board/Board.hpp"
#include "output/Output.hpp"

namespace t3
{

    class Playback
    {
    public:
        void run(const Board &board, const std::vector<Position> &positions, const std::vector<int> &checkpoints, const OutputRenderer &renderer, size_t startIndex) const;
    };

} // namespace t3
