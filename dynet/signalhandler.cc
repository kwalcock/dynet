#include "signalhandler.h"

#include <csignal>
#include <iostream>

namespace dynet {

SignalHandler::SignalHandler() { }

SignalHandler::~SignalHandler() { }

int SignalHandler::run(int signal) {
  std::cout << "Callback::run()" << std::endl;
  return 0;
}

void runSignalHandler(int signal);

void SignalHandlerHolder::set(int signal, SignalHandler* signalHandler) {
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
    delete keyValue.second;
  }
  signalHandlers.clear();
}

void SignalHandlerHolder::del(int signal) {
  std::signal(signal, SIG_DFL);
  std::map<int, SignalHandler*>::iterator signalHandlerItr = signalHandlers.find(signal);
  if (signalHandlerItr != signalHandlers.end()) {
    signalHandlers.erase(signalHandlerItr);
    delete signalHandlerItr->second;
  }
}

SignalHandlerHolder signalHandlerHolder;

void setSignalHandler(int signal, SignalHandler* signalHandler) {
  signalHandlerHolder.set(signal, signalHandler);
}

void resetSignalHandler(int signal) {
  signalHandlerHolder.reset(signal);
}

static void runSignalHandler(int signal) {
  signalHandlerHolder.run(signal);
}

}
