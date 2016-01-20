// fnlsim.cpp.

#include "stdafx.h"

#include "list.h"
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
  void* ctx;
};

template <typename T>
T* get(list::node* n) {
  return node_to_entry(n, T, fno.node);
}

struct CoreCBlock {
  int core_id;
  list::list ready_th;
  list::list other_th;
  FnlThread* current;
};

FnlProcess* make_process_obj(uint64_t id, char* bin_name) {
  FnlProcess* proc = kheap::alloc_t<FnlProcess>(1);
  proc->fno.type = obj::process;
  proc->id = id;
  proc->bin_name = bin_name;
  list::init(&proc->regions);
  list::init(&proc->threads);
  proc->handles = handle::make_table();
  return proc;
}

FnlThread* make_thread_obj(FnlProcess* proc, uint64_t id) {
  FnlThread* thread = kheap::alloc_t<FnlThread>(1);
  thread->fno.type = obj::thread;
  thread->id = id;
  thread->core = -1;
  thread->proc = proc;
  thread->ctx = nullptr;
  return thread;
}

namespace exec {

void init_core_block(int core_id) {
  CoreCBlock* cblk = kheap::alloc_t<CoreCBlock>(1);
  cblk->core_id = core_id;
  cblk->current = nullptr;
  list::init(&cblk->ready_th);
  list::init(&cblk->other_th);
  sim::kern_gs(cblk);
}

CoreCBlock* get_core_block() {
  return reinterpret_cast<CoreCBlock*>(sim::kern_gs(0));
}

void init_all_cores() {
  for (int ix = 1; ix != sim::num_cores; ++ix) {
    init_core_block(ix);
  }
}

void add_thread(FnlThread* thread, sim::ThreadFn fn) {
  thread->ctx = sim::make_thread(fn);
  CoreCBlock* cb = exec::get_core_block();
  list::push_back(&cb->ready_th, &thread->fno.node);
}

void __stdcall sys_routine(void* p) {
  volatile uint64_t x = 0;
  while (true) {
    ++x;
  }
}

void schedule() {
  CoreCBlock* cb = exec::get_core_block();
  if (cb->current)
    return;
  list::node* node = list::pop_front(&cb->ready_th);
  if (!node)
    return;
  auto thread = get<FnlThread>(node);
  thread->core = cb->core_id;
  sim::run_thread(thread->ctx);
}

void make_sys_process() {
  FnlProcess* proc = make_process_obj(1UL, ":sys0");
  list::push_back(&processes, &proc->fno.node);
  // the idle thread.
  FnlThread* idle = make_thread_obj(proc, 1UL);
  list::push_back(&threads, &idle->fno.node);
  // the sys thread.
  FnlThread* sys = make_thread_obj(proc, 2UL);
  list::push_back(&threads, &sys->fno.node);
  add_thread(sys, &sys_routine);
  schedule();
}

void init() {
  init_core_block(0);
  list::init(&processes);
  list::init(&threads);
  make_sys_process();
  init_all_cores();
}

}

int fnl_init() {
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

