#ifndef DYNET_EIGEN_INIT_H
#define DYNET_EIGEN_INIT_H

#include <string>
#include <vector>

namespace dynet {

extern float default_weight_decay_lambda;
extern int autobatch_flag;
extern int profiling_flag;
extern int forward_only_flag;
extern int dynamic_mem_flag;

/**
 * \brief Represents general parameters for dynet
 *
 */
struct DynetParams {
  DynetParams();
  ~DynetParams();
  unsigned random_seed; /**< The seed for random number generation */
  std::string mem_descriptor; /**< Total memory to be allocated for Dynet */
  float weight_decay; /**< Weight decay rate for L2 regularization */
  int autobatch; /**< Whether to autobatch or not */
  int profiling; /**< Whether to show autobatch debug info or not */
  int forward_only = 0; /**< Whether to support only inference, the forward pass */
  bool shared_parameters; /**< TO DOCUMENT */
  bool ngpus_requested; /**< GPUs requested by number */
  bool ids_requested; /**< GPUs requested by ids */
  bool cpu_requested; /**< CPU requested in multi-device case */
  int requested_gpus; /**< Number of requested GPUs */
  std::vector<int> gpu_mask; /**< List of required GPUs by ids */
  bool dynamic = false; /**< Dynamically allocate CPU memory for thread safety */
};

DynetParams extract_dynet_params(int& argc, char**& argv, bool shared_parameters = false);
void initialize(DynetParams& params);
void initialize(int& argc, char**& argv, bool shared_parameters = false);
void cleanup();

/**
 * \brief Resets random number generators
 *
 */
void reset_rng(unsigned seed);

} // namespace dynet

#endif
