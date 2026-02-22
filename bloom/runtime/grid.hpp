#pragma once
#include <cstdint>
#include <cstddef>

template <typename T>
struct Grid2D {
  T* data;
  int w;
  int h;
  int stride; // elements per row

  inline T& at(int x, int y) { return data[y * stride + x]; }
  inline const T& at(int x, int y) const { return data[y * stride + x]; }
};
