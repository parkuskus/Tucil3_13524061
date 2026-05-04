CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
INCLUDES := -Isrc/include

SRCS := src/main.cpp $(wildcard src/core/*/*.cpp)
OBJDIR := build/obj
OBJS := $(SRCS:src/%.cpp=$(OBJDIR)/%.o)
BIN := bin/solver

ifeq ($(OS),Windows_NT)
MKDIR_BIN = powershell -NoProfile -Command "if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Path 'bin' | Out-Null }"
MKDIR_OBJ = powershell -NoProfile -Command "New-Item -ItemType Directory -Path '$(dir $@)' -Force | Out-Null"
RUN_CMD = powershell -NoProfile -Command "$$inputPath = '$(INPUT)'; if ([string]::IsNullOrWhiteSpace($$inputPath)) { $$inputPath = Read-Host 'Input file path' } ; if ([string]::IsNullOrWhiteSpace($$inputPath)) { exit 1 } ; & .\\$(BIN) $$inputPath"
else
MKDIR_BIN = mkdir -p bin
MKDIR_OBJ = mkdir -p $(dir $@)
RUN_CMD = sh -c 'if [ -z "$(INPUT)" ]; then read -r -p "Input file path: " inputPath; else inputPath="$(INPUT)"; fi; if [ -z "$$inputPath" ]; then exit 1; fi; ./$(BIN) "$$inputPath"'
endif

all: $(BIN)

build: $(BIN)

$(BIN): $(OBJS)
	@$(MKDIR_BIN)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN)

$(OBJDIR)/%.o: src/%.cpp
	@$(MKDIR_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)
	rm -rf $(OBJDIR)

run: $(BIN)
	@$(RUN_CMD)

.PHONY: all build clean run
