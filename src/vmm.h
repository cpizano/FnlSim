#pragma once
#include "kheap.h"

namespace vmm {

enum Type {
  none,
  kern_heap,
};

struct Region {
  kheap::isll_entry isl;
  Type type;
  void* start;
  uint64_t len;
};

int init();

};

