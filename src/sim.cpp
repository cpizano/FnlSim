#include "stdafx.h"
#include "sim.h"


namespace sim {

struct Core {
  HANDLE win_th;
  CONTEXT ctx;
};

Core cores[num_cores];
HANDLE cport;

unsigned long __stdcall holder_tfn(void*) {
  KPANIC;
  return 0;
}

void* kern_gs(void* new_gs) {
  __declspec(thread) static void* gs;
  if (new_gs) {
    void* tmp = gs;
    gs = new_gs;
    return tmp;
  }
  else {
    return gs;
  }
}

void* user_gs(void* new_gs) {
  __declspec(thread) static void* gs;
  if (new_gs) {
    void* tmp = gs;
    gs = new_gs;
    return tmp;
  }
  else {
    return gs;
  }
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
    BOOL rv = ::GetQueuedCompletionStatus(cport, &bytes, &key, &ov, INFINITE);
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