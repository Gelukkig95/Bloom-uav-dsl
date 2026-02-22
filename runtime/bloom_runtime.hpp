#pragma once
#include <cstdint>
#include <cstdio>
#include <cmath>

#include "arena.hpp"
#include "grid.hpp"

// -------------------------
// Data types
// -------------------------
struct TileStats {
  float mean;
  float var;
};

// -------------------------
// Deterministic RNG (LCG)
// -------------------------
inline uint32_t bloom_lcg(uint32_t& s) {
  s = 1664525u * s + 1013904223u;
  return s;
}

// -------------------------
// Test frame generators
// -------------------------
// Default test frame (no seed): gradient + checker boost + fixed anomaly patch
inline void generate_test_frame(Grid2D<uint8_t> frame) {
  // gradient + checker
  for (int y = 0; y < frame.h; ++y) {
    for (int x = 0; x < frame.w; ++x) {
      int denom = (frame.w > 1) ? (frame.w - 1) : 1;
      int v = (x * 255) / denom;
      if (((x / 10) + (y / 10)) % 2 == 0) v = (v + 20) > 255 ? 255 : (v + 20);
      frame.at(x, y) = (uint8_t)v;
    }
  }

  // anomaly patch (high variance)
  int y0 = 40, y1 = 72;
  int x0 = 80, x1 = 112;
  if (y0 < 0) y0 = 0; if (x0 < 0) x0 = 0;
  if (y1 > frame.h) y1 = frame.h;
  if (x1 > frame.w) x1 = frame.w;

  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      frame.at(x, y) = (uint8_t)((x * 37 + y * 91) & 255);
    }
  }
}

// Seeded test frame: deterministic noise + a few repeatable "hot" patches
inline void generate_test_frame(Grid2D<uint8_t> frame, uint32_t seed) {
  // base: gradient + deterministic noise
  for (int y = 0; y < frame.h; ++y) {
    for (int x = 0; x < frame.w; ++x) {
      uint32_t r = bloom_lcg(seed);
      uint8_t noise = (uint8_t)((r >> 24) & 0x1F); // 0..31

      int denom = (frame.w > 1) ? (frame.w - 1) : 1;
      int base = (x * 255) / denom;
      int v = base + (int)noise;
      if (v > 255) v = 255;

      frame.at(x, y) = (uint8_t)v;
    }
  }

  // inject a few anomalies (repeatable with seed)
  for (int k = 0; k < 3; ++k) {
    uint32_t r = bloom_lcg(seed);
    int cx = (int)(r % (uint32_t)frame.w);
    r = bloom_lcg(seed);
    int cy = (int)(r % (uint32_t)frame.h);

    for (int dy = -3; dy <= 3; ++dy) {
      for (int dx = -3; dx <= 3; ++dx) {
        int x = cx + dx, y = cy + dy;
        if (0 <= x && x < frame.w && 0 <= y && y < frame.h) {
          int vv = (int)frame.at(x, y) + 120;
          frame.at(x, y) = (uint8_t)(vv > 255 ? 255 : vv);
        }
      }
    }
  }
}

// -------------------------
// Tile statistics
// -------------------------
inline TileStats tile_stats_u8(const Grid2D<uint8_t>& frame,
                              int x0, int y0, int tw, int th) {
  int x1 = x0 + tw; if (x1 > frame.w) x1 = frame.w;
  int y1 = y0 + th; if (y1 > frame.h) y1 = frame.h;

  const int w = (x1 - x0);
  const int h = (y1 - y0);
  const int n = w * h;
  if (n <= 0) return {0.0f, 0.0f};

  double sum = 0.0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) sum += frame.at(x, y);
  }
  double mean = sum / (double)n;

  double s2 = 0.0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      double d = (double)frame.at(x, y) - mean;
      s2 += d * d;
    }
  }
  double var = s2 / (double)n;

  return {(float)mean, (float)var};
}
