# Variables
BUILD_DIR ?= build
EXEC ?= $(BUILD_DIR)/sfml_app
CXX ?= g++-14
CC ?= gcc-14
UV ?= uv
CONFIG ?= config.yaml
RUN_ARGS ?=
OPT_ARGS ?=
OPT_LOG ?= optimization_log.csv
OPT_GRAPH ?= optimization_progress.svg
OPT_OUTPUT ?= config_optimized.yaml

.PHONY: all configure build run headless config-run config-headless help clean rebuild optimize optimize-quick plot plot-optimization run-optimized demo

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
	@cmake --build $(BUILD_DIR) --parallel

run: build
	@echo "Running application with $(CONFIG)..."
	@./$(EXEC) --config $(CONFIG) $(RUN_ARGS)

headless: build
	@echo "Running application in headless mode with $(CONFIG)..."
	@./$(EXEC) --config $(CONFIG) --headless $(RUN_ARGS)

config-run: run

config-headless: headless

optimize: build
	@echo "Running optimizer..."
	@$(UV) run tools/optimize_fields.py --output $(OPT_OUTPUT) --log $(OPT_LOG) $(OPT_ARGS)

optimize-quick: build
	@echo "Running quick optimizer check..."
	@$(UV) run tools/optimize_fields.py --popsize 1 --maxiter 1 --no-polish --output $(OPT_OUTPUT) --log $(OPT_LOG) $(OPT_ARGS)

plot: plot-optimization

plot-optimization:
	@echo "Building optimization graph..."
	@$(UV) run tools/plot_optimization_log.py $(OPT_LOG) --output $(OPT_GRAPH)

run-optimized: build
	@echo "Running optimized configuration..."
	@./$(EXEC) --config $(OPT_OUTPUT) $(RUN_ARGS)

demo: optimize-quick plot
	@echo "Demo artifacts ready:"
	@echo "  Optimized config: $(OPT_OUTPUT)"
	@echo "  Optimization log: $(OPT_LOG)"
	@echo "  Optimization graph: $(OPT_GRAPH)"

help:
	@echo "Common targets:"
	@echo "  make build"
	@echo "  make run"
	@echo "  make headless"
	@echo "  make config-run CONFIG=config_optimized.yaml"
	@echo "  make config-headless CONFIG=config_optimized.yaml"
	@echo "  make optimize OPT_ARGS=\"--seed 42 --maxiter 10\""
	@echo "  make optimize-quick"
	@echo "  make plot"
	@echo "  make run-optimized"
	@echo "  make demo"
	@echo ""
	@echo "Variables:"
	@echo "  CONFIG=config.yaml"
	@echo "  RUN_ARGS=\"...\""
	@echo "  OPT_ARGS=\"...\""

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

# Rebuild from scratch
rebuild: clean build
