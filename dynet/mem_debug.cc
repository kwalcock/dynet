#include "dynet/mem_debug.h"
#include <iostream>

#ifdef _DEBUG

void debugMem(const char* file, int line) {
#ifdef _MSC_VER
  int leaksFound = _CrtDumpMemoryLeaks();
  if (leaksFound)
    std::cerr << "Memory leaks were found when checked from " << file << " at line " << line << "." << std::endl;
  else
    std::cerr << "No memory leaks were found." << std::endl;
#endif
}

#else

void debugMem(const char* file, int line) {
  // no-op
}

#endif

MemDebug::MemDebug(bool atExit) {
#if defined(_DEBUG) && defined(_MSC_VER)
  // _CRTDBG_ALLOC_MEM_DF = Turn on debug allocation
  // _CRTDBG_LEAK_CHECK_DF = Leak check at program exit
  int flags = _CRTDBG_ALLOC_MEM_DF;
  if (atExit)
    flags |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(flags);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif
}

void MemDebug::debug() {
#if defined(_DEBUG) && defined(_MSC_VER)
  debugMem(__FILE__, __LINE__);
#endif
}

void MemDebug::leak() {
#if defined(_DEBUG) && defined(_MSC_VER)
  // This guarantees a memory leak which when displayed at program termination
  // verifies that leak detection is active.
  char* leak = (char*)malloc(20);
  strcpy(leak, "Hello, memory leak!");
#endif
}

MemDebug::~MemDebug() {
}
