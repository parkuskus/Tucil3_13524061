#include "gui/GUI.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <string>

#include <SFML/Graphics.hpp>

#include "heuristic/Heuristic.hpp"
#include "parsing/Parser.hpp"
#include "rules/Rules.hpp"
#include "search/Search.hpp"
#include "validator/Validator.hpp"

namespace
{

    struct Button
    {
        Button(const sf::Font &font, const std::string &text = "") : label(font, text)
        {
        }

        sf::RectangleShape rect;
        sf::Text label;

        bool contains(const sf::Vector2f &point) const
        {
            return rect.getGlobalBounds().contains(point);
        }
    };

    struct TextInput
    {
        TextInput(const sf::Font &font, const std::string &value = "") : text(font, value)
        {
        }

        sf::RectangleShape rect;
        sf::Text text;
        bool active = false;
    };

    struct GuiState
    {
        bool boardLoaded = false;
        bool hasSolution = false;
        bool playing = false;
        double elapsedMs = 0.0;
        int iterations = 0;
        int totalCost = 0;
        size_t stepIndex = 0;
        float speed = 2.0f;
        std::string status;

        t3::Board board;
        t3::SearchResult result;
        t3::SearchAlgorithm algorithm = t3::SearchAlgorithm::UCS;
        t3::HeuristicType heuristic = t3::HeuristicType::RemainingPlusManhattan;
    };

    std::string algorithmLabel(t3::SearchAlgorithm algorithm)
    {
        if (algorithm == t3::SearchAlgorithm::UCS)
        {
            return "UCS";
        }
        if (algorithm == t3::SearchAlgorithm::GBFS)
        {
            return "GBFS";
        }
        return "A*";
    }

    std::string heuristicLabel(t3::HeuristicType heuristic)
    {
        if (heuristic == t3::HeuristicType::ManhattanToTarget)
        {
            return "H1";
        }
        if (heuristic == t3::HeuristicType::RemainingPlusManhattan)
        {
            return "H2";
        }
        return "H3";
    }

    sf::Color tileColor(const t3::Tile &tile, bool passed)
    {
        if (tile.type() == t3::TileType::Wall)
        {
            return sf::Color(40, 40, 40);
        }
        if (tile.type() == t3::TileType::Lava)
        {
            return sf::Color(200, 50, 50);
        }
        if (tile.type() == t3::TileType::Goal)
        {
            return sf::Color(60, 160, 80);
        }
        if (tile.type() == t3::TileType::Start)
        {
            return sf::Color(70, 120, 200);
        }
        if (tile.type() == t3::TileType::Checkpoint)
        {
            if (passed)
            {
                return sf::Color(180, 180, 180);
            }
            return sf::Color(230, 170, 60);
        }
        return sf::Color(180, 180, 180);
    }

