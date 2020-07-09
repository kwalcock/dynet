#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

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

#endif

#endif
