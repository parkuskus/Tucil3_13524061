#pragma once

#include <string>
#include <vector>

#include "board/Board.hpp"
#include "parsing/Parser.hpp"

namespace t3
{

    class ValidationResult
    {
    public:
        Board &board();
        const Board &board() const;
        const std::vector<std::string> &errors() const;
        bool ok() const;
        void addError(const std::string &error);

    private:
        Board board_;
        std::vector<std::string> errors_;
    };

    class Validator
    {
    public:
        ValidationResult validate(const RawInput &raw) const;
    };

} // namespace t3
