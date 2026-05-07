CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
INCLUDES := -Isrc/include

CORE_SRCS := $(filter-out src/core/gui/%.cpp,$(wildcard src/core/*/*.cpp))
GUI_CORE_SRCS := $(wildcard src/core/*/*.cpp)
CLI_SRCS := src/main.cpp $(CORE_SRCS)
GUI_SRCS := src/gui_main.cpp $(GUI_CORE_SRCS)

OBJDIR_CLI := build/obj_cli
OBJDIR_GUI := build/obj_gui
OBJS_CLI := $(CLI_SRCS:src/%.cpp=$(OBJDIR_CLI)/%.o)
OBJS_GUI := $(GUI_SRCS:src/%.cpp=$(OBJDIR_GUI)/%.o)

BIN := bin/solver
GUI_BIN := bin/solver_gui

ifeq ($(OS),Windows_NT)
MKDIR_BIN = powershell -NoProfile -Command "if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Path 'bin' | Out-Null }"
MKDIR_OBJ = powershell -NoProfile -Command "New-Item -ItemType Directory -Path '$(dir $@)' -Force | Out-Null"
RUN_CMD = powershell -NoProfile -Command "$$inputPath = '$(INPUT)'; if ([string]::IsNullOrWhiteSpace($$inputPath)) { $$inputPath = Read-Host 'Input file path' } ; if ([string]::IsNullOrWhiteSpace($$inputPath)) { exit 1 } ; & .\\$(BIN) $$inputPath"
VCPKG_INSTALLED := $(CURDIR)/vcpkg_installed/x64-mingw-dynamic
GUI_INCLUDES := -I"$(VCPKG_INSTALLED)/include"
GUI_LDFLAGS := -L"$(VCPKG_INSTALLED)/lib" -lsfml-graphics -lsfml-window -lsfml-system
RUN_GUI_CMD = powershell -NoProfile -Command "$$env:PATH = '$(VCPKG_INSTALLED)/bin;' + $$env:PATH; & .\\$(GUI_BIN)"
else
MKDIR_BIN = mkdir -p bin
MKDIR_OBJ = mkdir -p $(dir $@)
RUN_CMD = sh -c 'if [ -z "$(INPUT)" ]; then read -r -p "Input file path: " inputPath; else inputPath="$(INPUT)"; fi; if [ -z "$$inputPath" ]; then exit 1; fi; ./$(BIN) "$$inputPath"'
RUN_GUI_CMD = ./$(GUI_BIN)
VCPKG_INSTALLED := $(CURDIR)/vcpkg_installed/x64-linux
GUI_INCLUDES := -I"$(VCPKG_INSTALLED)/include"
GUI_LDFLAGS := -L"$(VCPKG_INSTALLED)/lib" -l:libsfml-graphics-s.a -l:libsfml-window-s.a -l:libsfml-system-s.a -lfreetype -lpng16 -lbrotlidec -lbrotlicommon -lbz2 -lz -lX11 -lXrandr -lXi -lXcursor -ludev -lGL -pthread
endif

all: $(BIN)

build: $(BIN)

$(BIN): $(OBJS_CLI)
	@$(MKDIR_BIN)
	$(CXX) $(CXXFLAGS) $(OBJS_CLI) -o $(BIN)

$(OBJDIR_CLI)/%.o: src/%.cpp
	@$(MKDIR_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

gui: $(GUI_BIN)

$(GUI_BIN): $(OBJS_GUI)
	@$(MKDIR_BIN)
	$(CXX) $(CXXFLAGS) $(OBJS_GUI) -o $(GUI_BIN) $(GUI_LDFLAGS)

$(OBJDIR_GUI)/%.o: src/%.cpp
	@$(MKDIR_OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GUI_INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS_CLI) $(OBJS_GUI) $(BIN) $(GUI_BIN)
	rm -rf $(OBJDIR_CLI) $(OBJDIR_GUI)

run: $(BIN)
	@$(RUN_CMD)

run-gui: $(GUI_BIN)
	@$(RUN_GUI_CMD)

.PHONY: all build gui clean run run-gui
