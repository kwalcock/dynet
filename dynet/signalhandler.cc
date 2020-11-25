#include "signalhandler.h"

#include <csignal>
#include <iostream>

namespace dynet {

SignalHandler::SignalHandler() { }

SignalHandler::~SignalHandler() { std::cout << "SignalHandler::~SignalHandler()" << std::endl; }

int SignalHandler::run(int signal) {
  std::cout << "SignalHandler::run()" << std::endl;
  return 0;
}

dynet::SignalHandlerHolder signalHandlerHolder;

static void throwSignalException(int signal) {
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

}
