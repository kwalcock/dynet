#include "signalhandler.h"

#include <csignal>
#include <iostream>
#include <map>
#include <memory>

namespace dynet {

  class SignalHandler {
   public:
    SignalHandler();
    virtual ~SignalHandler();
    virtual int run(int signal);
  };

  SignalHandler::SignalHandler() { }

  SignalHandler::~SignalHandler() { }

  int SignalHandler::run(int signal) {
    std::cout << "Callback::run()" << std::endl;
    return 0;
  }

  void runSignalHandler(int signal);

  class SignalHandlerHolder {
   protected:
    std::map<int, SignalHandler*> signalHandlers;
   public:
    void set(int signal, SignalHandler* signalHandler) {
      del(signal);
      signalHandlers[signal] = signalHandler;
      std::signal(signal, runSignalHandler);
    }

    void run(int signal) {
      signalHandlers[signal]->run(signal);
    }

    void reset(int signal) {
      del(signal);
    }

    ~SignalHandlerHolder() {
      for (const auto& keyValue : signalHandlers) {
        std::signal(keyValue.first, SIG_DFL);
        delete keyValue.second;
      }
      signalHandlers.clear();
    }
   protected:
    void del(int signal) {
      std::signal(signal, SIG_DFL);
      std::map<int, SignalHandler*>::iterator signalHandlerItr = signalHandlers.find(signal);
      if (signalHandlerItr != signalHandlers.end()) {
        signalHandlers.erase(signalHandlerItr);
        delete signalHandlerItr->second;
      }
    }
  };

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
