#include "stdafx.h"
#include "vmm.h"
#include "kheap.h"

namespace vmm {

kheap::isll_head vm_region;

void add_region(Region* r) {
  Region* n = kheap::make_isll_node<Region>();
  *n = *r;
  kheap::push_isll(vm_region, (kheap::isll_entry*)n);
}

int init() {
  vm_region = kheap::make_isll();
  add_region(kheap::get_region());

  return 0;
}


}