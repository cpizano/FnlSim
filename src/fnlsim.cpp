// fnlsim.cpp.

#include "stdafx.h"

#include "vmm.h"
#include "kheap.h"


kheap::isll_head processes;
kheap::isll_head threads;

namespace obj {

struct Base {
  kheap::isll_entry entry;
  uint64_t refcnt;
  uint16_t type;
};

enum Type {
  vacant = 0,
  sentry = 1,
  invalid = 2,

  process = 32,
  thread = 33,
};

}  //========================

namespace handle {

const uint32_t first_lev_count = 64U;

struct Entry {
  uint16_t type;
  uint16_t access;
  uint32_t ob_rel;
};

Entry* make_table() {
  Entry* e = (Entry*)kheap::alloc(sizeof(Entry) * first_lev_count);
  e[0].type = obj::sentry;
  return e;
}

void add_obj(obj::Base* ob, handle::Entry* table) {
  uint32_t ix = table[0].ob_rel;

  for (; ix != first_lev_count; ++ix) {
    if (table[ix].type == obj::vacant) {
      table[ix].type = ob->type;
      table[ix].ob_rel = kheap::get_reladdr(ob);
      table[0].ob_rel = (ix + 1) % first_lev_count;
      return;
    }
  }
  KPANIC;
}

}  // =======================

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
  init_process_obj(proc, 1, ":kernel");

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

int main() {
  kheap::init();
  vmm::init();
  exec::init();
  return 0;
}

