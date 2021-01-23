#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

// See https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019 .
#ifdef _MSC_VER
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#else
#  include <stdlib.h>
#endif

namespace dynet {

void* dbg_malloc(size_t size);
void dbg_free(void* ptr);
void* dbg_mm_malloc(size_t size, size_t align);
void dbg_mm_free(void* ptr);

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

// https://stackoverflow.com/questions/704466/why-doesnt-delete-set-the-pointer-to-null
template<typename T>
void delete_zero(T*& ptr) {
  if (ptr) {
    delete ptr;
    ptr = nullptr;
  }
}

template<typename T>
void reset_ptr(T*& ptr) {
  if (ptr)
    ptr = nullptr;
}

// C-style memory management
#if defined(_DEBUG)
#  define DYNET_MALLOC(x) dbg_malloc(x)
#  define DYNET_FREE(x) dbg_free(x)
#  define DYNET_MM_MALLOC(n, align) dbg_mm_malloc(n, align)
#  define DYNET_MM_FREE(x) dbg_mm_free(x)
#else
#  define DYNET_MALLOC(x) malloc(x)
#  define DYNET_FREE(x) free(x)
#  define DYNET_MM_MALLOC(n, align) _mm_malloc(n, align)
#  define DYNET_MM_FREE(x) _mm_free(x)
#endif

// C++-style memory management
#if defined(_DEBUG) && defined(_MSC_VER)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define CLIENT_BLOCK(file, line) dbg_client_block(file, line)
#  define DYNET_NEW(x) new (CLIENT_BLOCK(__FILE__, __LINE__), __FILE__, __LINE__) x
#  define DYNET_NEW_ARR(x) DYNET_NEW(x)
#  define DYNET_DEL(x) delete_zero(x)
#  define DYNET_DEL_ARR(x) delete[] x
#else
#  define DYNET_NEW(x) new x
#  define DYNET_NEW_ARR(x) DYNET_NEW(x)
#  define DYNET_DEL(x) delete_zero(x)
#  define DYNET_DEL_ARR(x) delete[] x
#endif

} // namespace dynet

#endif
