#pragma once

namespace obj {

// First member (or base class) for any kernel object.
struct Base {
  kheap::isll_entry entry;
  uint64_t refcnt;
  uint16_t type;
};

enum Type {
  vacant = 0,
  sentry = 1,
  invalid = 2,

  process = 32,
  thread = 33,
};

}

