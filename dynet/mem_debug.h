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

void* dbg_malloc(size_t size);
void* dbg_mm_malloc(size_t size, size_t align);
void dbg_free(void* ptr);

void dbg_mem(const char* file, int line);
int dbg_client_block(const char* file, int line);

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
#  define MM_MALLOC(n, align) dbg_mm_malloc(n, align)
#  define FREE(x) dbg_free(x)
#else
#  define MALLOC(x) malloc(x)
#  define MM_MALLOC(n, align) _mm_malloc(n, align)
#  define FREE(x) free(x)
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define CLIENT_BLOCK(file, line) dbg_client_block(file, line)
#  define NEW new (CLIENT_BLOCK(__FILE__, __LINE__), __FILE__, __LINE__)
#else
#  define NEW new
#endif

} // namespace dynet

#endif
