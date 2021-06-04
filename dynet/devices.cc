#include "dynet/mem_debug.h"
#include "dynet/devices.h"

#include <iostream>
#include <mutex>
#include <string>
#include <unsupported/Eigen/CXX11/Tensor>

#include "dynet/cuda.h"
#include "dynet/dynet.h"
#include "dynet/expr.h"
#include "dynet/except.h"
#include "dynet/str-util.h"

using namespace std;

namespace dynet {

DeviceMempoolSizes::DeviceMempoolSizes(size_t total_size) {
  DYNET_ARG_CHECK(total_size > 0, "Attempt to allocate memory of size 0 in DeviceMempoolSizes");
  if (total_size < 4) {
    used[0] = used[1] = used[2] = used[3] = 1;
  } else {
    used[0] = total_size / 4;
    used[1] = total_size / 4;
    used[2] = total_size / 4;
    used[3] = total_size / 4;
  }
}

DeviceMempoolSizes::DeviceMempoolSizes(size_t fx_s, size_t dEdfs_s, size_t ps_s, size_t sc_s) {
  used[0] = fx_s;
  used[1] = dEdfs_s;
  used[2] = ps_s;
  used[3] = sc_s;
}

DeviceMempoolSizes::DeviceMempoolSizes(const std::string & descriptor) {
  vector<string> strs = str_split(descriptor, ',');
  if (strs.size() == 1) {
    size_t total_size = stoi(strs[0]);
    DYNET_ARG_CHECK(total_size > 0, "Attempt to allocate memory of size 0 in DeviceMempoolSizes");
    if (total_size < 4) {
      used[0] = used[1] = used[2] = used[3] = 1;
    } else {
      used[0] = total_size / 4;
      used[1] = total_size / 4;
      used[2] = total_size / 4;
      used[3] = total_size / 4;
    }
  } else if (strs.size() == 4) {
    used[0] = stoi(strs[0]);
    used[1] = stoi(strs[1]);
    used[2] = stoi(strs[2]);
    used[3] = stoi(strs[3]);
  } else {
    DYNET_INVALID_ARG("the format of --dynet-mem is invalid: " << descriptor);
  }
}

Device::~Device() {}

DeviceMempoolSizes Device::mark(ComputationGraph *cg) {
  cg->incremental_forward({cg, (VariableIndex)(cg->nodes.size() - 1)}); // needed so that we actually allocate the needed memory
  // for all existing nodes.
  return DeviceMempoolSizes(pools[0]->used(), pools[1]->used(), pools[2]->used(), pools[3]->used());
}

void Device::revert(const DeviceMempoolSizes & cp) {
  if(cp.used[0] > pools[0]->used())
    DYNET_INVALID_ARG("Saved value greater than original value in Device::revert (" << cp.used[0] << " > " << pools[0]->used() << ")");
  pools[0]->set_used(cp.used[0]);
  if(cp.used[1] > pools[1]->used())
    DYNET_INVALID_ARG("Saved value greater than original value in Device::revert (" << cp.used[1] << " > " << pools[1]->used() << ")");
  pools[1]->set_used(cp.used[1]);
  if(cp.used[2] > pools[2]->used())
    DYNET_INVALID_ARG("Saved value greater than original value in Device::revert (" << cp.used[2] << " > " << pools[2]->used() << ")");
  pools[2]->set_used(cp.used[2]);
  if(cp.used[3] > pools[3]->used())
    DYNET_INVALID_ARG("Saved value greater than original value in Device::revert (" << cp.used[3] << " > " << pools[3]->used() << ")");
  pools[3]->set_used(cp.used[3]);
}

void Device::allocate_tensor(DeviceMempool mp, Tensor & tens) {
  DYNET_ASSERT(mp != DeviceMempool::NONE, "Attempt to allocate tensor for NONE DeviceMempool");
  DYNET_ASSERT(pools[(int)mp] != nullptr, "Attempt to allocate tensor for null DeviceMempool");
  tens.v = (float*)pools[(int)mp]->allocate(tens.d.size() * sizeof(float));
  DYNET_ASSERT(tens.v != nullptr, "Allocated tensor is zero");
  tens.mem_pool = mp;
}

#if HAVE_CUDA
Device_GPU::Device_GPU(int my_id, const DeviceMempoolSizes & mbs,
                       int device_id, unsigned seed) :
  Device(my_id, DeviceType::GPU, &gpu_mem), cuda_device_id(device_id), gpu_mem(device_id) {
  CUDA_CHECK(cudaSetDevice(device_id));
  CUBLAS_CHECK(cublasCreate(&cublas_handle));
  CUBLAS_CHECK(cublasSetPointerMode(cublas_handle, CUBLAS_POINTER_MODE_DEVICE));
  reset_rng(seed);
#if HAVE_CUDNN
  CUDNN_CHECK(cudnnCreate(&cudnnHandle));
#endif
  kSCALAR_MINUSONE = (float*)gpu_mem.mymalloc(sizeof(float));
  kSCALAR_ONE = (float*)gpu_mem.mymalloc(sizeof(float));
  kSCALAR_ZERO = (float*)gpu_mem.mymalloc(sizeof(float));
  name = "GPU:" + std::to_string(device_id);
  float minusone = -1;
  CUDA_CHECK(cudaMemcpyAsync(kSCALAR_MINUSONE, &minusone, sizeof(float), cudaMemcpyHostToDevice));
  float one = 1;
  CUDA_CHECK(cudaMemcpyAsync(kSCALAR_ONE, &one, sizeof(float), cudaMemcpyHostToDevice));
  float zero = 0;
  CUDA_CHECK(cudaMemcpyAsync(kSCALAR_ZERO, &zero, sizeof(float), cudaMemcpyHostToDevice));

  // Initialize the Eigen device
  estream = DYNET_NEW(Eigen::CudaStreamDevice(device_id));
  edevice = DYNET_NEW(Eigen::GpuDevice(estream));

  // this is the big memory allocation.
  pools[0] = DYNET_NEW(AlignedMemoryPool("GPU forward memory", (mbs.used[0] << 20), &gpu_mem));
  pools[1] = DYNET_NEW(AlignedMemoryPool("GPU backward memory", (mbs.used[1] << 20), &gpu_mem));
  pools[2] = DYNET_NEW(AlignedMemoryPool("GPU parameter memory", (mbs.used[2] << 20), &gpu_mem));
  pools[3] = DYNET_NEW(AlignedMemoryPool("GPU scratch memory", (mbs.used[3] << 20), &gpu_mem));
}

Device_GPU::~Device_GPU()
{
#if HAVE_CUDNN
  CUDNN_CHECK(cudnnDestroy(cudnnHandle));
#endif
}

void Device_GPU::reset_rng(unsigned seed) {
  CURAND_CHECK(curandCreateGenerator(&curandeng,
                                     CURAND_RNG_PSEUDO_PHILOX4_32_10));
  CURAND_CHECK(curandSetPseudoRandomGeneratorSeed(curandeng,
                                                  seed + 1));
}
#endif

Device_CPU::Device_CPU(int my_id, const DeviceMempoolSizes & mbs, bool shared, bool dynamic) :
    Device(my_id, DeviceType::CPU, &cpu_mem), shmem(mem), shared(shared) {
  if (shared) shmem = DYNET_NEW(SharedAllocator());
  kSCALAR_MINUSONE = (float*) shmem->mymalloc(sizeof(float));
  *kSCALAR_MINUSONE = -1;
  kSCALAR_ONE = (float*) shmem->mymalloc(sizeof(float));
  *kSCALAR_ONE = 1;
  kSCALAR_ZERO = (float*) shmem->mymalloc(sizeof(float));
  *kSCALAR_ZERO = 0;
  name = "CPU";

  // Initialize the Eigen device
  edevice = DYNET_NEW(Eigen::DefaultDevice);

  // This is the big memory allocation.
  // In order to specify dynamic, the value of expanding_unit needs to be included.
  static const size_t expanding_unit = 1<<24;
  pools[0] = DYNET_NEW(AlignedMemoryPool("CPU forward memory",   (mbs.used[0] << 20), &cpu_mem, expanding_unit, dynamic));
  pools[1] = DYNET_NEW(AlignedMemoryPool("CPU backward memory",  (mbs.used[1] << 20), &cpu_mem, expanding_unit, dynamic));
  pools[2] = DYNET_NEW(AlignedMemoryPool("CPU parameter memory", (mbs.used[2] << 20), shmem,    expanding_unit, dynamic));
  pools[3] = DYNET_NEW(AlignedMemoryPool("CPU scratch memory",   (mbs.used[3] << 20), &cpu_mem, expanding_unit, dynamic));
}

Device_CPU::~Device_CPU() {
  clear();
}

void Device_CPU::clear() {

  for (AlignedMemoryPool* pool : pools) DYNET_DEL(pool);
  pools.clear();

  DYNET_DEL(edevice);

  shmem->myfree(&kSCALAR_ONE);
  shmem->myfree(&kSCALAR_ZERO);
  shmem->myfree(&kSCALAR_MINUSONE);

  if (shared) DYNET_DEL(shmem);
}

DeviceManager::DeviceManager() {}

DeviceManager::~DeviceManager() {
  clear();
}

void DeviceManager::clear() {
  for (Device* device : devices) DYNET_DEL(device);
  devices.clear();
  devices_map.clear();
}

void DeviceManager::add(Device* d) {
  devices.push_back(d);
  devices_map[d->name] = d;
}

Device* DeviceManager::get_global_device(const std::string & name) {
  if (name == "") {
    if (!dynet::default_device) {
      throw std::runtime_error("Default device does not exist");
    }
    return dynet::default_device;
  }
  auto it = devices_map.find(name);
  if (it == devices_map.end()) {
    throw std::runtime_error("Invalid device name: " + name);
  }
  return it->second;
}

static DeviceManager* device_manager(nullptr);
static std::mutex device_manager_mutex;

DeviceManager* set_device_manager() {
  std::lock_guard<std::mutex> guard(device_manager_mutex);
  device_manager = DYNET_NEW(DeviceManager);
  return device_manager;
}

void reset_device_manager() {
  std::lock_guard<std::mutex> guard(device_manager_mutex);
  DYNET_DEL(device_manager);
}

DeviceManager* get_device_manager() {
  // In C++11, initialization of function local static objects is thread safe.
  // See https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
  // However, if it is set here, it can never be unset, which means it's difficult
  // to do a complete cleanup and re-initialization, perhaps to demonstrate that
  // there are no memory leaks or to change parameters.  Instead, explicitly
  // guard the thing.
  return device_manager;
}

} // namespace dynet
