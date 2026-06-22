#!/usr/bin/env python3
# /// script
# requires-python = ">=3.14"
# dependencies = [
#     "scipy",
#     "pyyaml",
# ]
# ///

import argparse
import copy
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml
from scipy.optimize import differential_evolution

reconfigure = getattr(sys.stdout, "reconfigure", None)
if callable(reconfigure):
    reconfigure(line_buffering=True)

DEFAULT_CONFIG_PATH = Path("config.yaml")
DEFAULT_OUTPUT_PATH = Path("config_optimized.yaml")
DEFAULT_EXEC_PATH = Path("./build/sfml_app")
DEFAULT_LOG_PATH = Path("optimization_log.csv")
TARGET_FRACTION = 0.8


def load_config(path):
    with path.open("r", encoding="utf-8") as file:
        config = yaml.safe_load(file)
    if not isinstance(config, dict):
        raise ValueError(f"Invalid config: {path}")
    return config


def get_optimizable_fields(config):
    fields = config["simulation_elements"]["segment_fields"]
    result = []

    for index, field in enumerate(fields):
        if not field.get("optimize"):
            continue

        bounds = field.get("bounds")
        if not isinstance(bounds, list) or len(bounds) != 2:
            raise ValueError(f"Field #{index + 1} must have bounds: [min, max]")

        low = float(bounds[0])
        high = float(bounds[1])
        if low >= high:
            raise ValueError(f"Field #{index + 1} has invalid bounds: {bounds}")

        result.append(
            {
                "index": index,
                "bounds": (low, high),
                "start": field["start"],
                "end": field["end"],
                "initial": float(field["intensity"]),
            }
        )

    if not result:
        raise ValueError("No fields marked with optimize: true")

    return result


def apply_intensities(config, fields, intensities, seed=None):
    updated = copy.deepcopy(config)
    if seed is not None:
        updated["random_seed"] = int(seed)
    segment_fields = updated["simulation_elements"]["segment_fields"]

    for field, intensity in zip(fields, intensities, strict=True):
        segment_fields[field["index"]]["intensity"] = float(intensity)

    return updated


