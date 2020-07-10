#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

// See https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019 .

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

void debugMem(char* file, int line);

#ifdef _DEBUG

class MemDebug {
public:
  MemDebug();
  ~MemDebug();
};

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type.
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )

#else

#define DBG_NEW new

#endif

#endif
