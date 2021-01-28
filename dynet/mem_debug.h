#ifndef DYNET_MEM_DEBUG_H
#define DYNET_MEM_DEBUG_H

#if !defined(WIN32)
#  ifndef NDEBUG
#    define _DEBUG 1
#  endif
#endif

// See https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019 .
#if defined(WIN32)
#  if defined(_DEBUG)
#    define _CRTDBG_MAP_ALLOC
#  endif
#  include <stdlib.h>
#  if defined(_DEBUG)
#    include <crtdbg.h>
#  endif
#  include <malloc.h>
#else
#  include <stdlib.h>
#  include <mm_malloc.h>
#endif

#if defined(WIN32) || defined(APPLE)
void mtrace();
#endif

namespace dynet {

inline void* dynet_malloc(size_t size) {
  return malloc(size);
}

template<typename T>
inline void dynet_free(T** ptr) {
  if (*ptr) {
    free(*ptr);
    *ptr = nullptr;
  }
}

inline void* dynet_mm_malloc(size_t size, size_t align) {
  return _mm_malloc(size, align);
}

template<typename T>
inline void dynet_mm_free(T** ptr) {
  if (*ptr) {
    _mm_free(*ptr);
    *ptr = nullptr;
  }
}

inline void* dbg_dynet_malloc(size_t size) {
  return malloc(size);
}

template<typename T>
inline void dbg_dynet_free(T** ptr) {
  if (*ptr) {
    free(*ptr);
    *ptr = nullptr;
  }
}

inline void* dbg_dynet_mm_malloc(size_t n, size_t align) {
  return _mm_malloc(n, align);
}

template<typename T>
inline void dbg_dynet_mm_free(T** ptr) {
  if (*ptr) {
    _mm_free(*ptr);
    *ptr = nullptr;
  }
}

void dbg_mem(const char* file, int line);
int dbg_client_block(const char* file, int line);

int mtrace();
int muntrace();

class MemDebug {
 public:
  // Add interactive to be able to debug memory?
  MemDebug();
  ~MemDebug();

  void debug();
  void leak_malloc();
  void leak_new();
  void leak_mm_malloc();
  void set_break(long index);
};

class Trace {
 public:
  Trace();
  ~Trace();
};

// https://stackoverflow.com/questions/704466/why-doesnt-delete-set-the-pointer-to-null
template<typename T>
inline void dynet_delete(T*& ptr) {
  if (ptr) {
    delete ptr;
    ptr = nullptr;
  }
}

// C-style memory management
#if defined(_DEBUG)
#  define DYNET_MALLOC(x) dbg_dynet_malloc(x)
#  define DYNET_FREE(x) dbg_dynet_free(x)
#  define DYNET_MM_MALLOC(n, align) dbg_dynet_mm_malloc(n, align)
#  define DYNET_MM_FREE(x) dbg_dynet_mm_free(x)
#else
#  define DYNET_MALLOC(x) dynet_malloc(x)
#  define DYNET_FREE(x) dynet_free(x)
#  define DYNET_MM_MALLOC(n, align) dynet_mm_malloc(n, align)
#  define DYNET_MM_FREE(x) dynet_mm_free(x)
#endif

// C++-style memory management
#if defined(_DEBUG) && defined(WIN32)
   // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
   // allocations to be of _CLIENT_BLOCK type.
#  define CLIENT_BLOCK(file, line) dbg_client_block(file, line)
#  define DYNET_NEW(x) new (CLIENT_BLOCK(__FILE__, __LINE__), __FILE__, __LINE__) x
#  define DYNET_NEW_ARR(x) DYNET_NEW(x)
#  define DYNET_DEL(x) dynet_delete(x)
#  define DYNET_DEL_ARR(x) delete[] x
#else
#  define DYNET_NEW(x) new x
#  define DYNET_NEW_ARR(x) DYNET_NEW(x)
#  define DYNET_DEL(x) dynet_delete(x)
#  define DYNET_DEL_ARR(x) delete[] x
#endif

} // namespace dynet

#endif
