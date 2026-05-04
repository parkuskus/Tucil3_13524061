#include "parsing/Parser.hpp"

#include <fstream>

namespace t3
{

    int RawInput::rows() const { return rows_; }
    int RawInput::cols() const { return cols_; }
    const std::vector<std::string> &RawInput::mapLines() const { return mapLines_; }
    const std::vector<std::vector<int>> &RawInput::costs() const { return costs_; }

    void RawInput::setDimensions(int rows, int cols)
    {
        rows_ = rows;
        cols_ = cols;
    }

    void RawInput::addMapLine(const std::string &line)
    {
        mapLines_.push_back(line);
    }

    void RawInput::initCosts(int rows, int cols)
    {
        costs_.assign(rows, std::vector<int>(cols, 0));
    }

    void RawInput::setCost(int row, int col, int value)
    {
        if (row < 0 || col < 0 || row >= rows_ || col >= cols_)
        {
            return;
        }
        costs_[row][col] = value;
    }

    RawInput &ParseResult::raw() { return raw_; }
    const RawInput &ParseResult::raw() const { return raw_; }
    const std::vector<std::string> &ParseResult::errors() const { return errors_; }
    bool ParseResult::ok() const { return errors_.empty(); }
    void ParseResult::addError(const std::string &error) { errors_.push_back(error); }

    ParseResult Parser::parseFile(const std::string &path) const
    {
        ParseResult result;
        std::ifstream input(path);
        if (!input)
        {
            result.addError("Failed to open input file: " + path);
            return result;
        }

        int rows = 0;
        int cols = 0;
        if (!(input >> rows >> cols))
        {
            result.addError("Failed to read board dimensions (N M).");
            return result;
        }

        if (rows <= 0 || cols <= 0)
        {
            result.addError("Board dimensions must be positive.");
            return result;
        }

        result.raw().setDimensions(rows, cols);

        for (int r = 0; r < rows; ++r)
        {
            std::string row;
            if (!(input >> row))
            {
                result.addError("Missing board row at index " + std::to_string(r) + ".");
                return result;
            }
            result.raw().addMapLine(row);
        }

        result.raw().initCosts(rows, cols);
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int value = 0;
                if (!(input >> value))
                {
                    result.addError("Missing cost value at row " + std::to_string(r) + ", col " + std::to_string(c) + ".");
                    return result;
                }
                result.raw().setCost(r, c, value);
            }
        }

        return result;
    }

} // namespace t3
