#pragma once

namespace sim {
const int num_cores = 4;
const int stack_size = 128 * 1024;

// StartFn returns the interrupt context.
using StartFn = void* (*)();

using ThreadFn = void (__stdcall *)(void* p);

void init();
void run();

// processor GS register access;
void* kern_gs(void* new_gs);
void* user_gs(void* new_gs);

void core_start(int core_id, StartFn fn);

// processor-relative cpu context management.
void* make_context(ThreadFn fn);
void switch_context(void* ctx);

}