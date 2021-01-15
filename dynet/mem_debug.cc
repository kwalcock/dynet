#include "dynet/mem_debug.h"
#include <iostream>

namespace dynet {

void debugMem(const char* file, int line) {
#ifdef _MSC_VER
  int leaksFound = _CrtDumpMemoryLeaks();
  if (leaksFound)
    std::cerr << "Memory leaks were found when checked from " << file << " at line " << line << "." << std::endl;
  else
    std::cerr << "No memory leaks were found." << std::endl;
#endif
}

void* dbg_malloc(size_t size) {
  if (size == 1) {
    std::cerr << "Malloc with size of 1!" << std::endl;
  }
  return malloc(size);
}

#if defined(_DEBUG) && defined(_MSC_VER)
int get_client_block(const char* file, int line) {
  return _CLIENT_BLOCK;
}
#endif

MemDebug localMemDebug(true);

int callSetBreak(int index) {
    localMemDebug.set_break(index);
    return index;
}

#if defined(_DEBUG) && defined(_MSC_VER)
int breakIndex = callSetBreak(5);
#endif

MemDebug::MemDebug(bool atExit) {
#if defined(_DEBUG) && defined(_MSC_VER)
  // _CRTDBG_ALLOC_MEM_DF = Turn on debug allocation
  // _CRTDBG_LEAK_CHECK_DF = Leak check at program exit
  int flags = _CRTDBG_ALLOC_MEM_DF;
  if (atExit)
    flags |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(flags);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG /*| _CRTDBG_MODE_WNDW*/);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif
}

MemDebug::~MemDebug() {
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
  char* leak = (char*)MALLOC(20);
  strcpy(leak, "Hello, memory leak!");
#endif
}

void MemDebug::set_break(long index) {
#if defined(_DEBUG) && defined(_MSC_VER)
  _CrtSetBreakAlloc(index);
#endif
}

} // namespace dynet
