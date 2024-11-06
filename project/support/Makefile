CXX := g++
CXXFLAGS := -Wall -O2 -std=c++11

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

# Target executable
TARGET := $(BUILD_DIR)/udp_broadcast

# Source and object files
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Default target
all: $(BUILD_DIR) $(TARGET)

# Build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the target executable
$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -pthread

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -o $@ -c $<

# Clean build files
clean:
	rm -rf $(BUILD_DIR)