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

class MemDebug {
 public:
  // Add interactive to be able to debug memory?
  MemDebug(bool atExit = true);
  ~MemDebug();

  void debug();
  void leak();
  void set_break(long index);
};

#if defined(_DEBUG)
#  define MALLOC(x) dbg_malloc(x)
#  if defined(_MSC_VER)
int get_client_block(const char* file, int line);
#  endif
#else
#  define MALLOC(x) malloc(x)
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define GET_CLIENT_BLOCK get_client_block(__FILE__, __LINE__)
#  define DBG_NEW new ( _CLIENT_BLOCK , __FILE__ , __LINE__ )
#else
#  define DBG_NEW new
#endif

} // namespace dynet

#endif
