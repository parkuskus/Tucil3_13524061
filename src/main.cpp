#include <iostream>

#include "parsing/Parser.hpp"
#include "validator/Validator.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: solver <input.txt>\n";
        return 1;
    }

    std::string path = argv[1];
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
    {
        path = std::string("test/") + path;
    }

    try
    {
        t3::Parser parser;
        t3::RawInput parsed = parser.parseFile(path);
        t3::Validator validator;
        t3::Board board = validator.validate(parsed);

        std::cout << "Input OK: " << board.rows() << "x" << board.cols() << "\n";
        std::cout << "Checkpoint max: " << board.maxCheckpoint() << "\n";
    }
    catch (const t3::AppException &ex)
    {
        std::cout << "Input error: " << ex.what() << "\n";
        return 1;
    }
    catch (const std::exception &ex)
    {
        std::cout << "Unexpected error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
