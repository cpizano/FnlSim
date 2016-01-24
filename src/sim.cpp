#include "stdafx.h"
#include "sim.h"

#include <intrin.h>

namespace {

}

namespace sim {

struct Core {
  HANDLE win_th;
  CONTEXT th_ctx;
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
  StartFn fn;
};

void* ctx_orig_fiber = nullptr;
void* ctx_int_fiber = nullptr;

volatile long gate0 = 0;
volatile long gate1 = 0;

unsigned long __stdcall start_tfn(void* p) {
  auto ctx = reinterpret_cast<StartCtx*>(p);
  void* ff = ctx->fn();

  ctx_orig_fiber = ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
  ctx_int_fiber  = ::CreateFiberEx(4096 * 2, stack_size, FIBER_FLAG_FLOAT_SWITCH, (LPFIBER_START_ROUTINE)ff, nullptr);


  while (_InterlockedCompareExchange(&gate0, 1, 1) == 0L) {
    _InterlockedOr(&gate1, 1);
  }
 
#if 0
  wprintf(L"~ ctx is %p\n", ::GetCurrentFiber());
#endif

#if 0
  if (ctx_orig_fiber != ::GetCurrentFiber()) {
    ::SwitchToFiber(ctx_orig_fiber);
  }
#endif


  ::SwitchToFiber(ctx_int_fiber);

  while (true) {
    YieldProcessor();
  }

  return 0;
}

void core_start(int core_id, StartFn start) {
  Core* core = &cores[core_id];
  auto ctx = new StartCtx{start};
  core->win_th = ::CreateThread(nullptr, stack_size, start_tfn, ctx, 0, nullptr);

  while (true) {
    auto spin = _InterlockedCompareExchange(&gate1, 0, 1);
    if (spin)
      break;
    ::YieldProcessor();
  }

  ::SuspendThread(core->win_th);
  core->th_ctx.ContextFlags = CONTEXT_ALL;
  if (!::GetThreadContext(core->win_th, &core->th_ctx))
    __debugbreak();

  _InterlockedOr(&gate0, 1);
  ::ResumeThread(core->win_th);
}

void* make_context(ThreadFn fn) {
  return ::CreateFiberEx(4096 * 2, stack_size, FIBER_FLAG_FLOAT_SWITCH, fn, nullptr);
}


void switch_context(void* context) {
  //wprintf(L"ctx is %p\n", context);
  ::SwitchToFiber(context);
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
    BOOL rv = ::GetQueuedCompletionStatus(cport, &bytes, &key, &ov, 2000);
    if (!rv) {
      wprintf(L"timer\n");
      auto sc = ::SuspendThread(cores[0].win_th);
      if (sc > 0)
        __debugbreak();
      cores[0].th_ctx.ContextFlags = CONTEXT_FULL; //ONTEXT_CONTROL;
      if (!::SetThreadContext(cores[0].win_th, &cores[0].th_ctx))
        __debugbreak();
      ::ResumeThread(cores[0].win_th);
    }
    else {
      wprintf(L"got event. exiting\n");
      break;
    }
  }
}

}