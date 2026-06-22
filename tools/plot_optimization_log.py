#!/usr/bin/env python3

import argparse
import csv
from pathlib import Path


def load_points(path):
    with path.open("r", encoding="utf-8", newline="") as file:
        rows = list(csv.DictReader(file))

    if not rows:
        raise ValueError("Log file is empty")

    points = []
    for row in rows:
        points.append(
            {
                "eval": int(row["eval"]),
                "cost": float(row["cost"]),
                "best_cost": float(row["best_cost"]),
            }
        )
    return points


def scale(value, min_value, max_value, out_min, out_max):
    if max_value <= min_value:
        return (out_min + out_max) / 2
    ratio = (value - min_value) / (max_value - min_value)
    return out_min + ratio * (out_max - out_min)


def make_polyline(points, x_min, x_max, y_min, y_max, left, top, width, height, key):
    coords = []
    for point in points:
        x = scale(point["eval"], x_min, x_max, left, left + width)
        y = scale(point[key], y_min, y_max, top + height, top)
        coords.append(f"{x:.1f},{y:.1f}")
    return " ".join(coords)


def write_svg(points, output_path):
    width = 960
    height = 540
    left = 80
    right = 40
    top = 50
    bottom = 70
    plot_width = width - left - right
    plot_height = height - top - bottom

    x_min = min(point["eval"] for point in points)
    x_max = max(point["eval"] for point in points)
    y_values = [point["cost"] for point in points] + [point["best_cost"] for point in points]
    y_min = min(y_values)
    y_max = max(y_values)

    cost_line = make_polyline(
        points, x_min, x_max, y_min, y_max, left, top, plot_width, plot_height, "cost"
    )
    best_line = make_polyline(
        points, x_min, x_max, y_min, y_max, left, top, plot_width, plot_height, "best_cost"
    )

    last = points[-1]
    best = min(points, key=lambda point: point["best_cost"])

    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
  <rect width="100%" height="100%" fill="#f8fafc" />
  <text x="{left}" y="30" font-family="Arial, sans-serif" font-size="24" fill="#0f172a">Optimization Progress</text>
  <text x="{left}" y="{height - 20}" font-family="Arial, sans-serif" font-size="14" fill="#334155">Evaluation</text>
  <text x="20" y="{top - 10}" font-family="Arial, sans-serif" font-size="14" fill="#334155">Cost</text>

  <line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_height}" stroke="#94a3b8" stroke-width="1" />
  <line x1="{left}" y1="{top + plot_height}" x2="{left + plot_width}" y2="{top + plot_height}" stroke="#94a3b8" stroke-width="1" />

  <polyline fill="none" stroke="#cbd5e1" stroke-width="2" points="{cost_line}" />
  <polyline fill="none" stroke="#2563eb" stroke-width="3" points="{best_line}" />

  <rect x="{left + 20}" y="{top + 20}" width="14" height="4" fill="#cbd5e1" />
  <text x="{left + 42}" y="{top + 25}" font-family="Arial, sans-serif" font-size="14" fill="#334155">Current cost</text>
  <rect x="{left + 160}" y="{top + 20}" width="14" height="4" fill="#2563eb" />
  <text x="{left + 182}" y="{top + 25}" font-family="Arial, sans-serif" font-size="14" fill="#334155">Best-so-far cost</text>

  <text x="{left}" y="{top + plot_height + 28}" font-family="Arial, sans-serif" font-size="12" fill="#475569">1</text>
  <text x="{left + plot_width - 40}" y="{top + plot_height + 28}" font-family="Arial, sans-serif" font-size="12" fill="#475569">{last["eval"]}</text>
  <text x="{left - 55}" y="{top + 5}" font-family="Arial, sans-serif" font-size="12" fill="#475569">{y_max:.1f}</text>
  <text x="{left - 55}" y="{top + plot_height}" font-family="Arial, sans-serif" font-size="12" fill="#475569">{y_min:.1f}</text>

  <text x="{left + plot_width - 260}" y="{top + 50}" font-family="Arial, sans-serif" font-size="14" fill="#0f172a">Best cost: {best["best_cost"]:.3f}</text>
  <text x="{left + plot_width - 260}" y="{top + 72}" font-family="Arial, sans-serif" font-size="14" fill="#0f172a">Evaluations: {last["eval"]}</text>
</svg>
"""

    output_path.write_text(svg, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Build a simple SVG chart from optimization_log.csv")
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, default=Path("optimization_progress.svg"))
    args = parser.parse_args()

    points = load_points(args.log)
    write_svg(points, args.output)
    print(f"Saved graph to {args.output}")


if __name__ == "__main__":
    main()
