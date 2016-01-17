#include "stdafx.h"
#include "kheap.h"
#include "vmm.h"

HANDLE heap = nullptr;
char*  heap_start = nullptr;

const uint64_t heap_sz = 16 * 1024 * 1024;

void kheap::init() {
  heap = ::HeapCreate(HEAP_GENERATE_EXCEPTIONS, heap_sz, heap_sz);
  CHECKNE(heap, NULL);
  get_region();
}

void* kheap::alloc(uint64_t size) {
  return ::HeapAlloc(heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, size);
}

void* kheap::alloc_a16(uint64_t size) {
  return ::HeapAlloc(heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS | HEAP_CREATE_ALIGN_16, size);
}

void kheap::free(void * mem) {
  ::HeapFree(heap, 0, mem);
}

uint32_t kheap::get_reladdr(void * add) {
  return uint32_t((char*)add - heap_start);
}

void * kheap::get_fulladdr(uint32_t readd) {
  return (void*)(heap_start + readd);
}


kheap::isll_head kheap::make_isll() {
  SLIST_HEADER* hdr = (SLIST_HEADER*)alloc_a16(sizeof(SLIST_HEADER));
  ::InitializeSListHead(hdr);
  return hdr;
}

void kheap::destroy_isll(kheap::isll_head head) {
  kheap::free(head);
}

void kheap::push_isll(kheap::isll_head head, kheap::isll_entry * item) {
  ::InterlockedPushEntrySList(head, item);
}

kheap::isll_entry * kheap::pop_isll(isll_head head) {
  return ::InterlockedPopEntrySList(head);
}

vmm::Region * kheap::get_region() {
  static vmm::Region heapreg = {};

  if (heapreg.start) {
    return &heapreg;
  }

  PROCESS_HEAP_ENTRY phe = { 0 };
  while (true) {
    if (!::HeapWalk(heap, &phe))
      break;
    switch (phe.wFlags) {
    case 0:
      break;
    case PROCESS_HEAP_REGION:
      heap_start = (char*)phe.lpData;
      heapreg.start = phe.lpData;
      heapreg.len = phe.Region.dwCommittedSize;
      heapreg.type = vmm::kern_heap;
      break;
    case PROCESS_HEAP_UNCOMMITTED_RANGE:
      CHECK(phe.cbData, 0UL);
      break;
    case PROCESS_HEAP_ENTRY_BUSY:
      break;
    default:
      KPANIC;
    }
  }
  return &heapreg;
}