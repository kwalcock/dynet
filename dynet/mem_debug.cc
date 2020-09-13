#include "dynet/mem_debug.h"
#include <iostream>

#ifdef _DEBUG

void debugMem(const char* file, int line) {
#ifdef _MSC_VER
  std::cerr << "Memory leaks in " << file << " at line " << line << std::endl;
  _CrtDumpMemoryLeaks();
#endif
}

#else

void debugMem(const char* file, int line) {
  // no-op
}

#endif

MemDebug::MemDebug() {
#if defined(_DEBUG) && defined(_MSC_VER)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

MemDebug::~MemDebug() {
  //  debugMem(__FILE__, __LINE__);
}
