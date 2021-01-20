#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

// See https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019 .

#ifdef _MSC_VER
#  define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>

#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

namespace dynet {

void debugMem(const char* file, int line);

void* dbg_malloc(size_t size);
void* dbg_mm_malloc(size_t size, size_t align);

class MemDebug {
 public:
  // Add interactive to be able to debug memory?
  MemDebug(bool atExit = true);
  ~MemDebug();

  void debug();
  void leak_malloc();
  void leak_new();
  void leak_mm_malloc();
  void set_break(long index);
};

#if defined(_DEBUG)
#  define MALLOC(x) dbg_malloc(x)
#  if defined(_MSC_VER)
int dbg_client_block(const char* file, int line);
#  endif
#else
#  define MALLOC(x) malloc(x)
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define DBG_CLIENT_BLOCK(file, line) dbg_client_block(file, line)
#  define DBG_NEW new (DBG_CLIENT_BLOCK(__FILE__, __LINE__), __FILE__, __LINE__)
#else
#  define DBG_NEW new
#endif

#ifdef _DEBUG
#  define DBG_MM_MALLOC(n, align) dbg_mm_malloc(n, align)
#else
#  define DBG_MM_MALLOC(n, align) _mm_malloc(n, align)
#endif
} // namespace dynet

#endif
