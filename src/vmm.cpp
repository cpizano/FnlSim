#include "stdafx.h"
#include "vmm.h"
#include "kheap.h"

namespace vmm {

list::list vm_regions;

void add_region(Region* r) {
  Region* n = kheap::alloc_t<Region>(1);
  *n = *r;
  list::push_back(&vm_regions, &n->node);
}

int init() {
  list::init(&vm_regions);
  kheap::init();
  return 0;
}


}