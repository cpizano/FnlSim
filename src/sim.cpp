#include "stdafx.h"
#include "sim.h"


namespace sim {

struct Core {
  HANDLE win_th;
  void* fiber;
};

Core cores[num_cores];
HANDLE cport;

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

struct StartCtx {
  Core* core;
  StartFn fn;
};

unsigned long __stdcall start_tfn(void* p) {
  auto ctx = reinterpret_cast<StartCtx*>(p);
  ctx->core->fiber = ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
  int rv = ctx->fn();
  KPANIC;
  return 0;
}

void core_start(int core_id, StartFn start) {
  Core* core = &cores[core_id];
  auto ctx = new StartCtx{core, start};
  core->win_th = ::CreateThread(nullptr, stack_size, start_tfn, ctx, 0, nullptr);
}

void* make_thread(ThreadFn fn) {
  return ::CreateFiberEx(stack_size, 0, FIBER_FLAG_FLOAT_SWITCH, fn, nullptr);
}

void init() {
  cport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  CHECKNE(cport, nullptr);
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