#pragma once

#include <string>
#include <vector>

#include "board/Board.hpp"
#include "Exception/Exception.hpp"
#include "parsing/Parser.hpp"

namespace t3
{

    class Validator
    {
    public:
        Board validate(const RawInput &raw) const;
    };

} // namespace t3
