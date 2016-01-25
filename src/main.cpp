// fnlsim.cpp.

#include "stdafx.h"

#include "list.h"
#include "sim.h"
#include "vmm.h"
#include "kheap.h"
#include "object.h"
#include "handle.h"

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
  list::node snode;

  uint64_t id;
  int32_t core;
  FnlProcess* proc;
  void* ctx;
};

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

list::list processes;
list::list threads;

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
  thread->ctx = sim::make_context(fn);
  CoreCBlock* cb = exec::get_core_block();
  list::push_back(&cb->ready_th, &thread->snode);
}

volatile uint64_t xy = 0;

void __stdcall sys_routine1(void* p) {
  while (true) {
    ++xy;
  }
}

void __stdcall sys_routine2(void* p) {
  while (true) {
    --xy;
  }
}

void __stdcall sys_routine3(void* p) {
  while (true) {
    xy *= 2;
  }
}

int g_times = 0;

FnlThread* snode_get(list::node* n) {
  return node_to_entry(n, FnlThread, snode);
}

void schedule() {
  CoreCBlock* cb = exec::get_core_block();
  FnlThread* thread = nullptr;

  if (list::empty(&cb->ready_th)) {
    if (!cb->current)
      return;
  } else {
    auto thread = snode_get(list::pop_front(&cb->ready_th));
    // wprintf(L" %d switching to thread %llu\n", ++g_times, thread->id);
    if (cb->current) {
      CHECKNE(cb->current->id, thread->id);
      list::push_back(&cb->ready_th, &cb->current->snode);
    }
    cb->current = thread;
  }  
  sim::switch_context(cb->current->ctx);
}

void make_sys_process() {
  FnlProcess* proc = make_process_obj(1UL, ":sys0");
  list::push_back(&processes, &proc->fno.node);
  // the idle thread.
  FnlThread* idle = make_thread_obj(proc, 1UL);
  list::push_back(&threads, &idle->fno.node);
  // the sys thread 1.
  FnlThread* sys1 = make_thread_obj(proc, 2UL);
  list::push_back(&threads, &sys1->fno.node);
  add_thread(sys1, &sys_routine1);
  sys1 = nullptr;
  // the sys thread 2.
  FnlThread* sys2 = make_thread_obj(proc, 3UL);
  list::push_back(&threads, &sys2->fno.node);
  add_thread(sys2, &sys_routine2);
  sys2 = nullptr;
  // the sys thread 3.
  FnlThread* sys3 = make_thread_obj(proc, 4UL);
  list::push_back(&threads, &sys3->fno.node);
  add_thread(sys3, &sys_routine3);
}

void __stdcall interrupt(void* p) {
  schedule();
}

sim::ThreadFn init() {
  init_core_block(0);
  list::init(&processes);
  list::init(&threads);
  make_sys_process();
  return &interrupt;
}

}

sim::ThreadFn fnl_init() {
  vmm::init();
  return exec::init();
}

int main() {
  sim::init();
  sim::core_start(0, &fnl_init);
  sim::run();
  return 0;
}

