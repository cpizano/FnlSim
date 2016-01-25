#include "stdafx.h"
#include "threadjack_x64.h"

namespace {
HANDLE ready_event = nullptr;

// StackSpace returns 172 bytes.
DWORD64 StackSpace() {
  const int integer_sz = FIELD_OFFSET(CONTEXT, R15) - FIELD_OFFSET(CONTEXT, Rax);
  const int home_sz = sizeof(CONTEXT::R8) + sizeof(CONTEXT::Rcx) +
                      sizeof(CONTEXT::R9) + sizeof(CONTEXT::Rdx);
  const int control_sz = sizeof(CONTEXT::Rip) + sizeof(CONTEXT::EFlags);
  return integer_sz + control_sz + home_sz + sizeof(CONTEXT::Rip);
}

// Implementation is in restore.asm.
// We use ml64.exe /Fo restore.obj /c restore.asm to create the obj.
extern "C" void RestoreX64(PDWORD64 Rsp);

__declspec(noreturn) 
void __stdcall Trampolinex64(CONTEXT* old_context,      // rcx
                             HANDLE ready_event,        // rdx
                             PFIBER_START_ROUTINE fn,   // R8
                             void* arg) {               // R9
  CONTEXT oc = *old_context;
  if (!::SetEvent(ready_event))
    __debugbreak();

  DWORD64* Rsp = (DWORD64*)oc.Rsp;
  *(--Rsp) = oc.Rip;        // ret
  *(--Rsp) = oc.EFlags;     // popfq
  *(--Rsp) = oc.Rax;        // pop rax
  *(--Rsp) = oc.Rcx;        // pop rbx    ??
  *(--Rsp) = oc.Rdx;        // pop rcx    ??
  *(--Rsp) = oc.Rbx;        // pop rdx    ??
  *(--Rsp) = oc.Rbp;        // pop rbp
  *(--Rsp) = oc.Rsi;        // pop rsi
  *(--Rsp) = oc.Rdi;        // pop rdi
  *(--Rsp) = oc.R8;         // pop r8
  *(--Rsp) = oc.R9;         // pop r9
  *(--Rsp) = oc.R10;        // pop r10
  *(--Rsp) = oc.R11;        // pop r11
  *(--Rsp) = oc.R12;        // pop r12
  *(--Rsp) = oc.R13;        // pop r13
  *(--Rsp) = oc.R14;        // pop r14
  *(--Rsp) = oc.R15;        // pop r15

  fn(arg);
  // Restore the integer and contol registers.
  RestoreX64(Rsp);
}

__forceinline bool WINAPI IsThreadInUserMode(LONG ctx_flags) {
  const LONG mask = CONTEXT_EXCEPTION_REPORTING |
                    CONTEXT_EXCEPTION_ACTIVE |
                    CONTEXT_SERVICE_ACTIVE;
  return (mask & ctx_flags) == CONTEXT_EXCEPTION_REPORTING;
}

__forceinline bool UM_GetThreadContext(HANDLE thread, CONTEXT* old_context) {
  memset(old_context, 0, sizeof(*old_context));
  // Try to get the thread's context and ensure it is running in user-mode.
  old_context->ContextFlags = CONTEXT_CONTROL |
                              CONTEXT_INTEGER |
                              CONTEXT_EXCEPTION_REQUEST;
  return GetThreadContext(thread, old_context) &&
         IsThreadInUserMode(old_context->ContextFlags);
}

__forceinline bool SetNewContext(HANDLE thread,
                                 CONTEXT* old_context,
                                 HANDLE ready_event,
                                 PFIBER_START_ROUTINE fn,
                                 void* arg) {
  CONTEXT new_context = *old_context;
  new_context.Rip = (DWORD64)Trampolinex64;
  // Reserve space on the stack for the integer and control registers + 
  // home space for Trampoline (R9, R8, Rdx and Rcx) + return address.
  const DWORD64 reserve = StackSpace();
  new_context.Rsp -= reserve;
  // Arguments to trampoline.
  new_context.Rcx = (DWORD64)old_context;
  new_context.Rdx = (DWORD64)ready_event;
  new_context.R8 = (DWORD64)fn;
  new_context.R9 = (DWORD64)arg;
  // Jump to trampoline.
  return SetThreadContext(thread, &new_context) == TRUE;
}

}

bool threadjack::init() {
  // auto-reset event.
  ready_event = ::CreateEventW(NULL, FALSE, FALSE, NULL);
  return true;
}

bool threadjack::interrupt(HANDLE thread, PFIBER_START_ROUTINE fn, void * arg) {
  long sc = SuspendThread(thread);
  if (sc < 0)
    return false;
  // Try to set the new context and wait until the thread no longer
  // needs the old one.
  CONTEXT old_ctx;
  if (!UM_GetThreadContext(thread, &old_ctx))
    goto error;
  if (!SetNewContext(thread, &old_ctx, ready_event, fn, arg))
    goto error;
  ::ResumeThread(thread);
  return ::WaitForSingleObject(ready_event, INFINITE) == WAIT_OBJECT_0;

error:
  ::ResumeThread(thread);
  return false;
}
