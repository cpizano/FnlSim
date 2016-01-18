#pragma once
#include "list.h"

namespace vmm {

enum Type {
  none,
  kern_heap,
};

struct Region {
  list::node node;
  Type type;
  void* start;
  uint64_t len;
};

int init();

};

