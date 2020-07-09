#ifndef DYNET_DEBUG_H
#define DYNET_DEBUG_H

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG

class MemoryTest {
public:
  MemoryTest();
  ~MemoryTest();
};

#endif

#endif
