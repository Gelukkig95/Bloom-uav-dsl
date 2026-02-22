import sys
from pathlib import Path
import re

def parse_kv_config(path: str) -> dict:
    cfg = {}
    with open(path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            if "=" not in line:
                raise SystemExit(f"{path}:{lineno}: expected 'key = value', got: {line}")
            k, v = map(str.strip, line.split("=", 1))
            if not re.fullmatch(r"-?\d+(\.\d+)?", v):
                raise SystemExit(f"{path}:{lineno}: value must be number, got: {v}")
            cfg[k] = float(v) if "." in v else int(v)
    return cfg

CPP_TEMPLATE = r'''
#include <cstdint>
#include <cstdio>
#include <chrono>
#include "runtime/bloom_runtime.hpp"
#include <fstream>

static constexpr int W = __W__;
static constexpr int H = __H__;
static constexpr int TILE = __TILE__;
static constexpr float VAR_THRESHOLD = __VAR_THR__;
static constexpr uint32_t SEED = __SEED__;
static constexpr float BRIGHT_THR = __BRIGHT_THR__;

// heapless: fixed buffers
static uint8_t g_frame[W * H];
static float   g_mean_map[((W + TILE - 1) / TILE) * ((H + TILE - 1) / TILE)];
static float   g_var_map [((W + TILE - 1) / TILE) * ((H + TILE - 1) / TILE)];
static uint8_t g_anom_map[((W + TILE - 1) / TILE) * ((H + TILE - 1) / TILE)];

static uint8_t g_arena_mem[32 * 1024];
static Arena g_arena;

static void tile_stats_demo() {
  g_arena.init(g_arena_mem, sizeof(g_arena_mem));
  g_arena.reset();

  Grid2D<uint8_t> frame{ g_frame, W, H, W };
  generate_test_frame(frame, SEED);

  const int tiles_x = (W + TILE - 1) / TILE;
  const int tiles_y = (H + TILE - 1) / TILE;

  auto t0 = std::chrono::high_resolution_clock::now();

  int idx = 0;
  for (int ty = 0; ty < tiles_y; ++ty) {
    for (int tx = 0; tx < tiles_x; ++tx) {
      int x0 = tx * TILE;
      int y0 = ty * TILE;
      TileStats st = tile_stats_u8(frame, x0, y0, TILE, TILE);

      g_mean_map[idx] = st.mean;
      g_var_map[idx]  = st.var;

      uint8_t anom_var = (st.var > VAR_THRESHOLD) ? 1 : 0;
      uint8_t anom_bri = (st.mean > BRIGHT_THR) ? 1 : 0;
      g_anom_map[idx] = (anom_var || anom_bri) ? 1 : 0;

      idx++;
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  int anom_count = 0;
  for (int i = 0; i < tiles_x * tiles_y; ++i) anom_count += g_anom_map[i];

  std::printf("cfg: %dx%d tile=%d var_thr=%.1f bright_thr=%.1f seed=%u\n",
              W, H, TILE, (double)VAR_THRESHOLD, (double)BRIGHT_THR, (unsigned)SEED);
  std::printf("tile_stats_demo: %dx%d, tile=%d => tiles=%dx%d\n", W, H, TILE, tiles_x, tiles_y);
  std::printf("time: %.3f ms, anomalies: %d\n", ms, anom_count);

  std::printf("mean_map[0..7]: ");
  for (int i = 0; i < 8 && i < tiles_x * tiles_y; ++i) std::printf("%.1f ", g_mean_map[i]);
  std::printf("\n");

  std::printf("var_map[0..7]: ");
  for (int i = 0; i < 8 && i < tiles_x * tiles_y; ++i) std::printf("%.1f ", g_var_map[i]);
  std::printf("\n");

  std::printf("anom_map[0..7]: ");
  for (int i = 0; i < 8 && i < tiles_x * tiles_y; ++i) std::printf("%d ", (int)g_anom_map[i]);
  std::printf("\n");
}

int main() {
  tile_stats_demo();
  return 0;
}
'''

def main():
    if len(sys.argv) >= 2 and sys.argv[1] in ("-h", "--help"):
        print("Usage: python bloomc.py <input.bloom> [output.cpp]")
        sys.exit(0)

    if len(sys.argv) < 2:
        print("Usage: python bloomc.py <input.bloom> [output.cpp]")
        sys.exit(1)

    src = Path(sys.argv[1])
    out = Path(sys.argv[2]) if len(sys.argv) >= 3 else Path("out.cpp")

    # Defaults
    params = {
        "width": 160,
        "height": 120,
        "tile": 32,
        "var_threshold": 400.0,
        "seed": 1,
        "brightness_threshold": 9999.0,  # effectively disabled
    }

    # Override with .bloom (unknown keys ignored for now)
    user_cfg = parse_kv_config(str(src))
    for k in list(user_cfg.keys()):
        if k not in params:
            user_cfg.pop(k)
    params.update(user_cfg)

    # Normalize types
    params["width"] = int(params["width"])
    params["height"] = int(params["height"])
    params["tile"] = int(params["tile"])
    params["seed"] = int(params["seed"])
    params["var_threshold"] = float(params["var_threshold"])
    params["brightness_threshold"] = float(params["brightness_threshold"])

    print("[FINAL_PARAMS]", params)

    cpp = CPP_TEMPLATE
    cpp = cpp.replace("__W__", str(params["width"]))
    cpp = cpp.replace("__H__", str(params["height"]))
    cpp = cpp.replace("__TILE__", str(params["tile"]))
    cpp = cpp.replace("__VAR_THR__", str(params["var_threshold"]))
    cpp = cpp.replace("__SEED__", str(params["seed"]))
    cpp = cpp.replace("__BRIGHT_THR__", str(params["brightness_threshold"]))

    out.write_text(cpp, encoding="utf-8")
    print(f"[OK] Generated {out}")

if __name__ == "__main__":
    main()