// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>

#include <windows.h>

#include <stdio.h>
#include <tchar.h>
#include <stdint.h>

#define KPANIC  __debugbreak()
#define CHECK(x, y)  if ((x) != (y)) KPANIC
#define CHECKNE(x, y)  if ((x) == (y)) KPANIC


// TODO: reference additional headers your program requires here
