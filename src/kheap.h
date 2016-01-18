#pragma once

namespace vmm {
  struct Region;
}

namespace kheap {

void* alloc_raw(uint64_t size);

template <typename N> N* alloc_t(int32_t c) {
  return (N*)alloc_raw(sizeof(N)*c);
}
void  free(void* mem);

uint32_t get_reladdr(void* add);
void* get_fulladdr(uint32_t readd);

void init();
vmm::Region* get_region();

}
