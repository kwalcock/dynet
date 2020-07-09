#include "dynet/debug.h"
#include <iostream>

#ifdef _DEBUG

MemoryTest::MemoryTest() {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
}

MemoryTest::~MemoryTest() {
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
}

MemoryTest memoryTest;

#endif
