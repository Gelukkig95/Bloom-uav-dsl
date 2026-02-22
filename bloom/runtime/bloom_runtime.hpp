#pragma once
#include <cstdint>
#include <cstdio>
#include "arena.hpp"
#include "grid.hpp"

struct TileStats {
  float mean;
  float var;
};

inline void generate_test_frame(Grid2D<uint8_t> frame) {
  for (int y = 0; y < frame.h; ++y) {
    for (int x = 0; x < frame.w; ++x) {
      int v = (x * 255) / (frame.w - 1);
      if ((x / 10 + y / 10) % 2 == 0) v = (v + 20) > 255 ? 255 : (v + 20);
      frame.at(x, y) = (uint8_t)v;
    }
  }
  // anomaly patch (high variance)
  for (int y = 40; y < 72; ++y) {
    for (int x = 80; x < 112; ++x) {
      frame.at(x, y) = (uint8_t)((x * 37 + y * 91) & 255);
    }
  }
}

inline TileStats tile_stats_u8(const Grid2D<uint8_t>& frame,
                               int x0, int y0, int tw, int th) {
  int x1 = x0 + tw; if (x1 > frame.w) x1 = frame.w;
  int y1 = y0 + th; if (y1 > frame.h) y1 = frame.h;

  const int n = (x1 - x0) * (y1 - y0);
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
