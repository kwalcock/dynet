#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

// See https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019 .

#ifdef MSVC
#  define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>

#ifdef MSVC
#  include <crtdbg.h>
#endif

class MemDebug {
public:
  MemDebug();
  ~MemDebug();
};

#if defined(_DEBUG) && defined(MSVC)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#  define DBG_NEW new
#endif

#endif
