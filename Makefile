# Variables
BUILD_DIR = build
EXEC = $(BUILD_DIR)/sfml_app
CXX = g++-14
CC = gcc-14
ARGS = 

.PHONY: all configure build run clean rebuild

# Default target
all: build

# Run CMake configuration if it hasn't been run yet
configure:
	@mkdir -p $(BUILD_DIR)
	@if [ ! -f $(BUILD_DIR)/Makefile ]; then \
		echo "Configuring project with CMake using $(CXX)..."; \
		cd $(BUILD_DIR) && CXX=$(CXX) CC=$(CC) cmake -DCMAKE_BUILD_TYPE=Release ..; \
	fi

# Build the executable
build: configure
	@echo "Building project..."
	@cmake --build $(BUILD_DIR)

# Run the compiled application with optional arguments (e.g., make run ARGS="--headless")
run: build
	@echo "Running application..."
	@./$(EXEC) $(ARGS)

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

# Rebuild from scratch
rebuild: clean build
