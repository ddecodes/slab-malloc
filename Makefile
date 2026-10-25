# ============================================================
#  Makefile — Build the Slab Allocator project
# ============================================================
#
#  Usage:
#    make            → Build the benchmark executable
#    make test       → Build and run tests
#    make all        → Build everything
#    make run        → Run benchmark
#    make clean      → Remove build artifacts
#
#  Works on macOS, Linux, and Windows (MinGW/MSYS2).
# ============================================================

CXX       ?= clang++
CXXFLAGS  = -std=c++17 -Wall -Wextra -O2

# On macOS, auto-detect Xcode SDK if CommandLineTools SDK has tapi mismatches
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    XCODE_SDK := /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
    ifneq ($(wildcard $(XCODE_SDK)),)
        CXXFLAGS += -isysroot $(XCODE_SDK)
    endif
endif

# Directories
SRC_DIR   = src
TEST_DIR  = tests
BUILD_DIR = build

# Output binaries
ALLOCATOR = $(BUILD_DIR)/allocator
TEST_BIN  = $(BUILD_DIR)/test_allocator

# Source files
MAIN_SRC  = $(SRC_DIR)/main.cpp $(SRC_DIR)/malloc.cpp
TEST_SRC  = $(TEST_DIR)/test_allocator.cpp $(SRC_DIR)/malloc.cpp

.PHONY: all clean test run

all: $(ALLOCATOR) $(TEST_BIN)

$(ALLOCATOR): $(MAIN_SRC) $(SRC_DIR)/slab_allocator.h $(SRC_DIR)/bitmap.h $(SRC_DIR)/malloc.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(MAIN_SRC)
	@echo "Built: $@"

$(TEST_BIN): $(TEST_SRC) $(SRC_DIR)/slab_allocator.h $(SRC_DIR)/bitmap.h $(SRC_DIR)/malloc.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC)
	@echo "Built: $@"

test: $(TEST_BIN)
	@echo "Running tests..."
	@./$(TEST_BIN)

run: $(ALLOCATOR)
	@echo "Running benchmark..."
	@./$(ALLOCATOR)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"
