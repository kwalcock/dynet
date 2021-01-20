#include "dynet/mem_debug.h"
#include <iostream>
#include <memory>

#if !_WINDOWS
#include <mcheck.h>
#include <mm_malloc.h>
#endif

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
  return malloc(size);
}

#if defined(_DEBUG) && defined(_MSC_VER)
int dbg_client_block(const char* file, int line) {
  return _CLIENT_BLOCK;
}
#endif

void* dbg_mm_malloc(size_t n, size_t align) {
  return _mm_malloc(n, align);
}

MemDebug localMemDebug(true);

int callSetBreak(int index) {
    localMemDebug.set_break(index);
    return index;
}

#if defined(_DEBUG) && defined(_MSC_VER)
int breakIndex = 0; // callSetBreak(5);
#endif

MemDebug::MemDebug(bool atExit) {
#if defined(_DEBUG)
#  if defined(_MSC_VER)
  // _CRTDBG_ALLOC_MEM_DF = Turn on debug allocation
  // _CRTDBG_LEAK_CHECK_DF = Leak check at program exit
  int flags = _CRTDBG_ALLOC_MEM_DF;
  if (atExit)
    flags |= _CRTDBG_LEAK_CHECK_DF;
  _CrtSetDbgFlag(flags);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG /*| _CRTDBG_MODE_WNDW*/);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#  else
  mtrace();
#  endif
#endif
}

MemDebug::~MemDebug() {
}

void MemDebug::debug() {
#if defined(_DEBUG) && defined(_MSC_VER)
  debugMem(__FILE__, __LINE__);
#endif
}

// These guarantee a memory leak which when displayed at program termination
// verifies that leak detection is active.

void MemDebug::leak_malloc() {
#if defined(_DEBUG) && defined(_MSC_VER)
  // This leak is via malloc.
  char* leak = (char*)malloc(15);
  strcpy(leak, "No leaks here!");
#endif
}

void MemDebug::leak_new() {
#if defined(_DEBUG) && defined(_MSC_VER)
  // This is via new.
  std::string* leak = DBG_NEW std::string("No leaks there!");
#endif
}

void MemDebug::leak_mm_malloc() {
#if defined(_DEBUG) && defined(_MSC_VER)
  // This is via mm_malloc.
  char* leak = (char*)_mm_malloc(15, 4);
  strcpy(leak, "Or anywhere!");
#endif
}

void MemDebug::set_break(long index) {
#if defined(_DEBUG) && defined(_MSC_VER)
  _CrtSetBreakAlloc(index);
#endif
}

} // namespace dynet
