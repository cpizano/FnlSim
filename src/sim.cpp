#include "stdafx.h"
#include "sim.h"
#include "threadjack_x64.h"

#include <intrin.h>

namespace sim {

struct Core {
  HANDLE win_th;
};

Core cores[num_cores];
HANDLE cport;
volatile uint64_t ctx_switches = 0;

__declspec(thread) void* k_gs;
__declspec(thread) void* u_gs;

void* kern_gs(void* new_gs) {
  if (new_gs)
    k_gs = new_gs;
  return k_gs;
}

void* user_gs(void* new_gs) {
  if (new_gs)
    u_gs = new_gs;
  return u_gs;
}

struct StartCtx {
  StartFn init_fn;
};

PFIBER_START_ROUTINE interrupt_fn = nullptr;

unsigned long __stdcall start_tfn(void* p) {
  ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
  auto ctx = reinterpret_cast<StartCtx*>(p);
  interrupt_fn = ctx->init_fn();

  while (true) {
    YieldProcessor();
  }
  return 0;
}

void core_start(int core_id, StartFn init_fn) {
  Core* core = &cores[core_id];
  auto ctx = new StartCtx{init_fn};
  core->win_th = ::CreateThread(nullptr, stack_size, start_tfn, ctx, 0, nullptr);
}

void* make_context(ThreadFn fn) {
  return ::CreateFiberEx(4096 * 2, stack_size, FIBER_FLAG_FLOAT_SWITCH, fn, nullptr);
}

void switch_context(void* context) {
  ++ctx_switches;
  ::SwitchToFiber(context);
}

void init() {
  cport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  CHECKNE(cport, nullptr);
  threadjack::init();
}

void run() {
  unsigned long bytes = 0;
  UINT_PTR key = 0;
  OVERLAPPED* ov = nullptr;

  wprintf(L"fnl x64 sim running. %d simulated cores\n", num_cores);

  if (!::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
    __debugbreak();

  while (true) {
    BOOL rv = ::GetQueuedCompletionStatus(cport, &bytes, &key, &ov, 30);
    if (!rv) {
      wprintf(L"ctx switches %llu\r", ctx_switches);
      while (!threadjack::interrupt(cores[0].win_th, interrupt_fn, nullptr)) {
        YieldProcessor();
      }
    }
    else {
      wprintf(L"got event. exiting\n");
      break;
    }
  }
}

}