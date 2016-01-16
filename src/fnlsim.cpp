// fnlsim.cpp.

#include "stdafx.h"

#include "vmm.h"
#include "kheap.h"

struct FnlHandleEntry {
  uint16_t type;
  uint16_t access;
  uint32_t ob_readd;
};

struct FnObject {
  kheap::isll_entry isl;
  uint64_t refcnt;
  uint64_t tagname;
};

struct FnlProcess {
  FnObject fno;

  uint64_t id;
  char* bin_name;
  kheap::isll_head regions;
  kheap::isll_head threads;
  FnlHandleEntry* hadles;
};

struct FnlThread {
  FnObject fno;

  uint64_t id;
  HANDLE backing_th;
};



int main() {
  kheap::init();
  vmm::init();
  return 0;
}

