
#include <cstdint>
#include <cstdio>
#include <chrono>
#include "runtime/bloom_runtime.hpp"

static constexpr int W = 160;
static constexpr int H = 120;
static constexpr int TILE = 32;
static constexpr float VAR_THRESHOLD = 400.0f;

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
  generate_test_frame(frame);

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
      g_anom_map[idx] = (st.var > VAR_THRESHOLD) ? 1 : 0;
      idx++;
    }
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  int anom_count = 0;
  for (int i = 0; i < tiles_x * tiles_y; ++i) anom_count += g_anom_map[i];

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

int bloom_main() {
  tile_stats_demo();
  return 0;
}

int main() { return bloom_main(); }
