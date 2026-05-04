#include "playback/Playback.hpp"

#include <algorithm>
#include <iostream>
#include <limits>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace t3
{

    namespace
    {
        enum class InputKey
        {
            Left,
            Right,
            Esc,
            Other
        };

        struct KeyInput
        {
            InputKey key = InputKey::Other;
            char ch = '\0';
        };

#ifdef _WIN32
        KeyInput readKey()
        {
            int first = _getch();
            if (first == 0 || first == 224)
            {
                int second = _getch();
                if (second == 75)
                {
                    return {InputKey::Left, '\0'};
                }
                if (second == 77)
                {
                    return {InputKey::Right, '\0'};
                }
                return {InputKey::Other, '\0'};
            }
            if (first == 27)
            {
                return {InputKey::Esc, '\0'};
            }
            return {InputKey::Other, static_cast<char>(first)};
        }
#else
        KeyInput readKey()
        {
            termios oldt;
            termios newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);

            int ch = getchar();
            if (ch == 27)
            {
                int ch1 = getchar();
                int ch2 = getchar();
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                if (ch1 == '[' && ch2 == 'D')
                {
                    return {InputKey::Left, '\0'};
                }
                if (ch1 == '[' && ch2 == 'C')
                {
                    return {InputKey::Right, '\0'};
                }
                return {InputKey::Esc, '\0'};
            }

            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return {InputKey::Other, static_cast<char>(ch)};
        }
#endif

    } // namespace

    void Playback::run(const Board &board, const std::vector<Position> &positions, const std::vector<int> &checkpoints, const OutputRenderer &renderer, size_t startIndex) const
    {
        if (positions.empty() || checkpoints.empty())
        {
            return;
        }
        size_t index = std::min(startIndex, positions.size() - 1);
        while (true)
        {
            std::cout << "Playback step " << index << " of " << (positions.size() - 1) << "\n";
            renderer.printBoard(renderer.renderBoard(board, positions[index], checkpoints[index]));
            std::cout << "\n";
            std::cout << "Controls: Left/Right to move, Esc to jump, Q to quit\n";

            KeyInput input = readKey();
            if (input.key == InputKey::Left)
            {
                if (index > 0)
                {
                    --index;
                }
                continue;
            }
            if (input.key == InputKey::Right)
            {
                if (index + 1 < positions.size())
                {
                    ++index;
                }
                continue;
            }
            if (input.key == InputKey::Esc)
            {
                std::cout << "Jump to step (0-" << (positions.size() - 1) << "): ";
                size_t target = 0;
                if (std::cin >> target)
                {
                    if (target < positions.size())
                    {
                        index = target;
                    }
                }
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            if (input.key == InputKey::Other && (input.ch == 'q' || input.ch == 'Q'))
            {
                break;
            }
        }
    }

} // namespace t3