    void updateButtonLabel(Button &button, const std::string &text)
    {
        button.label.setString(text);
        sf::FloatRect bounds = button.label.getLocalBounds();
        sf::Vector2f pos = button.rect.getPosition();
        sf::Vector2f size = button.rect.getSize();
        button.label.setOrigin(sf::Vector2f{bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
        button.label.setPosition(sf::Vector2f{pos.x + size.x / 2.0f, pos.y + size.y / 2.0f});
    }

    void setTextInput(TextInput &input, const std::string &value)
    {
        input.text.setString(value);
        sf::Vector2f pos = input.rect.getPosition();
        input.text.setPosition(sf::Vector2f{pos.x + 8.0f, pos.y + 6.0f});
    }

    std::string readInputText(const TextInput &input)
    {
        return input.text.getString().toAnsiString();
    }

    std::string normalizePath(const std::string &path)
    {
        if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
        {
            return std::string("test/") + path;
        }
        return path;
    }

    float clampFloat(float value, float minValue, float maxValue)
    {
        if (value < minValue)
        {
            return minValue;
        }
        if (value > maxValue)
        {
            return maxValue;
        }
        return value;
    }

} // namespace

int t3::GUI::run()
{
    const int windowWidth = 1280;
    const int windowHeight = 800;
    const float panelWidth = 320.0f;

    sf::RenderWindow window(sf::VideoMode({static_cast<unsigned int>(windowWidth), static_cast<unsigned int>(windowHeight)}), "Ice Sliding Puzzle Solver");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    {
        std::cerr << "Failed to load font. Install DejaVuSans.ttf or update the font path.\n";
        return 1;
    }

    GuiState state;
    state.status = "Load input file to start.";

    Button loadButton(font);
    loadButton.rect.setPosition(sf::Vector2f{20.0f, 20.0f});
    loadButton.rect.setSize({280.0f, 36.0f});
    loadButton.rect.setFillColor(sf::Color(60, 90, 140));
    loadButton.label.setFont(font);
    loadButton.label.setCharacterSize(16);
    loadButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(loadButton, "Load Input");

    Button solveButton(font);
    solveButton.rect.setPosition(sf::Vector2f{20.0f, 70.0f});
    solveButton.rect.setSize({280.0f, 36.0f});
    solveButton.rect.setFillColor(sf::Color(60, 140, 90));
    solveButton.label.setFont(font);
    solveButton.label.setCharacterSize(16);
    solveButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(solveButton, "Solve");

    Button algoButton(font);
    algoButton.rect.setPosition(sf::Vector2f{20.0f, 130.0f});
    algoButton.rect.setSize({280.0f, 32.0f});
    algoButton.rect.setFillColor(sf::Color(80, 80, 120));
    algoButton.label.setFont(font);
    algoButton.label.setCharacterSize(15);
    algoButton.label.setFillColor(sf::Color::White);

    Button heuristicButton(font);
    heuristicButton.rect.setPosition(sf::Vector2f{20.0f, 170.0f});
    heuristicButton.rect.setSize({280.0f, 32.0f});
    heuristicButton.rect.setFillColor(sf::Color(80, 80, 120));
    heuristicButton.label.setFont(font);
    heuristicButton.label.setCharacterSize(15);
    heuristicButton.label.setFillColor(sf::Color::White);

    Button playButton(font);
    playButton.rect.setPosition(sf::Vector2f{20.0f, 240.0f});
    playButton.rect.setSize({130.0f, 32.0f});
    playButton.rect.setFillColor(sf::Color(90, 120, 80));
    playButton.label.setFont(font);
    playButton.label.setCharacterSize(15);
    playButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(playButton, "Play");

    Button prevButton(font);
    prevButton.rect.setPosition(sf::Vector2f{170.0f, 240.0f});
    prevButton.rect.setSize({60.0f, 32.0f});
    prevButton.rect.setFillColor(sf::Color(100, 100, 100));
    prevButton.label.setFont(font);
    prevButton.label.setCharacterSize(15);
    prevButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(prevButton, "<");

    Button nextButton(font);
    nextButton.rect.setPosition(sf::Vector2f{240.0f, 240.0f});
    nextButton.rect.setSize({60.0f, 32.0f});
    nextButton.rect.setFillColor(sf::Color(100, 100, 100));
    nextButton.label.setFont(font);
    nextButton.label.setCharacterSize(15);
    nextButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(nextButton, ">");

    Button speedDownButton(font);
    speedDownButton.rect.setPosition(sf::Vector2f{20.0f, 290.0f});
    speedDownButton.rect.setSize({60.0f, 28.0f});
    speedDownButton.rect.setFillColor(sf::Color(90, 90, 120));
    speedDownButton.label.setFont(font);
    speedDownButton.label.setCharacterSize(15);
    speedDownButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(speedDownButton, "-");

    Button speedUpButton(font);
    speedUpButton.rect.setPosition(sf::Vector2f{240.0f, 290.0f});
    speedUpButton.rect.setSize({60.0f, 28.0f});
    speedUpButton.rect.setFillColor(sf::Color(90, 90, 120));
    speedUpButton.label.setFont(font);
    speedUpButton.label.setCharacterSize(15);
    speedUpButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(speedUpButton, "+");

    TextInput speedLabel(font);
    speedLabel.rect.setPosition(sf::Vector2f{90.0f, 290.0f});
    speedLabel.rect.setSize({140.0f, 28.0f});
    speedLabel.rect.setFillColor(sf::Color(40, 40, 50));
    speedLabel.text.setFont(font);
    speedLabel.text.setCharacterSize(14);
    speedLabel.text.setFillColor(sf::Color::White);
    setTextInput(speedLabel, "Speed: 2.0x");

    TextInput fileInput(font);
    fileInput.rect.setPosition(sf::Vector2f{20.0f, 360.0f});
    fileInput.rect.setSize({280.0f, 32.0f});
    fileInput.rect.setFillColor(sf::Color(35, 35, 35));
    fileInput.text.setFont(font);
    fileInput.text.setCharacterSize(14);
    fileInput.text.setFillColor(sf::Color::White);
    setTextInput(fileInput, "test/test-1-base-case.txt");

    Button jumpButton(font);
    jumpButton.rect.setPosition(sf::Vector2f{20.0f, 410.0f});
    jumpButton.rect.setSize({80.0f, 28.0f});
    jumpButton.rect.setFillColor(sf::Color(90, 90, 120));
    jumpButton.label.setFont(font);
    jumpButton.label.setCharacterSize(14);
    jumpButton.label.setFillColor(sf::Color::White);
    updateButtonLabel(jumpButton, "Jump");

    TextInput jumpInput(font);
    jumpInput.rect.setPosition(sf::Vector2f{110.0f, 410.0f});
    jumpInput.rect.setSize({190.0f, 28.0f});
    jumpInput.rect.setFillColor(sf::Color(35, 35, 35));
    jumpInput.text.setFont(font);
    jumpInput.text.setCharacterSize(14);
    jumpInput.text.setFillColor(sf::Color::White);
    setTextInput(jumpInput, "0");

    sf::Text statusText(font, "", 14);
    statusText.setFillColor(sf::Color::White);
    statusText.setPosition(sf::Vector2f{20.0f, 460.0f});

    sf::Clock clock;
    float accumulator = 0.0f;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto *mouseButton = event->getIf<sf::Event::MouseButtonPressed>())
            {
                sf::Vector2f mouse{static_cast<float>(mouseButton->position.x), static_cast<float>(mouseButton->position.y)};

                fileInput.active = fileInput.rect.getGlobalBounds().contains(mouse);
                jumpInput.active = jumpInput.rect.getGlobalBounds().contains(mouse);

                if (loadButton.contains(mouse))
                {
                    try
                    {
                        std::string path = normalizePath(readInputText(fileInput));
                        t3::Parser parser;
                        t3::RawInput parsed = parser.parseFile(path);
                        t3::Validator validator;
                        state.board = validator.validate(parsed);
                        state.boardLoaded = true;
                        state.status = "Board loaded.";
                        state.hasSolution = false;
                    }
                    catch (const std::exception &ex)
                    {
                        state.status = std::string("Load failed: ") + ex.what();
                        state.boardLoaded = false;
                    }
                }
                else if (solveButton.contains(mouse) && state.boardLoaded)
                {
                    t3::Rules rules;
                    t3::Search search;
                    t3::SearchConfig config;
                    config.algorithm = state.algorithm;
                    config.heuristic = state.heuristic;
                    config.logIterations = true;

                    auto startTime = std::chrono::steady_clock::now();
                    state.result = search.solve(state.board, rules, config);
                    auto endTime = std::chrono::steady_clock::now();
                    state.elapsedMs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;

                    state.iterations = state.result.iterations;
                    state.totalCost = state.result.totalCost;
                    state.hasSolution = state.result.found;
                    state.stepIndex = 0;
                    state.playing = false;

                    state.status = state.hasSolution ? "Solution found." : "No solution found.";
                }
                else if (algoButton.contains(mouse))
                {
                    if (state.algorithm == t3::SearchAlgorithm::UCS)
                    {
                        state.algorithm = t3::SearchAlgorithm::GBFS;
                    }
                    else if (state.algorithm == t3::SearchAlgorithm::GBFS)
                    {
                        state.algorithm = t3::SearchAlgorithm::AStar;
                    }
                    else
                    {
                        state.algorithm = t3::SearchAlgorithm::UCS;
                    }
                }
                else if (heuristicButton.contains(mouse))
                {
                    if (state.algorithm == t3::SearchAlgorithm::UCS)
                    {
                        state.heuristic = t3::HeuristicType::RemainingPlusManhattan;
                    }
                    else
                    {
                        if (state.heuristic == t3::HeuristicType::ManhattanToTarget)
                        {
                            state.heuristic = t3::HeuristicType::RemainingPlusManhattan;
                        }
                        else if (state.heuristic == t3::HeuristicType::RemainingPlusManhattan)
                        {
                            state.heuristic = t3::HeuristicType::WeightedRemainingToGoal;
                        }
                        else
                        {
                            state.heuristic = t3::HeuristicType::ManhattanToTarget;
                        }
                    }
                }
                else if (playButton.contains(mouse) && state.hasSolution)
                {
                    state.playing = !state.playing;
                }
                else if (prevButton.contains(mouse) && state.hasSolution)
                {
                    if (state.stepIndex > 0)
                    {
                        state.stepIndex--;
                    }
                }
                else if (nextButton.contains(mouse) && state.hasSolution)
                {
                    if (state.stepIndex + 1 < state.result.pathPositions.size())
                    {
                        state.stepIndex++;
                    }
                }
                else if (speedDownButton.contains(mouse))
                {
                    state.speed = clampFloat(state.speed - 0.5f, 0.5f, 10.0f);
                }
                else if (speedUpButton.contains(mouse))
                {
                    state.speed = clampFloat(state.speed + 0.5f, 0.5f, 10.0f);
                }
                else if (jumpButton.contains(mouse) && state.hasSolution)
                {
                    std::string value = readInputText(jumpInput);
                    size_t index = 0;
                    try
                    {
                        index = static_cast<size_t>(std::stoul(value));
                    }
                    catch (...)
                    {
                        index = 0;
                    }
                    if (!state.result.pathPositions.empty())
                    {
                        state.stepIndex = std::min(index, state.result.pathPositions.size() - 1);
                    }
                }
            }
            else if (const auto *textEntered = event->getIf<sf::Event::TextEntered>())
            {
                if (fileInput.active)
                {
                    if (textEntered->unicode == 8)
                    {
                        std::string current = readInputText(fileInput);
                        if (!current.empty())
                        {
                            current.pop_back();
                            setTextInput(fileInput, current);
                        }
                    }
                    else if (textEntered->unicode >= 32 && textEntered->unicode < 127)
                    {
                        std::string current = readInputText(fileInput);
                        current.push_back(static_cast<char>(textEntered->unicode));
                        setTextInput(fileInput, current);
                    }
                }
                else if (jumpInput.active)
                {
                    if (textEntered->unicode == 8)
                    {
                        std::string current = readInputText(jumpInput);
                        if (!current.empty())
                        {
                            current.pop_back();
                            setTextInput(jumpInput, current);
                        }
                    }
                    else if (textEntered->unicode >= 48 && textEntered->unicode <= 57)
                    {
                        std::string current = readInputText(jumpInput);
                        current.push_back(static_cast<char>(textEntered->unicode));
                        setTextInput(jumpInput, current);
                    }
                }
            }
            else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Space && state.hasSolution)
                {
                    state.playing = !state.playing;
                }
                else if (keyPressed->code == sf::Keyboard::Key::Left && state.hasSolution)
                {
                    if (state.stepIndex > 0)
                    {
                        state.stepIndex--;
                    }
                }
                else if (keyPressed->code == sf::Keyboard::Key::Right && state.hasSolution)
                {
                    if (state.stepIndex + 1 < state.result.pathPositions.size())
                    {
                        state.stepIndex++;
                    }
                }
            }
        }

        float delta = clock.restart().asSeconds();
        accumulator += delta;
        if (state.playing && state.hasSolution && state.result.pathPositions.size() > 1)
        {
            float stepInterval = 1.0f / state.speed;
            while (accumulator >= stepInterval)
            {
                accumulator -= stepInterval;
                if (state.stepIndex + 1 < state.result.pathPositions.size())
                {
                    state.stepIndex++;
                }
                else
                {
                    state.playing = false;
                    break;
                }
            }
        }

        updateButtonLabel(algoButton, "Algorithm: " + algorithmLabel(state.algorithm));
        if (state.algorithm == t3::SearchAlgorithm::UCS)
        {
            updateButtonLabel(heuristicButton, "Heuristic: N/A");
        }
        else
        {
            updateButtonLabel(heuristicButton, "Heuristic: " + heuristicLabel(state.heuristic));
        }
        updateButtonLabel(playButton, state.playing ? "Pause" : "Play");
        setTextInput(speedLabel, "Speed: " + std::to_string(state.speed) + "x");

        statusText.setString(state.status + "\n" +
                             "Iterations: " + std::to_string(state.iterations) +
                             "\nCost: " + std::to_string(state.totalCost) +
                             "\nTime (ms): " + std::to_string(state.elapsedMs));

        window.clear(sf::Color(25, 25, 30));

        window.draw(loadButton.rect);
        window.draw(loadButton.label);
        window.draw(solveButton.rect);
        window.draw(solveButton.label);
        window.draw(algoButton.rect);
        window.draw(algoButton.label);
        window.draw(heuristicButton.rect);
        window.draw(heuristicButton.label);
        window.draw(playButton.rect);
        window.draw(playButton.label);
        window.draw(prevButton.rect);
        window.draw(prevButton.label);
        window.draw(nextButton.rect);
        window.draw(nextButton.label);
        window.draw(speedDownButton.rect);
        window.draw(speedDownButton.label);
        window.draw(speedUpButton.rect);
        window.draw(speedUpButton.label);
        window.draw(speedLabel.rect);
        window.draw(speedLabel.text);
        window.draw(fileInput.rect);
        window.draw(fileInput.text);
        window.draw(jumpButton.rect);
        window.draw(jumpButton.label);
        window.draw(jumpInput.rect);
        window.draw(jumpInput.text);
        window.draw(statusText);

        if (state.boardLoaded)
        {
            int rows = state.board.rows();
            int cols = state.board.cols();
            float availableWidth = windowWidth - panelWidth - 40.0f;
            float availableHeight = windowHeight - 40.0f;
            float tileSize = std::min(availableWidth / cols, availableHeight / rows);
            float offsetX = panelWidth + 20.0f;
            float offsetY = 20.0f;

            size_t stepIndex = state.stepIndex;
            int nextCheckpoint = 0;
            t3::Position actor = state.board.start();
            if (state.hasSolution && !state.result.pathPositions.empty())
            {
                stepIndex = std::min(stepIndex, state.result.pathPositions.size() - 1);
                actor = state.result.pathPositions[stepIndex];
                nextCheckpoint = state.result.pathCheckpoints[stepIndex];
            }

            for (int r = 0; r < rows; ++r)
            {
                for (int c = 0; c < cols; ++c)
                {
                    const t3::Tile &tile = state.board.tiles()[r][c];
                    bool passed = tile.type() == t3::TileType::Checkpoint && tile.value() < nextCheckpoint;
                    sf::RectangleShape cell;
                    cell.setSize(sf::Vector2f{tileSize - 1.0f, tileSize - 1.0f});
                    cell.setPosition(sf::Vector2f{offsetX + c * tileSize, offsetY + r * tileSize});
                    cell.setFillColor(tileColor(tile, passed));
                    window.draw(cell);

                    if (tile.type() == t3::TileType::Checkpoint && !passed)
                    {
                        sf::Text digit(font, std::to_string(tile.value()), static_cast<unsigned int>(tileSize * 0.45f));
                        digit.setFillColor(sf::Color::Black);
                        digit.setPosition(sf::Vector2f{cell.getPosition().x + tileSize * 0.25f, cell.getPosition().y + tileSize * 0.1f});
                        window.draw(digit);
                    }
                    else if (tile.type() == t3::TileType::Goal)
                    {
                        sf::Text label(font, "O", static_cast<unsigned int>(tileSize * 0.45f));
                        label.setFillColor(sf::Color::Black);
                        label.setPosition(sf::Vector2f{cell.getPosition().x + tileSize * 0.25f, cell.getPosition().y + tileSize * 0.1f});
                        window.draw(label);
                    }
                }
            }

            sf::CircleShape actorShape(tileSize * 0.35f);
            actorShape.setFillColor(sf::Color(240, 220, 80));
            actorShape.setPosition(sf::Vector2f{offsetX + actor.col() * tileSize + tileSize * 0.15f,
                                                offsetY + actor.row() * tileSize + tileSize * 0.15f});
            window.draw(actorShape);
        }

        window.display();
    }

    return 0;
}
