#ifndef DYNET_CALLBACK_H
#define DYNET_CALLBACK_H

namespace dynet {

class Callback {
 public:
  Callback();
  virtual ~Callback();
  virtual void run();
};


class Caller {
 private:
  Callback* _callback;
 public:
  Caller();
  ~Caller();
  void delCallback();
  void setCallback(Callback* cb);
  void call();
};

} // namespace dynet

#endif