def parse_status(line):
    line = line.strip()
    if not line.startswith("SIM_STATUS") and not line.startswith("SIM_RESULT"):
        return None

    values = {}
    for token in line.split()[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            values[key] = value

    try:
        return {
            "time": float(values["virtual_time"]),
            "reached": int(values["reached"]),
            "total": int(values["total"]),
            "success": values.get("success") == "1" if line.startswith("SIM_RESULT") else None,
        }
    except (KeyError, ValueError):
        return None


def run_simulation(intensities, config, fields, exec_path, seed, log_state):
    updated_config = apply_intensities(config, fields, intensities, seed)
    particle_count = int(updated_config.get("particle_count", 50))
    target_count = max(1, int(TARGET_FRACTION * particle_count))

    reached = 0
    virtual_time = 1000.0

    temp_path = None
    process = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".yaml",
            prefix="optimizer_",
            delete=False,
            encoding="utf-8",
        ) as temp_file:
            yaml.safe_dump(updated_config, temp_file, sort_keys=False)
            temp_path = Path(temp_file.name)

        process = subprocess.Popen(
            [str(exec_path), "--config", str(temp_path), "--headless"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )

        if process.stdout is not None:
            for line in process.stdout:
                status = parse_status(line)
                if status is None:
                    continue

                particle_count = status["total"]
                target_count = max(1, int(TARGET_FRACTION * particle_count))
                virtual_time = status["time"]
                reached = status["reached"]

                if reached >= target_count:
                    process.terminate()
                    break

        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
    except Exception:
        if process is not None:
            try:
                process.kill()
            except Exception:
                pass
        return 10000.0
    finally:
        if temp_path is not None and temp_path.exists():
            temp_path.unlink(missing_ok=True)

    if reached >= target_count:
        cost = virtual_time
    else:
        cost = 1000.0 + (target_count - reached) * 100.0

    values = ", ".join(
        f"#{field['index'] + 1}={float(intensity):.1f}"
        for field, intensity in zip(fields, intensities, strict=True)
    )
    log_state["eval"] += 1
    log_state["best_cost"] = min(log_state["best_cost"], cost)
    row = {
        "eval": log_state["eval"],
        "cost": f"{cost:.6f}",
        "best_cost": f"{log_state['best_cost']:.6f}",
        "reached": reached,
        "total": particle_count,
        "virtual_time": f"{virtual_time:.6f}",
    }
    for field, intensity in zip(fields, intensities, strict=True):
        row[f"field_{field['index'] + 1}"] = f"{float(intensity):.6f}"
    log_state["writer"].writerow(row)
    log_state["file"].flush()
    print(
        f"Testing [{values}] -> reached {reached}/{particle_count}, "
        f"time {virtual_time:.3f}s, cost {cost:.3f}",
        flush=True,
    )
    return cost


def save_config(config, fields, intensities, output_path, seed):
    optimized = apply_intensities(config, fields, intensities, seed)
    with output_path.open("w", encoding="utf-8") as file:
        yaml.safe_dump(optimized, file, sort_keys=False)


def main():
    parser = argparse.ArgumentParser(
        description="Optimize segment field intensities with differential evolution."
    )
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG_PATH)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_PATH)
    parser.add_argument("--exec", dest="exec_path", type=Path, default=DEFAULT_EXEC_PATH)
    parser.add_argument("--log", type=Path, default=DEFAULT_LOG_PATH)
    parser.add_argument("--seed", type=int)
    parser.add_argument("--popsize", type=int, default=3)
    parser.add_argument("--maxiter", type=int, default=5)
    parser.add_argument("--no-polish", action="store_true")
    args = parser.parse_args()

    if not args.config.exists():
        print(f"Error: config not found: {args.config}")
        sys.exit(1)

    if not args.exec_path.exists():
        print(f"Error: executable not found: {args.exec_path}")
        sys.exit(1)

    try:
        config = load_config(args.config)
        fields = get_optimizable_fields(config)
    except Exception as error:
        print(f"Error: {error}")
        sys.exit(1)

    print("Starting optimization...", flush=True)
    print(f"Config: {args.config}", flush=True)
    print(f"Output: {args.output}", flush=True)
    print(f"Log: {args.log}", flush=True)
    print(f"Seed: {args.seed if args.seed is not None else config.get('random_seed', 42)}", flush=True)
    print(f"Target: {int(TARGET_FRACTION * 100)}% of particles", flush=True)
    print("Fields:", flush=True)
    for field in fields:
        print(
            f"  #{field['index'] + 1} "
            f"{field['start']} -> {field['end']} "
            f"initial={field['initial']:.1f} bounds={field['bounds']}",
            flush=True,
        )
    print("-" * 60, flush=True)

    bounds = [field["bounds"] for field in fields]
    csv_columns = ["eval", "cost", "best_cost", "reached", "total", "virtual_time"]
    csv_columns.extend(f"field_{field['index'] + 1}" for field in fields)

    log_file = args.log.open("w", encoding="utf-8", newline="")
    writer = csv.DictWriter(log_file, fieldnames=csv_columns)
    writer.writeheader()
    log_state = {
        "file": log_file,
        "writer": writer,
        "eval": 0,
        "best_cost": float("inf"),
    }

    try:
        result = differential_evolution(
            lambda intensities: run_simulation(
                intensities, config, fields, args.exec_path, args.seed, log_state
            ),
            bounds=bounds,
            popsize=args.popsize,
            maxiter=args.maxiter,
            tol=0.02,
            disp=False,
            polish=not args.no_polish,
        )
    except KeyboardInterrupt:
        log_file.close()
        print("\nOptimization interrupted. config.yaml was not modified.")
        sys.exit(0)
    except Exception as error:
        log_file.close()
        print(f"\nOptimization failed: {error}")
        sys.exit(1)

    log_file.close()

    print("-" * 60)
    print("Best intensities:")
    for field, intensity in zip(fields, result.x, strict=True):
        print(f"  #{field['index'] + 1}: {float(intensity):.2f}")
    print(f"Best cost: {result.fun:.3f}")

    save_config(config, fields, result.x, args.output, args.seed)
    print(f"Saved to {args.output}")
    print(f"Log saved to {args.log}")
    print(f"Original config unchanged: {args.config}")


if __name__ == "__main__":
    main()
