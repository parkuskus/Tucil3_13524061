# Ice Sliding Puzzle Solver

![Group Photo](cover.png)

## Overview

Ice Sliding Puzzle Solver is a program that finds solutions to ice sliding puzzles using three pathfinding algorithms: Uniform Cost Search (UCS), Greedy Best-First Search (GBFS), and A*. The program reads a board input file, validates the game rules, searches for a solution, and displays the results including move sequence, total cost, iteration count, execution time, board visualization, and solution playback.

## Features

- Supports three search algorithms: UCS, GBFS, and A*.
- Supports three heuristics: Manhattan to Target, Remaining Checkpoints + Manhattan, and Weighted Remaining to Goal.
- Input validation for board layout, checkpoints, start position, goal, and allowed characters.
- Cost calculation based on tiles traversed during sliding.
- Step-by-step solution visualization in the CLI.
- Solution playback after the search completes.
- Save solution and iteration log to a `.txt` file.
- SFML-based GUI for interactive visualization.

## Project Structure

```text
Tucil3_13524061/
├── bin/                # Build output executables
├── doc/                # Report documents and supporting assets
├── src/                # Program source code
├── test/               # Input test case files
├── Makefile            # Build script
├── LICENSE             # Project license
└── README.md           # Main documentation
```

## Requirements

- C++17 compiler such as `g++` or `clang++`.
- GNU Make.
- SFML library for the GUI.
- `vcpkg` or a local library setup appropriate for your environment.

## How To Run

### CLI

Build the CLI program:

```bash
make build
```

Run the CLI program with an input file:

```bash
make run INPUT=test/test-1-base-case.txt
```

Or run the executable directly after building:

```bash
./bin/solver test/test-1-base-case.txt
```

### GUI

Build the GUI program:

```bash
make gui
```

Run the GUI:

```bash
make run-gui
```

## Input File Format

The input file format is:

```text
N M
[N rows of the map]
[N rows of tile costs]
```

Map symbol reference:

- `*` = path
- `X` = rock / obstacle
- `L` = lava
- `Z` = starting position
- `O` = goal
- `0`-`9` = checkpoints that must be visited in order

Short example:

```text
3 3
Z*0
***
**O
1 1 1
1 1 1
1 1 1
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for full details.

## Author

| Name                        | Student ID |
|-----------------------------|------------|
| Muhammad Aufar Rizqi Kusuma | 13524061   |