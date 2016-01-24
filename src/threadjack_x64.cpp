#include "stdafx.h"
#include "threadjack_x64.h"

namespace {

#define INTEGER_REG_SIZE (FIELD_OFFSET(CONTEXT, R15) - FIELD_OFFSET(CONTEXT, Rax))

#define HOME_SPACE_SIZE (sizeof(((PCONTEXT)0)->R8) + sizeof(((PCONTEXT)0)->Rcx) + \
                         sizeof(((PCONTEXT)0)->R9) + sizeof(((PCONTEXT)0)->Rdx))

#define CONTROL_REG_SIZE (sizeof(((PCONTEXT)0)->Rip) + sizeof(((PCONTEXT)0)->EFlags))

// Implementation is in restore.asm.
extern "C" void RestoreX64(PDWORD64 Rsp);

__declspec(noreturn) 
void __stdcall Trampolinex64(
  PCONTEXT PointerToOldContext,
  HANDLE Event,
  PFIBER_START_ROUTINE fn,
  PVOID arg) {

  CONTEXT OldContext;
  PDWORD64 Rsp;

  OldContext = *PointerToOldContext;
  SetEvent(Event);

  Rsp = (PDWORD64)OldContext.Rsp;
  *(--Rsp) = OldContext.Rip;
  *(--Rsp) = OldContext.EFlags;
  *(--Rsp) = OldContext.Rax;
  *(--Rsp) = OldContext.Rcx;
  *(--Rsp) = OldContext.Rdx;
  *(--Rsp) = OldContext.Rbx;
  *(--Rsp) = OldContext.Rbp;
  *(--Rsp) = OldContext.Rsi;
  *(--Rsp) = OldContext.Rdi;
  *(--Rsp) = OldContext.R8;
  *(--Rsp) = OldContext.R9;
  *(--Rsp) = OldContext.R10;
  *(--Rsp) = OldContext.R11;
  *(--Rsp) = OldContext.R12;
  *(--Rsp) = OldContext.R13;
  *(--Rsp) = OldContext.R14;
  *(--Rsp) = OldContext.R15;

  fn(arg);

  //
  // Restore the integer and contol registers.
  //
  RestoreX64(Rsp);
}

FORCEINLINE BOOL WINAPI
IsThreadInUserMode(__in LONG ContextFlags) {
  const LONG Mask = CONTEXT_EXCEPTION_REPORTING
    | CONTEXT_EXCEPTION_ACTIVE
    | CONTEXT_SERVICE_ACTIVE;

  return (Mask & ContextFlags) == CONTEXT_EXCEPTION_REPORTING;
}

FORCEINLINE BOOL xGetThreadContext(HANDLE Thread,
  PCONTEXT OldContext) {
  RtlZeroMemory(OldContext, sizeof(*OldContext));
  // Try to get the thread's context and ensure it is running in user-mode.
  OldContext->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_EXCEPTION_REQUEST;
  return GetThreadContext(Thread, OldContext) && IsThreadInUserMode(OldContext->ContextFlags);
}

FORCEINLINE BOOL SetNewContext(
    HANDLE Thread,
    PCONTEXT OldContext,
    HANDLE Event,
    PFIBER_START_ROUTINE Function,
    PVOID Argument) {
  CONTEXT NewContext = *OldContext;
  NewContext.Rip = (DWORD64)Trampolinex64;

  //
  // Reserve space on the stack for the integer and control registers + 
  // home space for Trampoline (R9, R8, Rdx and Rcx) + return address.
  //

  NewContext.Rsp -= INTEGER_REG_SIZE + CONTROL_REG_SIZE +
    HOME_SPACE_SIZE + sizeof(((PCONTEXT)0)->Rip);

  // Arguments.
  NewContext.Rcx = (DWORD64)OldContext;
  NewContext.Rdx = (DWORD64)Event;
  NewContext.R8 = (DWORD64)Function;
  NewContext.R9 = (DWORD64)Argument;

  return SetThreadContext(Thread, &NewContext);
}

}

bool threadjack::interrupt(HANDLE thread, PFIBER_START_ROUTINE fn, void * arg) {
  HANDLE Event;
  CONTEXT OldContext;

  long sc = SuspendThread(thread);
  if (sc != 0)
    return false;

  Event = NULL;

  //
  // Try to set the new context and wait until the thread no longer
  // needs the old one.
  //

  bool Success = xGetThreadContext(thread, &OldContext)
    && (Event = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL
    && SetNewContext(thread, &OldContext, Event, fn, arg);

  ResumeThread(thread);

  Success = Success && WaitForSingleObject(Event, INFINITE) == WAIT_OBJECT_0;

  if (Event != NULL) {
    CloseHandle(Event);
  }

  return Success;

  return false;
}
