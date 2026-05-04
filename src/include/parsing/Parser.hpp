#pragma once

#include <string>
#include <vector>

namespace t3
{

    class RawInput
    {
    public:
        int rows() const;
        int cols() const;
        const std::vector<std::string> &mapLines() const;
        const std::vector<std::vector<int>> &costs() const;

        void setDimensions(int rows, int cols);
        void addMapLine(const std::string &line);
        void initCosts(int rows, int cols);
        void setCost(int row, int col, int value);

    private:
        int rows_ = 0;
        int cols_ = 0;
        std::vector<std::string> mapLines_;
        std::vector<std::vector<int>> costs_;
    };

    class ParseResult
    {
    public:
        RawInput &raw();
        const RawInput &raw() const;
        const std::vector<std::string> &errors() const;
        bool ok() const;
        void addError(const std::string &error);

    private:
        RawInput raw_;
        std::vector<std::string> errors_;
    };

    class Parser
    {
    public:
        ParseResult parseFile(const std::string &path) const;
    };

} // namespace t3
