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

typedef SLIST_HEADER* isll_head;
typedef SLIST_ENTRY   isll_entry;

isll_head make_isll();
void destroy_isll(isll_head head);
void push_isll(isll_head head, isll_entry* item);
isll_entry* pop_isll(isll_head head);



void init();
vmm::Region* get_region();

}
