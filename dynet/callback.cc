#include "callback.h"

#include <iostream>

namespace dynet {

  Callback::Callback() { }

  Callback::~Callback() { std::cout << "Callback::~Callback()" << std::endl; }

  void Callback::run() { std::cout << "Callback::run()" << std::endl; }

  Caller::Caller() : _callback(nullptr) {}

  Caller::~Caller() { delCallback(); }

  void Caller::delCallback() { delete _callback; _callback = nullptr; }

  void Caller::setCallback(Callback* cb) { delCallback(); _callback = cb; }

  void Caller::call() { if (_callback) _callback->run(); }

}
