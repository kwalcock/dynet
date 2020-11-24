#ifndef DYNET_SIGNALHANDLER_H
#define DYNET_SIGNALHANDLER_H

#include <map>

namespace dynet {

class SignalHandler {
 public:
  SignalHandler();
  virtual ~SignalHandler();
  virtual int run(int signal);
};

class SignalHandlerHolder {
 protected:
  std::map<int, SignalHandler*> signalHandlers;
 public:
  void set(int signal, SignalHandler* signalHandler);
  void run(int signal);
  void reset(int signal);
  ~SignalHandlerHolder();
 protected:
  void del(int signal);
};

void setSignalHandler(int signal, SignalHandler* signalHandler);
void resetSignalHandler(int signal);

} // namespace dynet

#endif
