#include "dynet/mem_debug.h"
#include <cstring>
#include <iostream>
#include <memory>

#if !_WINDOWS
#include <mcheck.h>
#include <mm_malloc.h>
#endif

namespace dynet {

void dbg_mem(const char* file, int line) {
#ifdef _MSC_VER
  int leaksFound = _CrtDumpMemoryLeaks();
  if (leaksFound)
    std::cerr << "Memory leaks were found when checked from " << file << " at line " << line << "." << std::endl;
  else
    std::cerr << "No memory leaks were found." << std::endl;
#endif
}

int dbg_client_block(const char* file, int line) {
#if defined(_DEBUG) && defined(_MSC_VER)
  return _CLIENT_BLOCK;
#else
  return 0;
#endif
}

int callSetBreak(int index) {
#if defined(_DEBUG) && defined(_MSC_VER)
  return _CrtSetBreakAlloc(index);
#else
  return index;
#endif
}

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
#else
#  if !defined(_MSC_VER)
  // Use this even in production version for Linux.
  mtrace();
#  endif
#endif
}

MemDebug::~MemDebug() {
}

void MemDebug::debug() {
  dbg_mem(__FILE__, __LINE__);
}

// These guarantee a memory leak which when displayed at program termination
// verifies that leak detection is active.

void MemDebug::leak_malloc() {
  char* leak = (char*) DYNET_MALLOC(15);
  strcpy(leak, "No leaks here!");
}

void MemDebug::leak_new() {
  std::string* leak = DYNET_NEW(std::string("No leaks there!"));
}

void MemDebug::leak_mm_malloc() {
  char* leak = (char*) DYNET_MM_MALLOC(19, 4);
  strcpy(leak, "No leaks anywhere!");
}

void MemDebug::set_break(long index) {
  callSetBreak(index);
}

#if defined(_DEBUG) && defined(_MSC_VER)
MemDebug localMemDebug(true);
int breakIndex = 0; // callSetBreak(303);
#endif

} // namespace dynet
