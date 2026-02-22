# Bloom DSL (v0.2)

Ultra-light DSL for **on-drone tile statistics & anomaly detection** (GeoAI).  
Bloom compiles a `.bloom` spec into **C++17** and runs on a tiny, heapless runtime.

## What it does (MVP)
- Generates a deterministic test frame (seeded)
- Computes per-tile **mean / variance**
- Detects anomalies using:
  - `var > var_threshold` OR `mean > brightness_threshold`
- Exports results to **out.json** (`mean_map` / `var_map` / `anom_map`)

## Quickstart (Windows + MSYS2 UCRT64)
```powershell
cd D:\bloom
.\scripts\build.ps1

You should see console output like:

cfg: ... bright_thr=... seed=...
and out.json created in the repo root.

Example Bloom file

examples/tile_stats.bloom

width = 160
height = 120
tile = 16
seed = 123
var_threshold = 200
brightness_threshold = 180

Output

out.json includes:

metadata (width, height, tile, tiles_x, tiles_y, thresholds, seed)
mean_map, var_map (float arrays)
anom_map (0/1 array)
total anomalies count

Example snippet:

{
  "width": 160,
  "height": 120,
  "tile": 16,
  "tiles_x": 10,
  "tiles_y": 8,
  "anomalies": 37,
  "mean_map": [ ... ],
  "var_map": [ ... ],
  "anom_map": [ ... ]
}

Repo layout

bloomc.py — compiler (DSL → C++)

runtime/ — tiny heapless runtime (Grid2D, Arena, tile stats, test frames)

examples/ — sample .bloom

scripts/build.ps1 — one-shot build/run

Roadmap

RGB/multi-band frames → color_threshold
Real input frames (file/camera integration)
Proper grammar with Lark (params {} / pipeline {})
Onboard-friendly output formats (binary / tiled heatmap)

