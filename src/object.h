#pragma once
#include "list.h"

namespace obj {

// First member (or base class) for any kernel object.
struct Base {
  list::node node;
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

