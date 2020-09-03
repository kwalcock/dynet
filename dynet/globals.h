#ifndef DYNET_GLOBALS_H
#define DYNET_GLOBALS_H

#include <mutex>
#include <random>

namespace dynet {

class Device;
class NamedTimer;

extern std::mt19937* rndeng;
extern std::mutex rndengMutex; // Protect the global rndeng.
extern Device* default_device;
extern NamedTimer timer; // debug timing in executors.

} // namespace dynet

#endif
