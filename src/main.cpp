// fnlsim.cpp.

#include "stdafx.h"

#include "sim.h"
#include "vmm.h"
#include "kheap.h"
#include "object.h"
#include "handle.h"

list::list processes;
list::list threads;

struct FnlProcess {
  obj::Base fno;

  uint64_t id;
  char* bin_name;
  list::list regions;
  list::list threads;
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
  list::init(&proc->regions);
  list::init(&proc->threads);
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

  list::push_back(&processes, &proc->fno.node);
  list::push_back(&threads, &thread->fno.node);
}

namespace hal {

struct CoreCBlock {
  int core_id;
  list::list ready_th;
  list::list other_th;
};

void init_cblock(int core_id) {
  CoreCBlock* cblk = kheap::alloc_t<CoreCBlock>(1);
  cblk->core_id = core_id;
  list::init(&cblk->ready_th);
  list::init(&cblk->other_th);
  sim::kern_gs(cblk);
}

void init_core0() {
  init_cblock(0);
}

}

namespace exec {

void init() {
  list::init(&processes);
  list::init(&threads);
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

