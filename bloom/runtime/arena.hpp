#pragma once
#include <cstddef>
#include <cstdint>

struct Arena {
  uint8_t* base;
  size_t cap;
  size_t off;

  void init(void* mem, size_t bytes) {
    base = static_cast<uint8_t*>(mem);
    cap = bytes;
    off = 0;
  }

  void reset() { off = 0; }

  void* alloc(size_t n, size_t align = 8) {
    size_t p = (off + align - 1) & ~(align - 1);
    if (p + n > cap) return nullptr;
    off = p + n;
    return base + p;
  }
};
