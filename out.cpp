
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <fstream>
#include "runtime/bloom_runtime.hpp"

static constexpr int W = 160;
static constexpr int H = 120;
static constexpr int TILE = 16;
static constexpr float VAR_THRESHOLD = 200.0;
static constexpr uint32_t SEED = 123;
static constexpr float BRIGHT_THR = 180.0;

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

  // ---- write JSON output (out.json) ----
  {
    std::ofstream os("out.json", std::ios::binary);
    os << "{\n";
    os << "  \"width\": " << W << ",\n";
    os << "  \"height\": " << H << ",\n";
    os << "  \"tile\": " << TILE << ",\n";
    os << "  \"tiles_x\": " << tiles_x << ",\n";
    os << "  \"tiles_y\": " << tiles_y << ",\n";
    os << "  \"var_threshold\": " << (double)VAR_THRESHOLD << ",\n";
    os << "  \"brightness_threshold\": " << (double)BRIGHT_THR << ",\n";
    os << "  \"seed\": " << (unsigned)SEED << ",\n";
    os << "  \"anomalies\": " << anom_count << ",\n";

    auto write_float_array = [&](const char* name, const float* a, int n) {
      os << "  \"" << name << "\": [";
      for (int i = 0; i < n; ++i) {
        if (i) os << ", ";
        os << (double)a[i];
      }
      os << "]";
    };

    auto write_u8_array = [&](const char* name, const uint8_t* a, int n) {
      os << "  \"" << name << "\": [";
      for (int i = 0; i < n; ++i) {
        if (i) os << ", ";
        os << (int)a[i];
      }
      os << "]";
    };

    const int ntiles = tiles_x * tiles_y;
    write_float_array("mean_map", g_mean_map, ntiles); os << ",\n";
    write_float_array("var_map",  g_var_map,  ntiles); os << ",\n";
    write_u8_array  ("anom_map", g_anom_map, ntiles); os << "\n";

    os << "}\n";
  }
  std::printf("wrote out.json (%dx%d tiles)\n", tiles_x, tiles_y);
}

int main() { tile_stats_demo(); return 0; }
