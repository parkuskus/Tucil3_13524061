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

    const std::string path = argv[1];
    t3::Parser parser;
    t3::ParseResult parsed = parser.parseFile(path);
    if (!parsed.ok())
    {
        std::cout << "Input parse failed:\n";
        for (const auto &error : parsed.errors())
        {
            std::cout << "- " << error << "\n";
        }
        return 1;
    }

    t3::Validator validator;
    t3::ValidationResult validated = validator.validate(parsed.raw());
    if (!validated.ok())
    {
        std::cout << "Input validation failed:\n";
        for (const auto &error : validated.errors())
        {
            std::cout << "- " << error << "\n";
        }
        return 1;
    }

    std::cout << "Input OK: " << validated.board().rows() << "x" << validated.board().cols() << "\n";
    std::cout << "Checkpoint max: " << validated.board().maxCheckpoint() << "\n";
    return 0;
}
