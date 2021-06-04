#include "dynet/mem_debug.h"
#include "dynet/saxe-init.h"
#include "dynet/tensor.h"
#include "dynet/tensor-eigen.h"
#include "dynet/globals.h"

#include <random>
#include <cstring>

#include <Eigen/SVD>

using namespace std;

namespace dynet {

void orthonormal_random(unsigned dd, float g, Tensor& x) {
  Tensor t;
  t.d = Dim({dd, dd});
  t.v = DYNET_NEW_ARR(float[dd * dd]);
  normal_distribution<float> distribution(0, 0.01);
  {
    const std::lock_guard<std::mutex> rndengLock(rndengMutex);
    auto b = [&]() {return distribution(*rndeng); };
    generate(t.v, t.v + dd * dd, b);
  }
  Eigen::JacobiSVD<Eigen::MatrixXf> svd(mat(t), Eigen::ComputeFullU);
  mat(x) = svd.matrixU();
  DYNET_DEL_ARR(t.v);
}

}

