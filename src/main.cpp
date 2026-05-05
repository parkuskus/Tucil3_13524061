#include <algorithm>
#include <chrono>
#include <cctype>
#include <limits>
#include <iostream>

#include "heuristic/Heuristic.hpp"
#include "output/Output.hpp"
#include "parsing/Parser.hpp"
#include "playback/Playback.hpp"
#include "rules/Rules.hpp"
#include "search/Search.hpp"
#include "validator/Validator.hpp"

namespace
{

    std::string toLower(const std::string &text)
    {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });
        return result;
    }

    bool parseAlgorithm(const std::string &value, t3::SearchAlgorithm &out)
    {
        std::string key = toLower(value);
        if (key == "ucs")
        {
            out = t3::SearchAlgorithm::UCS;
            return true;
        }
        if (key == "gbfs")
        {
            out = t3::SearchAlgorithm::GBFS;
            return true;
        }
        if (key == "astar" || key == "a*")
        {
            out = t3::SearchAlgorithm::AStar;
            return true;
        }
        return false;
    }

    bool parseHeuristic(const std::string &value, t3::HeuristicType &out)
    {
        std::string key = toLower(value);
        if (key == "h1" || key == "manhattan")
        {
            out = t3::HeuristicType::ManhattanToTarget;
            return true;
        }
        if (key == "h2" || key == "remaining")
        {
            out = t3::HeuristicType::RemainingPlusManhattan;
            return true;
        }
        if (key == "h3" || key == "weighted")
        {
            out = t3::HeuristicType::WeightedRemainingToGoal;
            return true;
        }
        return false;
    }

    bool isYes(const std::string &value)
    {
        std::string key = toLower(value);
        return key == "ya" || key == "y" || key == "yes";
    }

    std::string readLine()
    {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    void discardLine()
    {
        if (std::cin.fail())
        {
            std::cin.clear();
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

} // namespace

int main(int argc, char **argv)
{
    std::string path;
    if (argc >= 2)
    {
        path = argv[1];
    }
    else
    {
        std::cout << ">> Masukan file input :\n";
        path = readLine();
    }

    if (path.empty())
    {
        std::cout << "Input file path is required.\n";
        return 1;
    }
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

        t3::SearchAlgorithm algorithm = t3::SearchAlgorithm::UCS;
        t3::HeuristicType heuristicType = t3::HeuristicType::RemainingPlusManhattan;

        std::cout << ">> Algoritma apa yang anda pilih? (UCS/GBFS/A*)\n";
        std::string algoInput = readLine();
        if (!parseAlgorithm(algoInput, algorithm))
        {
            std::cout << "Algoritma tidak dikenali.\n";
            return 1;
        }

        if (algorithm != t3::SearchAlgorithm::UCS)
        {
            std::cout << ">> Heuristic apa yang anda pilih? (H1/H2/H3)\n";
            std::string heuristicInput = readLine();
            if (!parseHeuristic(heuristicInput, heuristicType))
            {
                std::cout << "Heuristic tidak dikenali.\n";
                return 1;
            }
        }

        t3::Rules rules;
        t3::Search search;
        t3::SearchConfig config;
        config.algorithm = algorithm;
        config.heuristic = heuristicType;
        config.logIterations = true;

        auto startTime = std::chrono::steady_clock::now();
        t3::SearchResult result = search.solve(board, rules, config);
        auto endTime = std::chrono::steady_clock::now();
        double elapsedMs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;

        t3::OutputRenderer renderer;
        if (result.found)
        {
            std::cout << "Solusi Yang Ditemukan : " << result.moves << "\n";
            std::cout << "Cost dari Solusi : " << result.totalCost << "\n";
            renderer.printSolutionSteps(board, result.pathPositions, result.pathCheckpoints, result.moves);
        }
        else
        {
            std::cout << "Solusi tidak ditemukan.\n";
        }

        std::cout << ">> Waktu eksekusi: " << elapsedMs << " ms\n";
        std::cout << ">> Banyak iterasi yang dilakukan: " << result.iterations << " iterasi\n";

        if (result.found)
        {
            std::cout << ">> Apakah Anda ingin melakukan playback? (Ya/Tidak) :\n";
            std::string playbackInput = readLine();
            if (isYes(playbackInput))
            {
                std::cout << ">> Pada step berapa anda ingin melakukan playback :\n";
                size_t startIndex = 0;
                if (!(std::cin >> startIndex))
                {
                    startIndex = 0;
                }
                discardLine();
                t3::Playback playback;
                playback.run(board, result.pathPositions, result.pathCheckpoints, renderer, startIndex);
            }
        }

        std::cout << ">> Apakah Anda ingin menyimpan solusi? (Ya/Tidak) :\n";
        std::string saveSolutionInput = readLine();
        if (isYes(saveSolutionInput))
        {
            std::cout << ">> Solusi disimpan pada :\n";
            std::string solutionPath = readLine();
            if (!solutionPath.empty())
            {
                if (!renderer.writeSolutionFile(solutionPath, result, elapsedMs))
                {
                    std::cout << "Gagal menyimpan solusi pada: " << solutionPath << "\n";
                }
            }
        }

        std::cout << ">> Apakah Anda ingin menyimpan iterasi? (Ya/Tidak) :\n";
        std::string saveIterInput = readLine();
        if (isYes(saveIterInput))
        {
            std::cout << ">> Iterasi disimpan pada :\n";
            std::string iterationsPath = readLine();
            if (!iterationsPath.empty())
            {
                if (!renderer.writeIterationFile(iterationsPath, result.iterationLog))
                {
                    std::cout << "Gagal menyimpan iterasi pada: " << iterationsPath << "\n";
                }
            }
        }
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
