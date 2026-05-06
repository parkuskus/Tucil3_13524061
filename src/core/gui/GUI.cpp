#include "gui/GUI.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

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

    struct Dropdown
    {
        static constexpr size_t MAX_VISIBLE_ITEMS = 5;

        Dropdown(const sf::Font &font, const std::vector<std::string> &items = {}, size_t selected = 0) : label(font, "")
        {
            setItems(items, selected);
        }

        sf::RectangleShape rect;
        sf::Text label;
        std::vector<std::string> items;
        size_t selectedIndex = 0;
        bool expanded = false;
        int expandDirection = 1; // 1 = down, -1 = up
        size_t scrollOffset = 0; // For scrolling when items > MAX_VISIBLE_ITEMS

        void setItems(const std::vector<std::string> &newItems, size_t selected = 0)
        {
            items = newItems;
            if (items.empty())
            {
                selectedIndex = 0;
                label.setString("-");
            }
            else
            {
                selectedIndex = std::min(selected, items.size() - 1);
                label.setString(items[selectedIndex]);
            }
        }

        void select(size_t index)
        {
            if (!items.empty())
            {
                selectedIndex = std::min(index, items.size() - 1);
                label.setString(items[selectedIndex]);
            }
            expanded = false;
        }

        size_t maxScrollOffset() const
        {
            size_t visibleCount = getVisibleCount();
            if (items.size() <= visibleCount)
            {
                return 0;
            }
            return items.size() - visibleCount;
        }

        void scrollBy(int delta)
        {
            if (items.size() <= getVisibleCount())
            {
                scrollOffset = 0;
                return;
            }

            int nextOffset = static_cast<int>(scrollOffset) - delta;
            nextOffset = std::clamp(nextOffset, 0, static_cast<int>(maxScrollOffset()));
            scrollOffset = static_cast<size_t>(nextOffset);
        }

        const std::string &selected() const
        {
            static const std::string empty;
            if (items.empty())
            {
                return empty;
            }
            return items[selectedIndex];
        }

        bool contains(const sf::Vector2f &point) const
        {
            return rect.getGlobalBounds().contains(point);
        }

        bool optionContains(const sf::Vector2f &point, size_t index) const
        {
            sf::FloatRect bounds = optionBounds(index);
            return bounds.contains(point);
        }

        size_t getVisibleCount() const
        {
            return std::min(items.size(), MAX_VISIBLE_ITEMS);
        }

        sf::FloatRect optionBounds(size_t index) const
        {
            sf::Vector2f pos = rect.getPosition();
            sf::Vector2f size = rect.getSize();
            if (expandDirection == 1)
            {
                return sf::FloatRect(sf::Vector2f{pos.x, pos.y + size.y * static_cast<float>(index + 1)}, size);
            }
            else
            {
                return sf::FloatRect(sf::Vector2f{pos.x, pos.y - size.y * static_cast<float>(index + 1)}, size);
            }
        }

        void calculateExpandDirection(float windowHeight)
        {
            sf::Vector2f pos = rect.getPosition();
            sf::Vector2f size = rect.getSize();
            size_t visibleCount = getVisibleCount();
            float menuHeight = size.y * static_cast<float>(visibleCount);
            float spaceBelow = windowHeight - (pos.y + size.y);
            float spaceAbove = pos.y;
            expandDirection = (spaceBelow >= menuHeight) ? 1 : ((spaceAbove >= menuHeight) ? -1 : 1);
        }
    };

    struct SpeedSlider
    {
        sf::RectangleShape track;
        sf::RectangleShape fill;
        sf::CircleShape knob;
        float minValue = 0.5f;
        float maxValue = 10.0f;
        float value = 2.0f;
        bool dragging = false;

        void setGeometry(const sf::Vector2f &position, const sf::Vector2f &size)
        {
            track.setPosition(position);
            track.setSize(size);
            track.setFillColor(sf::Color(38, 48, 62));
            track.setOutlineThickness(1.0f);
            track.setOutlineColor(sf::Color(110, 150, 190, 90));

            fill.setPosition(position);
            fill.setSize({0.0f, size.y});
            fill.setFillColor(sf::Color(120, 180, 235));

            knob.setRadius(size.y * 0.75f);
            knob.setOrigin(sf::Vector2f{knob.getRadius(), knob.getRadius()});
            knob.setFillColor(sf::Color(235, 245, 255));
            knob.setOutlineThickness(2.0f);
            knob.setOutlineColor(sf::Color(70, 130, 180));
            updateVisual();
        }

        void setValue(float newValue)
        {
            value = std::clamp(newValue, minValue, maxValue);
            updateVisual();
        }

        void setFromX(float mouseX)
        {
            float left = track.getPosition().x;
            float width = track.getSize().x;
            float ratio = width > 0.0f ? (mouseX - left) / width : 0.0f;
            ratio = std::clamp(ratio, 0.0f, 1.0f);
            setValue(minValue + ratio * (maxValue - minValue));
        }

        void updateVisual()
        {
            float ratio = (value - minValue) / (maxValue - minValue);
            ratio = std::clamp(ratio, 0.0f, 1.0f);
            sf::Vector2f position = track.getPosition();
            sf::Vector2f size = track.getSize();
            fill.setSize(sf::Vector2f{size.x * ratio, size.y});
            knob.setPosition(sf::Vector2f{position.x + size.x * ratio, position.y + size.y / 2.0f});
        }

        bool contains(const sf::Vector2f &point) const
        {
            return track.getGlobalBounds().contains(point) || knob.getGlobalBounds().contains(point);
        }
    };

    struct FileEntry
    {
        std::string label;
        std::string path;
    };

    struct Snowflake
    {
        sf::Vector2f position;
        float speed = 0.0f;
        float drift = 0.0f;
        float radius = 0.0f;
        float phase = 0.0f;
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
        float transitionProgress = 0.0f;
        std::string status;

        t3::Board board;
        t3::SearchResult result;
        t3::SearchAlgorithm algorithm = t3::SearchAlgorithm::UCS;
        t3::HeuristicType heuristic = t3::HeuristicType::RemainingPlusManhattan;
    };

    sf::Color tileColor(const t3::Tile &tile, bool passed)
    {
        if (tile.type() == t3::TileType::Wall)
        {
            return sf::Color(176, 214, 232);
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

    std::string fitTextToWidth(const sf::Font &font, unsigned int characterSize, const std::string &text, float maxWidth)
    {
        if (maxWidth <= 0.0f)
        {
            return text;
        }

        sf::Text probe(font, text, characterSize);
        if (probe.getLocalBounds().size.x <= maxWidth)
        {
            return text;
        }

        const std::string ellipsis = ".....";
        if (text.size() <= ellipsis.size())
        {
            return ellipsis;
        }

        std::string clipped = text;
        while (!clipped.empty())
        {
            if (clipped.size() <= ellipsis.size())
            {
                return ellipsis;
            }

            clipped = clipped.substr(0, clipped.size() - 1);
            probe.setString(clipped + ellipsis);
            if (probe.getLocalBounds().size.x <= maxWidth)
            {
                return clipped + ellipsis;
            }
        }

        return ellipsis;
    }

    void setDropdownLabel(Dropdown &dropdown, const std::string &prefix)
    {
        std::string text = prefix + ": ";
        if (!dropdown.items.empty())
        {
            text += dropdown.selected();
        }
        else
        {
            text += "-";
        }
        text = fitTextToWidth(dropdown.label.getFont(), dropdown.label.getCharacterSize(), text, dropdown.rect.getSize().x - 16.0f);
        dropdown.label.setString(text);
        sf::FloatRect bounds = dropdown.label.getLocalBounds();
        sf::Vector2f pos = dropdown.rect.getPosition();
        sf::Vector2f size = dropdown.rect.getSize();
        dropdown.label.setOrigin(sf::Vector2f{bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
        dropdown.label.setPosition(sf::Vector2f{pos.x + size.x / 2.0f, pos.y + size.y / 2.0f});
    }

    std::string truncateText(const std::string &text, size_t maxLength)
    {
        if (text.length() > maxLength)
        {
            if (maxLength <= 5)
            {
                return std::string(maxLength, '.');
            }
            return text.substr(0, maxLength - 5) + ".....";
        }
        return text;
    }

    void drawDropdown(sf::RenderWindow &window, const Dropdown &dropdown, const sf::Font &font)
    {
        window.draw(dropdown.rect);
        window.draw(dropdown.label);

        if (!dropdown.expanded)
        {
            return;
        }

        size_t visibleCount = dropdown.getVisibleCount();
        size_t totalItems = dropdown.items.size();

        for (size_t i = 0; i < visibleCount; ++i)
        {
            size_t actualIndex = dropdown.scrollOffset + i;
            if (actualIndex >= totalItems)
            {
                break;
            }

            sf::RectangleShape option = dropdown.rect;
            if (dropdown.expandDirection == 1)
            {
                option.setPosition(sf::Vector2f{dropdown.rect.getPosition().x,
                                                dropdown.rect.getPosition().y + dropdown.rect.getSize().y * static_cast<float>(i + 1)});
            }
            else
            {
                option.setPosition(sf::Vector2f{dropdown.rect.getPosition().x,
                                                dropdown.rect.getPosition().y - dropdown.rect.getSize().y * static_cast<float>(i + 1)});
            }
            option.setFillColor(actualIndex == dropdown.selectedIndex ? sf::Color(75, 105, 145) : sf::Color(50, 62, 82));
            option.setOutlineThickness(1.0f);
            option.setOutlineColor(sf::Color(155, 195, 235, 80));
            window.draw(option);

            std::string displayText = truncateText(dropdown.items[actualIndex], 25);
            sf::Text optionText(font, displayText, 13);
            optionText.setFillColor(sf::Color::White);
            sf::Vector2f pos = option.getPosition();
            optionText.setPosition(sf::Vector2f{pos.x + 8.0f, pos.y + 6.0f});
            window.draw(optionText);
        }
    }

    std::string readInputText(const TextInput &input)
    {
        return input.text.getString().toAnsiString();
    }

    std::vector<FileEntry> loadTestFiles()
    {
        std::vector<FileEntry> files;
        const std::filesystem::path testDir{"test"};
        if (!std::filesystem::exists(testDir))
        {
            return files;
        }

        for (const auto &entry : std::filesystem::directory_iterator(testDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".txt")
            {
                continue;
            }
            FileEntry file;
            file.label = entry.path().filename().string();
            file.path = entry.path().generic_string();
            files.push_back(file);
        }

        std::sort(files.begin(), files.end(), [](const FileEntry &a, const FileEntry &b)
                  {
                      auto extractIndex = [](const std::string &text) -> int
                      {
                          size_t start = text.find_first_of("0123456789");
                          if (start == std::string::npos)
                          {
                              return -1;
                          }

                          size_t end = start;
                          while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end])))
                          {
                              ++end;
                          }

                          try
                          {
                              return std::stoi(text.substr(start, end - start));
                          }
                          catch (...)
                          {
                              return -1;
                          }
                      };

                      int leftIndex = extractIndex(a.label);
                      int rightIndex = extractIndex(b.label);
                      if (leftIndex != rightIndex)
                      {
                          if (leftIndex < 0)
                          {
                              return false;
                          }
                          if (rightIndex < 0)
                          {
                              return true;
                          }
                          return leftIndex < rightIndex;
                      }

                      return a.label < b.label; });

        return files;
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

    sf::Vector2f lerp(const sf::Vector2f &a, const sf::Vector2f &b, float t)
    {
        return sf::Vector2f{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    float smoothStep(float t)
    {
        t = clampFloat(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    sf::Vector2f tileCenter(int row, int col, float tileSize, float offsetX, float offsetY)
    {
        return sf::Vector2f{offsetX + col * tileSize + tileSize / 2.0f,
                            offsetY + row * tileSize + tileSize / 2.0f};
    }

    void resetPlaybackToCurrentStep(GuiState &state)
    {
        state.playing = false;
        state.transitionProgress = 0.0f;
        if (!state.result.pathPositions.empty())
        {
            state.stepIndex = std::min(state.stepIndex, state.result.pathPositions.size() - 1);
        }
    }

    void drawSnowflake(sf::RenderWindow &window, const Snowflake &flake)
    {
        sf::CircleShape shape(flake.radius);
        shape.setPosition(sf::Vector2f{flake.position.x, flake.position.y});
        shape.setFillColor(sf::Color(255, 255, 255, 175));
        window.draw(shape);
    }

    void drawBackdrop(sf::RenderWindow &window, int windowWidth, int windowHeight)
    {
        sf::RectangleShape base(sf::Vector2f{static_cast<float>(windowWidth), static_cast<float>(windowHeight)});
        base.setFillColor(sf::Color(15, 24, 40));
        window.draw(base);

        sf::CircleShape glow(280.0f);
        glow.setFillColor(sf::Color(90, 140, 190, 24));
        glow.setPosition(sf::Vector2f{windowWidth - 430.0f, -120.0f});
        window.draw(glow);

        sf::CircleShape snowDune1(380.0f);
        snowDune1.setFillColor(sf::Color(230, 242, 255, 28));
        snowDune1.setPosition(sf::Vector2f{-120.0f, static_cast<float>(windowHeight) - 150.0f});
        window.draw(snowDune1);

        sf::CircleShape snowDune2(260.0f);
        snowDune2.setFillColor(sf::Color(210, 232, 250, 20));
        snowDune2.setPosition(sf::Vector2f{180.0f, static_cast<float>(windowHeight) - 120.0f});
        window.draw(snowDune2);

        sf::RectangleShape frostBand(sf::Vector2f{static_cast<float>(windowWidth), 8.0f});
        frostBand.setPosition(sf::Vector2f{0.0f, static_cast<float>(windowHeight) - 8.0f});
        frostBand.setFillColor(sf::Color(210, 235, 255, 55));
        window.draw(frostBand);
    }

    std::vector<Snowflake> createSnowflakes(int windowWidth, int windowHeight)
    {
        std::vector<Snowflake> flakes;
        flakes.reserve(90);
        for (int i = 0; i < 90; ++i)
        {
            Snowflake flake;
            flake.position = sf::Vector2f{static_cast<float>((i * 97) % windowWidth), static_cast<float>((i * 53) % windowHeight)};
            flake.speed = 18.0f + static_cast<float>(i % 7) * 11.0f;
            flake.drift = 12.0f + static_cast<float>(i % 5) * 7.0f;
            flake.radius = 1.2f + static_cast<float>(i % 4) * 0.8f;
            flake.phase = static_cast<float>(i) * 0.35f;
            flakes.push_back(flake);
        }
        return flakes;
    }

    void updateSnowflakes(std::vector<Snowflake> &flakes, float deltaSeconds, int windowWidth, int windowHeight)
    {
        for (size_t i = 0; i < flakes.size(); ++i)
        {
            Snowflake &flake = flakes[i];
            flake.phase += deltaSeconds * 0.6f;
            flake.position.y += flake.speed * deltaSeconds;
            flake.position.x += std::sin(flake.phase) * flake.drift * deltaSeconds;
            if (flake.position.y > static_cast<float>(windowHeight) + 10.0f)
            {
                flake.position.y = -10.0f;
                flake.position.x = static_cast<float>((static_cast<int>(i) * 97) % windowWidth);
            }
            if (flake.position.x < -10.0f)
            {
                flake.position.x = static_cast<float>(windowWidth) + 10.0f;
            }
            if (flake.position.x > static_cast<float>(windowWidth) + 10.0f)
            {
                flake.position.x = -10.0f;
            }
        }
    }

    std::string formatSpeed(float value)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(1) << value;
        return stream.str();
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

    Dropdown algorithmDropdown(font);
    algorithmDropdown.rect.setPosition(sf::Vector2f{20.0f, 130.0f});
    algorithmDropdown.rect.setSize({280.0f, 32.0f});
    algorithmDropdown.rect.setFillColor(sf::Color(70, 88, 120));
    algorithmDropdown.rect.setOutlineThickness(1.0f);
    algorithmDropdown.rect.setOutlineColor(sf::Color(160, 190, 230, 90));
    algorithmDropdown.label.setFont(font);
    algorithmDropdown.label.setCharacterSize(15);
    algorithmDropdown.label.setFillColor(sf::Color::White);
    algorithmDropdown.setItems({"UCS", "GBFS", "A*"}, 0);
    setDropdownLabel(algorithmDropdown, "Algorithm");

    Dropdown heuristicDropdown(font);
    heuristicDropdown.rect.setPosition(sf::Vector2f{20.0f, 172.0f});
    heuristicDropdown.rect.setSize({280.0f, 32.0f});
    heuristicDropdown.rect.setFillColor(sf::Color(70, 88, 120));
    heuristicDropdown.rect.setOutlineThickness(1.0f);
    heuristicDropdown.rect.setOutlineColor(sf::Color(160, 190, 230, 90));
    heuristicDropdown.label.setFont(font);
    heuristicDropdown.label.setCharacterSize(15);
    heuristicDropdown.label.setFillColor(sf::Color::White);
    heuristicDropdown.setItems({"H1 - Manhattan", "H2 - Remaining + Manhattan", "H3 - Weighted Goal"}, 1);
    setDropdownLabel(heuristicDropdown, "Heuristic");

    std::vector<FileEntry> testFiles = loadTestFiles();
    if (testFiles.empty())
    {
        testFiles.push_back(FileEntry{"test/test-1-base-case.txt", "test/test-1-base-case.txt"});
    }

    Dropdown fileDropdown(font);
    fileDropdown.rect.setPosition(sf::Vector2f{20.0f, 360.0f});
    fileDropdown.rect.setSize({280.0f, 32.0f});
    fileDropdown.rect.setFillColor(sf::Color(55, 68, 90));
    fileDropdown.rect.setOutlineThickness(1.0f);
    fileDropdown.rect.setOutlineColor(sf::Color(160, 190, 230, 90));
    fileDropdown.label.setFont(font);
    fileDropdown.label.setCharacterSize(14);
    fileDropdown.label.setFillColor(sf::Color::White);
    std::vector<std::string> fileLabels;
    fileLabels.reserve(testFiles.size());
    for (const FileEntry &entry : testFiles)
    {
        fileLabels.push_back(entry.label);
    }
    fileDropdown.setItems(fileLabels, 0);
    setDropdownLabel(fileDropdown, "File");

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

    SpeedSlider speedSlider;
    speedSlider.setGeometry(sf::Vector2f{90.0f, 332.0f}, sf::Vector2f{140.0f, 8.0f});
    speedSlider.setValue(state.speed);

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
    statusText.setPosition(sf::Vector2f{20.0f, 460.0f});

    auto syncAlgorithm = [](Dropdown &dropdown, t3::SearchAlgorithm &algorithm)
    {
        switch (dropdown.selectedIndex)
        {
        case 0:
            algorithm = t3::SearchAlgorithm::UCS;
            break;
        case 1:
            algorithm = t3::SearchAlgorithm::GBFS;
            break;
        default:
            algorithm = t3::SearchAlgorithm::AStar;
            break;
        }
    };

    auto syncHeuristic = [](Dropdown &dropdown, t3::HeuristicType &heuristic)
    {
        switch (dropdown.selectedIndex)
        {
        case 0:
            heuristic = t3::HeuristicType::ManhattanToTarget;
            break;
        case 1:
            heuristic = t3::HeuristicType::RemainingPlusManhattan;
            break;
        default:
            heuristic = t3::HeuristicType::WeightedRemainingToGoal;
            break;
        }
    };

    auto closeDropdowns = [&]()
    {
        algorithmDropdown.expanded = false;
        heuristicDropdown.expanded = false;
        fileDropdown.expanded = false;
    };

    sf::Clock clock;
    std::vector<Snowflake> snowflakes = createSnowflakes(windowWidth, windowHeight);

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
                sf::Vector2f mouse = window.mapPixelToCoords(mouseButton->position);

                jumpInput.active = state.hasSolution && jumpInput.rect.getGlobalBounds().contains(mouse);

                auto processDropdown = [&](Dropdown &dropdown, auto syncState)
                {
                    if (dropdown.contains(mouse))
                    {
                        bool wasExpanded = dropdown.expanded;
                        closeDropdowns();
                        dropdown.expanded = !wasExpanded;
                        dropdown.scrollOffset = 0;
                        return true;
                    }

                    if (dropdown.expanded)
                    {
                        size_t visibleCount = dropdown.getVisibleCount();
                        for (size_t i = 0; i < visibleCount; ++i)
                        {
                            size_t actualIndex = dropdown.scrollOffset + i;
                            if (actualIndex >= dropdown.items.size())
                            {
                                break;
                            }
                            if (dropdown.optionContains(mouse, i))
                            {
                                dropdown.select(actualIndex);
                                syncState(dropdown);
                                closeDropdowns();
                                return true;
                            }
                        }
                        dropdown.expanded = false;
                    }

                    return false;
                };

                if (processDropdown(fileDropdown, [&](Dropdown &) {}))
                {
                }
                else if (processDropdown(algorithmDropdown, [&](Dropdown &dropdown)
                                         { syncAlgorithm(dropdown, state.algorithm); }))
                {
                }
                else if (processDropdown(heuristicDropdown, [&](Dropdown &dropdown)
                                         { syncHeuristic(dropdown, state.heuristic); }))
                {
                }
                else
                {
                    closeDropdowns();
                }

                algorithmDropdown.calculateExpandDirection(windowHeight);
                heuristicDropdown.calculateExpandDirection(windowHeight);
                fileDropdown.calculateExpandDirection(windowHeight);

                if (loadButton.contains(mouse))
                {
                    try
                    {
                        std::string path = normalizePath(testFiles[fileDropdown.selectedIndex].path);
                        t3::Parser parser;
                        t3::RawInput parsed = parser.parseFile(path);
                        t3::Validator validator;
                        state.board = validator.validate(parsed);
                        state.boardLoaded = true;
                        state.status = "Board loaded.";
                        state.hasSolution = false;
                        state.result = {};
                        state.transitionProgress = 0.0f;
                    }
                    catch (const std::exception &ex)
                    {
                        state.status = std::string("Load failed: ") + ex.what();
                        state.boardLoaded = false;
                        state.hasSolution = false;
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
                    state.transitionProgress = 0.0f;
                    resetPlaybackToCurrentStep(state);

                    state.status = state.hasSolution ? "Solusi ditemukan." : "Tidak ada solusi!";
                }
                else if (state.hasSolution && playButton.contains(mouse))
                {
                    state.playing = !state.playing;
                    if (state.playing)
                    {
                        state.transitionProgress = 0.0f;
                    }
                }
                else if (state.hasSolution && prevButton.contains(mouse))
                {
                    resetPlaybackToCurrentStep(state);
                    if (state.stepIndex > 0)
                    {
                        state.stepIndex--;
                    }
                }
                else if (state.hasSolution && nextButton.contains(mouse))
                {
                    resetPlaybackToCurrentStep(state);
                    if (state.stepIndex + 1 < state.result.pathPositions.size())
                    {
                        state.stepIndex++;
                    }
                }
                else if (state.hasSolution && speedDownButton.contains(mouse))
                {
                    speedSlider.setValue(speedSlider.value - 0.5f);
                    state.speed = speedSlider.value;
                }
                else if (state.hasSolution && speedUpButton.contains(mouse))
                {
                    speedSlider.setValue(speedSlider.value + 0.5f);
                    state.speed = speedSlider.value;
                }
                else if (state.hasSolution && speedSlider.contains(mouse))
                {
                    speedSlider.dragging = true;
                    speedSlider.setFromX(mouse.x);
                    state.speed = speedSlider.value;
                }
                else if (state.hasSolution && jumpButton.contains(mouse))
                {
                    resetPlaybackToCurrentStep(state);
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
            else if (const auto *mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                if (mouseWheel->delta != 0.0f)
                {
                    if (fileDropdown.expanded)
                    {
                        fileDropdown.scrollBy(static_cast<int>(mouseWheel->delta));
                    }
                    else if (algorithmDropdown.expanded)
                    {
                        algorithmDropdown.scrollBy(static_cast<int>(mouseWheel->delta));
                    }
                    else if (heuristicDropdown.expanded)
                    {
                        heuristicDropdown.scrollBy(static_cast<int>(mouseWheel->delta));
                    }
                }
            }
            else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>())
            {
                if (speedSlider.dragging)
                {
                    sf::Vector2f mouse = window.mapPixelToCoords(mouseMoved->position);
                    speedSlider.setFromX(mouse.x);
                    state.speed = speedSlider.value;
                }
            }
            else if (event->is<sf::Event::MouseButtonReleased>())
            {
                speedSlider.dragging = false;
            }
            else if (const auto *textEntered = event->getIf<sf::Event::TextEntered>())
            {
                if (jumpInput.active)
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
                    if (state.playing)
                    {
                        state.transitionProgress = 0.0f;
                    }
                }
                else if (keyPressed->code == sf::Keyboard::Key::Left && state.hasSolution)
                {
                    resetPlaybackToCurrentStep(state);
                    if (state.stepIndex > 0)
                    {
                        state.stepIndex--;
                    }
                }
                else if (keyPressed->code == sf::Keyboard::Key::Right && state.hasSolution)
                {
                    resetPlaybackToCurrentStep(state);
                    if (state.stepIndex + 1 < state.result.pathPositions.size())
                    {
                        state.stepIndex++;
                    }
                }
            }
        }

        float delta = clock.restart().asSeconds();
        updateSnowflakes(snowflakes, delta, windowWidth, windowHeight);

        if (state.playing && state.hasSolution && state.result.pathPositions.size() > 1)
        {
            float stepInterval = 1.0f / state.speed;
            state.transitionProgress += delta / stepInterval;
            while (state.transitionProgress >= 1.0f)
            {
                if (state.stepIndex + 1 < state.result.pathPositions.size())
                {
                    state.stepIndex++;
                    state.transitionProgress -= 1.0f;
                }
                else
                {
                    state.transitionProgress = 0.0f;
                    state.playing = false;
                    break;
                }
            }
        }
        else
        {
            state.transitionProgress = 0.0f;
        }

        speedSlider.setValue(state.speed);

        setDropdownLabel(algorithmDropdown, "Algorithm");
        setDropdownLabel(heuristicDropdown, "Heuristic");
        setDropdownLabel(fileDropdown, "File");
        setTextInput(speedLabel, "Speed: " + formatSpeed(state.speed) + "x");
        updateButtonLabel(playButton, state.playing ? "Pause" : "Play");

        statusText.setString(state.status + "\n" +
                             "Iterations: " + std::to_string(state.iterations) +
                             "\nCost: " + std::to_string(state.totalCost) +
                             "\nTime (ms): " + std::to_string(state.elapsedMs));
        if (state.status == "Tidak ada solusi!")
        {
            statusText.setFillColor(sf::Color(230, 90, 90));
        }
        else
        {
            statusText.setFillColor(sf::Color::White);
        }

        drawBackdrop(window, windowWidth, windowHeight);
        for (const Snowflake &flake : snowflakes)
        {
            drawSnowflake(window, flake);
        }

        window.draw(loadButton.rect);
        window.draw(loadButton.label);
        window.draw(solveButton.rect);
        window.draw(solveButton.label);
        drawDropdown(window, algorithmDropdown, font);
        drawDropdown(window, heuristicDropdown, font);
        drawDropdown(window, fileDropdown, font);
        window.draw(statusText);

        if (state.hasSolution)
        {
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
            window.draw(speedSlider.track);
            window.draw(speedSlider.fill);
            window.draw(speedSlider.knob);
            window.draw(jumpButton.rect);
            window.draw(jumpButton.label);
            window.draw(jumpInput.rect);
            window.draw(jumpInput.text);
        }

        if (state.boardLoaded)
        {
            int rows = state.board.rows();
            int cols = state.board.cols();
            float availableWidth = windowWidth - panelWidth - 40.0f;
            float availableHeight = windowHeight - 40.0f;
            float tileSize = std::min(availableWidth / cols, availableHeight / rows);
            float boardWidth = tileSize * cols;
            float boardHeight = tileSize * rows;
            float offsetX = panelWidth + 20.0f + (availableWidth - boardWidth) / 2.0f;
            float offsetY = 20.0f + (availableHeight - boardHeight) / 2.0f;

            size_t stepIndex = state.stepIndex;
            int nextCheckpoint = 0;
            sf::Vector2f actor = tileCenter(state.board.start().row(), state.board.start().col(), tileSize, offsetX, offsetY);
            if (state.hasSolution && !state.result.pathPositions.empty())
            {
                stepIndex = std::min(stepIndex, state.result.pathPositions.size() - 1);
                actor = tileCenter(state.result.pathPositions[stepIndex].row(), state.result.pathPositions[stepIndex].col(), tileSize, offsetX, offsetY);
                nextCheckpoint = state.result.pathCheckpoints[stepIndex];

                if (state.playing && stepIndex + 1 < state.result.pathPositions.size())
                {
                    float eased = smoothStep(state.transitionProgress);
                    sf::Vector2f from = tileCenter(state.result.pathPositions[stepIndex].row(), state.result.pathPositions[stepIndex].col(), tileSize, offsetX, offsetY);
                    sf::Vector2f to = tileCenter(state.result.pathPositions[stepIndex + 1].row(), state.result.pathPositions[stepIndex + 1].col(), tileSize, offsetX, offsetY);
                    actor = lerp(from,
                                 to,
                                 eased);
                    nextCheckpoint = state.result.pathCheckpoints[stepIndex];
                }
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
                    cell.setOutlineThickness(1.0f);
                    cell.setOutlineColor(sf::Color(240, 248, 255, 40));
                    window.draw(cell);

                    sf::Vector2f center = tileCenter(r, c, tileSize, offsetX, offsetY);
                    if (tile.type() == t3::TileType::Wall)
                    {
                        sf::RectangleShape topBand(sf::Vector2f{tileSize - 10.0f, tileSize * 0.18f});
                        topBand.setPosition(sf::Vector2f{cell.getPosition().x + 5.0f, cell.getPosition().y + 4.0f});
                        topBand.setFillColor(sf::Color(245, 250, 255, 120));
                        window.draw(topBand);

                        sf::RectangleShape sideBand(sf::Vector2f{tileSize * 0.16f, tileSize - 10.0f});
                        sideBand.setPosition(sf::Vector2f{cell.getPosition().x + 4.0f, cell.getPosition().y + 5.0f});
                        sideBand.setFillColor(sf::Color(150, 190, 215, 70));
                        window.draw(sideBand);

                        sf::CircleShape frost(tileSize * 0.08f);
                        frost.setFillColor(sf::Color(255, 255, 255, 85));
                        frost.setPosition(sf::Vector2f{center.x - frost.getRadius() * 0.5f, center.y - frost.getRadius() * 0.9f});
                        window.draw(frost);

                        sf::RectangleShape crack1(sf::Vector2f{tileSize * 0.35f, 2.0f});
                        crack1.setPosition(sf::Vector2f{cell.getPosition().x + tileSize * 0.18f, cell.getPosition().y + tileSize * 0.55f});
                        crack1.setRotation(sf::degrees(-18.0f));
                        crack1.setFillColor(sf::Color(120, 160, 190, 95));
                        window.draw(crack1);
                    }
                    else if (tile.type() == t3::TileType::Lava)
                    {
                        sf::CircleShape ember(tileSize * 0.09f);
                        ember.setFillColor(sf::Color(255, 210, 130, 140));
                        ember.setPosition(sf::Vector2f{center.x - ember.getRadius(), center.y - ember.getRadius()});
                        window.draw(ember);
                    }
                    else if (tile.type() == t3::TileType::Checkpoint && !passed)
                    {
                        sf::CircleShape shard(tileSize * 0.12f);
                        shard.setFillColor(sf::Color(255, 255, 255, 110));
                        shard.setOutlineThickness(1.0f);
                        shard.setOutlineColor(sf::Color(70, 120, 170, 120));
                        shard.setPosition(sf::Vector2f{center.x - shard.getRadius(), center.y - shard.getRadius()});
                        window.draw(shard);
                    }

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

            sf::CircleShape actorGlow(tileSize * 0.42f);
            actorGlow.setFillColor(sf::Color(150, 200, 255, 28));
            actorGlow.setPosition(sf::Vector2f{actor.x - actorGlow.getRadius(), actor.y - actorGlow.getRadius()});
            window.draw(actorGlow);

            sf::CircleShape actorShape(tileSize * 0.35f);
            actorShape.setFillColor(sf::Color(245, 248, 255));
            actorShape.setOutlineThickness(3.0f);
            actorShape.setOutlineColor(sf::Color(90, 150, 210));
            actorShape.setPosition(sf::Vector2f{actor.x - actorShape.getRadius(), actor.y - actorShape.getRadius()});
            window.draw(actorShape);
        }

        window.display();
    }

    return 0;
}
