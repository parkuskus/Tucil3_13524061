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

    RawInput Parser::parseFile(const std::string &path) const
    {
        RawInput result;
        std::ifstream input(path);
        if (!input)
        {
            throw IOException("Failed to open input file: " + path);
        }

        int rows = 0;
        int cols = 0;
        if (!(input >> rows >> cols))
        {
            throw ParseException("Failed to read board dimensions (N M).");
        }

        if (rows <= 0 || cols <= 0)
        {
            throw ParseException("Board dimensions must be positive.");
        }

        result.setDimensions(rows, cols);

        for (int r = 0; r < rows; ++r)
        {
            std::string row;
            if (!(input >> row))
            {
                throw ParseException("Missing board row at index " + std::to_string(r) + ".");
            }
            result.addMapLine(row);
        }

        result.initCosts(rows, cols);
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int value = 0;
                if (!(input >> value))
                {
                    throw ParseException("Missing cost value at row " + std::to_string(r) + ", col " + std::to_string(c) + ".");
                }
                result.setCost(r, c, value);
            }
        }

        return result;
    }

} // namespace t3
