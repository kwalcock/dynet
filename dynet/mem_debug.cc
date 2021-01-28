#include "dynet/mem_debug.h"
#include <cstring>
#include <iostream>
#include <memory>

#if defined(WIN32)
#  include <cstdlib>
#else
#  if !defined(APPLE)
#    include <mcheck.h>
#  endif
#  include <mm_malloc.h>
#endif

#if defined(WIN32) || defined(APPLE)
void mtrace() {
#  if defined(_DEBUG)
  bool trace = std::getenv("MALLOC_TRACE") != nullptr;

  if (trace) {
    bool atExit = true;
    // _CRTDBG_ALLOC_MEM_DF = Turn on debug allocation
    // _CRTDBG_LEAK_CHECK_DF = Leak check at program exit
    int flags = _CRTDBG_ALLOC_MEM_DF;
    if (atExit)
      flags |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(flags);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG /*| _CRTDBG_MODE_WNDW*/);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
  }
#  endif
}

void muntrace() {
}
#endif

namespace dynet {

void dbg_mem(const char* file, int line) {
#if defined(WIN32)
  int leaksFound = _CrtDumpMemoryLeaks();
  if (leaksFound)
    std::cerr << "Memory leaks were found when checked from " << file << " at line " << line << "." << std::endl;
  else
    std::cerr << "No memory leaks were found." << std::endl;
#endif
}

int dbg_client_block(const char* file, int line) {
#if defined(_DEBUG) && defined(WIN32)
  return _CLIENT_BLOCK;
#else
  return 0;
#endif
}

int callSetBreak(int index) {
#if defined(_DEBUG) && defined(WIN32)
  return _CrtSetBreakAlloc(index);
#else
  return index;
#endif
}

int mtrace() {
  ::mtrace();
  return 0;
}

int muntrace() {
  ::muntrace();
  return 0;
}

MemDebug::MemDebug() {
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

Trace::Trace() {
  mtrace();
}

Trace::~Trace() {
  muntrace();
}

#if defined(_DEBUG) || !defined(WIN32)
// We always try to trace in debug mode.  If not Windows, we even try to trace no matter what,
// because the overhead is low if MALLOC_TRACE is not set.  It is important that tracing
// starts now rather than later because Java will start creating C++ objects, mostly via
// static object constructors, before we are able to call mtrace from there.
int traceIndex = mtrace();
// If there is a known, early bad allocation, the breakpoint may need to be set here.
int breakIndex = 0; // callSetBreak(303); 
#endif

} // namespace dynet
