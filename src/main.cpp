// fnlsim.cpp.

#include "stdafx.h"

#include "sim.h"
#include "vmm.h"
#include "kheap.h"
#include "object.h"
#include "handle.h"

kheap::isll_head processes;
kheap::isll_head threads;

struct FnlProcess {
  obj::Base fno;

  uint64_t id;
  char* bin_name;
  kheap::isll_head regions;
  kheap::isll_head threads;
  handle::Entry* handles;
};

struct FnlThread {
  obj::Base fno;

  uint64_t id;
  int32_t core;
  FnlProcess* proc;
};

void init_process_obj(FnlProcess* proc, uint64_t id, char* bin_name) {
  proc->fno.type = obj::process;
  proc->id = id;
  proc->bin_name = bin_name;
  proc->regions = kheap::make_isll();
  proc->threads = kheap::make_isll();
  proc->handles = handle::make_table();
}

void init_thread_obj(FnlThread* thread, FnlProcess* proc, uint64_t id) {
  thread->fno.type = obj::thread;
  thread->id = id;
  thread->proc = proc;
  thread->core = -1;
}

void make_sys_process(char* name, uint64_t thread_id, uint64_t proc_id) {
  FnlProcess* proc = kheap::alloc_t<FnlProcess>(1);
  init_process_obj(proc, proc_id, name);

  FnlThread* thread = kheap::alloc_t<FnlThread>(1);
  init_thread_obj(thread, proc, thread_id);

  kheap::push_isll(processes, (kheap::isll_entry*)proc);
  kheap::push_isll(threads, (kheap::isll_entry*)thread);
}

namespace hal {

struct CoreCBlock {
  int core_id;
  kheap::isll_head ready_th;
  kheap::isll_head other_th;
};

void init_cblock(int core_id) {
  CoreCBlock* cblk = kheap::alloc_t<CoreCBlock>(1);
  cblk->core_id = core_id;
  cblk->ready_th = kheap::make_isll();
  cblk->other_th = kheap::make_isll();
  sim::kern_gs(cblk);
}

void init_core0() {
  init_cblock(0);
}

}

namespace exec {

void init() {
  processes = kheap::make_isll();
  threads = kheap::make_isll();
  make_sys_process(":sys0", 1UL, 1UL);

}

}

unsigned long __stdcall fnl_init(void*) {
  kheap::init();
  hal::init_core0();
  vmm::init();
  exec::init();
  return 0;
}

int main() {
  sim::init();
  sim::core_start(0, &fnl_init);
  sim::run();
  return 0;
}

