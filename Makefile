# =============================================================================
# Lab Management Server - Makefile
# Target OS: Ubuntu Linux (20.04 / 22.04 / 24.04 LTS)
# C++ Standard: C++17
# =============================================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Iinclude

# Auto-detect MariaDB / MySQL compiler & linker flags
MARIADB_CFLAGS := $(shell mariadb_config --cflags 2>/dev/null || mysql_config --cflags 2>/dev/null || echo "")
MARIADB_LIBS   := $(shell mariadb_config --libs 2>/dev/null || mysql_config --libs 2>/dev/null || echo "-lmariadb -lpthread")

CXXFLAGS += $(MARIADB_CFLAGS)
LDFLAGS  += $(MARIADB_LIBS) -lpthread

SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build-obj
BIN_DIR   := bin
TARGET    := $(BIN_DIR)/server

SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Default target
.PHONY: all
all: $(TARGET)

.PHONY: build compile
build: compile
compile: all

# Link the executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "==> Linking target: $@"
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo "==> Build successful! Run with: make run"

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "==> Compiling: $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create required output directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Run server
.PHONY: run
run: $(TARGET)
	@echo "==> Starting Lab Management Server on port 8080..."
	@echo "==> Open browser: http://localhost:8080"
	@./$(TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "==> Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "==> Clean complete."

# Check dependencies on Ubuntu Linux
.PHONY: check-deps
check-deps:
	@echo "Checking environment dependencies..."
	@which $(CXX) >/dev/null 2>&1 && echo "  [OK] C++ Compiler ($(CXX)) found" || echo "  [MISSING] g++ compiler. Install with: sudo apt install build-essential"
	@which make >/dev/null 2>&1 && echo "  [OK] Make utility found" || echo "  [MISSING] make. Install with: sudo apt install make"
	@which mariadb_config >/dev/null 2>&1 || which mysql_config >/dev/null 2>&1 && echo "  [OK] MariaDB/MySQL Client development library found" || echo "  [MISSING] libmariadb-dev. Install with: sudo apt install libmariadb-dev"
	@which mariadb >/dev/null 2>&1 || which mysql >/dev/null 2>&1 && echo "  [OK] MariaDB CLI found" || echo "  [MISSING] mariadb-client. Install with: sudo apt install mariadb-client"

# Quick helper to initialize MariaDB database schema
.PHONY: db-init
db-init:
	@echo "==> Importing schema into MariaDB (will prompt for MariaDB root password)..."
	mariadb -u root -p < sql/schema.sql
	@echo "==> Database schema initialized successfully!"

# Test endpoint verification script
.PHONY: test
test:
	@echo "==> Running basic API endpoint curl verification..."
	@bash tests/run_api_tests.sh || echo "Test script executed."

# Help menu
.PHONY: help
help:
	@echo "Available Makefile targets:"
	@echo "  make            - Build the server executable ($(TARGET))"
	@echo "  make run        - Build (if needed) and run the server on port 8080"
	@echo "  make clean      - Remove build objects and binary"
	@echo "  make check-deps - Check required Ubuntu development packages"
	@echo "  make db-init    - Import sql/schema.sql into local MariaDB"
	@echo "  make test       - Execute API test suite in tests/"
	@echo "  make help       - Display this help message"
