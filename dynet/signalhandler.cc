#include "signalhandler.h"

#include <csignal>
#include <iostream>

#include <eh.h>
#include <windows.h>

namespace dynet {

SignalHandler::SignalHandler() { }

SignalHandler::~SignalHandler() { std::cout << "SignalHandler::~SignalHandler()" << std::endl; }

int SignalHandler::run(int signal) {
  std::cout << "SignalHandler::run()" << std::endl;
  return 0;
}

dynet::SignalHandlerHolder signalHandlerHolder;

static void throwSignalException(int signal) {
  // make special kind of exception for this signal_exception
  throw std::runtime_error("Signal handler was activated."); // signal_error
}

extern "C" {
  static void runSignalHandler(int signal) {
    signalHandlerHolder.run(signal);
    // The run may not result in the same signal.  The handler is reapplied afterwards.
    // This is to avoid recursive calls that might quickly get out of hand.
    std::signal(signal, runSignalHandler);
    throwSignalException(signal);
  }
}

void SignalHandlerHolder::set(int signal, dynet::SignalHandler* signalHandler) {
  del(signal);
  signalHandlers[signal] = signalHandler;
  std::signal(signal, runSignalHandler);
}

void SignalHandlerHolder::run(int signal) {
  signalHandlers[signal]->run(signal);
}

void SignalHandlerHolder::reset(int signal) {
  del(signal);
}

SignalHandlerHolder::~SignalHandlerHolder() {
  for (const auto& keyValue : signalHandlers) {
    std::signal(keyValue.first, SIG_DFL);
    // delete keyValue.second;
  }
  signalHandlers.clear();
}

void SignalHandlerHolder::del(int signal) {
  std::map<int, SignalHandler*>::iterator signalHandlerItr = signalHandlers.find(signal);
  if (signalHandlerItr != signalHandlers.end()) {
    std::signal(signal, SIG_DFL);
    signalHandlers.erase(signalHandlerItr);
    // delete signalHandlerItr->second;
  }
}

void setSignalHandler(int signal, dynet::SignalHandler* signalHandler) {
  signalHandlerHolder.set(signal, signalHandler);
}

void resetSignalHandler(int signal) {
  signalHandlerHolder.reset(signal);
}




/////////////// WINDOWS

class SE_Exception : public std::exception
{
private:
  const unsigned int nSE;
public:
  SE_Exception() noexcept : SE_Exception{ 0 } {}
  SE_Exception(unsigned int n) noexcept : nSE{ n } {}
  unsigned int getSeNumber() const noexcept { return nSE; }
};

class Scoped_SE_Translator
{
private:
  const _se_translator_function old_SE_translator;
public:
  Scoped_SE_Translator(_se_translator_function new_SE_translator) noexcept
    : old_SE_translator{ _set_se_translator(new_SE_translator) } {}
  ~Scoped_SE_Translator() noexcept { _set_se_translator(old_SE_translator); }
};

void trans_func(unsigned int u, EXCEPTION_POINTERS* nothing)
{
  // Convert
  unsigned windowsU = u == EXCEPTION_ACCESS_VIOLATION ? SIGSEGV : u;
  signalHandlerHolder.run(windowsU); // Only if it exists.
  throwSignalException(windowsU);
//  throw SE_Exception(windowsU);
}

Scoped_SE_Translator scoped_se_translator{ trans_func };
}
