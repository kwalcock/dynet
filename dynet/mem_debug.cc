#include "dynet/mem_debug.h"
#include <iostream>

#ifdef _DEBUG

void debugMem(char* file, int line) {
  std::cerr << "Memory leaks in " << file << " at line " << line << std::endl;
  _CrtDumpMemoryLeaks();
}

MemDebug::MemDebug() {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//  debugMem(__FILE__, __LINE__);
}

MemDebug::~MemDebug() {
//  debugMem(__FILE__, __LINE__);
}

MemDebug memDebug;

#else

void debugMem(char* file, int line) {
  // no-op
}

#endif
