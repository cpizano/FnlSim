#include "stdafx.h"
#include "sim.h"

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

void* ctx_int_fiber = nullptr;

volatile int strange = 0;

unsigned long __stdcall start_tfn(void* p) {
  auto ctx = reinterpret_cast<StartCtx*>(p);
  ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
  ctx_int_fiber = ctx->fn();
  ::SuspendThread(GetCurrentThread());
rip_here:
  strange = strange + 1;
  while (true) {
    ::SwitchToFiber(ctx_int_fiber);
    // We hit the __debugbreak, below. Which should not happend
    // given that SwitchToFiber() should call the main.cpp interrupt() which calls'
    // schedule() which calls switch_context() which should switch bettwen
    // sys_routine1() and sys_routine2() either of those just loop forever so in
    // theory only way to come back here is via run()'s SetThreadContext which
    // should reset the RIP back to 'rip_here'.
    //
    // Commenting the debugbreak() below cause the other "runaway condition"
    // __debugbreak(); in main.cpp to hit.
    //
    __debugbreak();
  }
  return 0;
}

void core_start(int core_id, StartFn start) {
  Core* core = &cores[core_id];
  auto ctx = new StartCtx{start};
  core->win_th = ::CreateThread(nullptr, stack_size, start_tfn, ctx, 0, nullptr);
#if 0
  while (true) {
    ::SuspendThread(core->win_th);
    auto count = ::ResumeThread(core->win_th);
    if (count == 2UL)
      break;
    ::Sleep(10);
  }
#else
  ::Sleep(300);
#endif

  core->th_ctx.ContextFlags = CONTEXT_ALL;
  if (!::GetThreadContext(core->win_th, &core->th_ctx))
    __debugbreak();
  ::ResumeThread(core->win_th);
}

void* make_context(ThreadFn fn) {
  return ::CreateFiberEx(0, stack_size, FIBER_FLAG_FLOAT_SWITCH, fn, nullptr);
}


void switch_context(void* context) {
  wprintf(L"ctx is %p\n", context);
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
      cores[0].th_ctx.ContextFlags = CONTEXT_ALL;
      cores[0].th_ctx.Rip -= 0x14;
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