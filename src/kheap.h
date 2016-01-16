#pragma once

namespace vmm {
  struct Region;
}

namespace kheap {

void* alloc(uint64_t size);
void* alloc_a16(uint64_t size);
void  free(void* mem);

uint32_t get_reladdr(void* add);
void* get_fulladdr(uint32_t readd);

typedef SLIST_HEADER* isll_head;
typedef SLIST_ENTRY   isll_entry;

isll_head make_isll();
void destroy_isll(isll_head head);
void push_isll(isll_head head, isll_entry* item);
isll_entry* pop_isll(isll_head head);

template <typename N>
N* make_isll_node() {
  return (N*)alloc_a16(sizeof(N));
}

void init();
vmm::Region* get_region();

}
