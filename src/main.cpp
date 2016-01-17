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
  HANDLE win_th;
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
}

void make_kernel_process() {
  FnlProcess* proc = kheap::make_isll_node<FnlProcess>();
  init_process_obj(proc, 1, ":sys0");

  FnlThread* thread = kheap::make_isll_node<FnlThread>();
  init_thread_obj(thread, proc, 1);

  kheap::push_isll(processes, (kheap::isll_entry*)proc);
  kheap::push_isll(threads, (kheap::isll_entry*)thread);
}

namespace exec {

void init() {
  processes = kheap::make_isll();
  threads = kheap::make_isll();
  make_kernel_process();
}

}

unsigned long __stdcall fnl_init(void*) {
  kheap::init();
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

