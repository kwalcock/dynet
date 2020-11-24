#ifndef DYNET_CALLBACK_H
#define DYNET_CALLBACK_H

#include <cstdio>
#include <iostream>

namespace dynet {

class Callback {
 public:
  Callback();
  virtual ~Callback();
  virtual void run();
};


class Caller {
 private:
  int _callback;
 public:
  Caller();
  ~Caller();
  void delCallback();
  void setCallback(int cb);
  void call();
};

} // namespace dynet

#endif
