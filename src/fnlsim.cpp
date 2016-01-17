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


namespace sim {

struct Core {
  HANDLE win_th;
  CONTEXT ctx;
};

const int num_cores = 4;
Core cores[num_cores];
HANDLE cport;

unsigned long __stdcall holder_tfn(void*) {
  KPANIC;
  return 0;
}

void init_core(Core* core) {
  core->win_th = ::CreateThread(nullptr, 0, holder_tfn, nullptr, CREATE_SUSPENDED, nullptr);
  CHECKNE(core->win_th, nullptr);
  core->ctx.ContextFlags = CONTEXT_ALL;
  ::GetThreadContext(core->win_th, &core->ctx);
}

void core_start(int core_id, void* start) {
  Core& core = cores[core_id];
  // This only works with newly created (suspended) threads.
  core.ctx.Rcx = (DWORD64)start;
  core.ctx.ContextFlags = CONTEXT_INTEGER;
  ::SetThreadContext(core.win_th, &core.ctx);
  ::ResumeThread(core.win_th);
}

void init() {
  cport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  CHECKNE(cport, nullptr);
  for (int ix = 0; ix != num_cores; ++ix) {
    init_core(&cores[ix]);
  }
}

void run() {
  unsigned long bytes = 0;
  UINT_PTR key = 0;
  OVERLAPPED* ov = nullptr;

  wprintf(L"fnl sim running. %d cores x64\n", num_cores);

  while (true) {
    BOOL rv =::GetQueuedCompletionStatus(cport, &bytes, &key, &ov, INFINITE);
    if (!rv) {
      wprintf(L"got cp error\n");
    }
    else {
      wprintf(L"got event. exiting\n");
      break;
    }
  }
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

