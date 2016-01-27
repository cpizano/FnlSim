#include "stdafx.h"
#include "sim.h"
#include "threadjack_x64.h"

#include <intrin.h>

// The simulator design has two threads per core. One runs the client code and the other
// one runs the interrupt timeout (APIC) logic. The timeout hijacks the user thread via
// SuspendThread / GetContext / SetContext (which is rather slow) and then redirects to
// |interrupt_fn| which should run a scheduler which should call switch_context(), which
// swiches client threads which are handled as Windows Fibers.
//
//   Core    --> thread  (sim_th)  / core_start
//   APIC    --> thread  (int_th)
//   Thread  --> Fiber   make_context / switch_context

namespace sim {

struct Core {
  HANDLE sim_th;
  HANDLE int_th;
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

unsigned long __stdcall apic_fn(void* p) {
  auto core = reinterpret_cast<Core*>(p);
  while (true) {
    if (::SleepEx(30, true) == WAIT_IO_COMPLETION)
      break;
resume:
    if (!core->sim_th)
      continue;
    while (!threadjack::interrupt(core->sim_th, interrupt_fn, nullptr)) {
      YieldProcessor();
    }
  }

  // some waiting logic here.
  goto resume;
  return 0;
}
void __stdcall APC_Exit(ULONG_PTR) {
  ::ExitThread(0UL);
}

void core_start(int core_id, StartFn init_fn) {
  Core* core = &cores[core_id];
  auto ctx = new StartCtx{init_fn};
  // TODO: set processor afinity to these two theads.
  core->sim_th = ::CreateThread(nullptr, stack_size, start_tfn, ctx, 0, nullptr);
  core->int_th = ::CreateThread(nullptr, stack_size, apic_fn, core, 0, nullptr);
}

void core_stop(int core_id) {
  auto core = cores[core_id];
  if (!core.int_th)
    return;
  if (::QueueUserAPC(APC_Exit, core.int_th, 0UL))
    ::WaitForSingleObject(core.int_th, INFINITE);
  ::TerminateThread(core.sim_th, 0x33);
  ::CloseHandle(core.int_th);
  ::CloseHandle(core.sim_th); 
}

void* make_context(ThreadFn fn) {
  return ::CreateFiberEx(4096 * 2, stack_size, FIBER_FLAG_FLOAT_SWITCH, fn, nullptr);
}

void switch_context(void* context) {
  ++ctx_switches;
  ::SwitchToFiber(context);
}

void init() {
  memset(&cores, 0, sizeof(cores));
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
    BOOL rv = ::GetQueuedCompletionStatus(cport, &bytes, &key, &ov, 200);
    if (!rv) {
      wprintf(L"core switches %llu\r", ctx_switches);
    }
    else {
      wprintf(L"got event. exiting\n");
      break;
    }
  }

  for (int ix = 0; ix != num_cores; ++ix) {
    core_stop(ix);
  }
}

}