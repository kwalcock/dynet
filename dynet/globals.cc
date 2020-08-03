#include "dynet/globals.h"
#include "dynet/devices.h"
#include "dynet/timing.h"

#ifdef HAVE_CUDA
#include "dynet/cuda.h"
#endif

namespace dynet {

std::mt19937* rndeng = nullptr;
Device* default_device = nullptr;
float default_weight_decay_lambda;
int autobatch_flag = 0;
int profiling_flag = 0;
int forward_only_flag = 0;
NamedTimer timer;

}
