#include "dynet/mem_debug.h"

#include <dynet/dynet.h>
#include <dynet/lstm.h>
#include <memory>
#include <thread>

/*
It works without dynamic memory if thread count is 1.
It does not work without dynamic memory if thread count is 2.
  An exception is purposely thrown from the C++ code.

It works with dynamic memory if thread count is 1.
It does not work with dynamic memory if thread count is 2.
  An exception is thrown by the OS during operator delete.
*/

int main(int _argc, char** _argv) {
  debugMem(__FILE__, __LINE__);

  const int threadCount = 1; // 8

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;

  char* args[] = {
    "",
    "--dynet-seed", "10",
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1"
  };
  char** argv = &args[0];
  int argc = 7;

  debugMem(__FILE__, __LINE__);
  int* test = new int[10];
  debugMem(__FILE__, __LINE__);
  delete test;
  debugMem(__FILE__, __LINE__);

  dynet::initialize(argc, argv);
  debugMem(__FILE__, __LINE__);

  char* kwa = (char*) malloc(4);
  kwa[0] = 'H';
  kwa[1] = 'i';
  kwa[2] = '!';
  kwa[3] = '\0';

  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;

  {
    std::vector<std::vector<float>> results(threadCount);
    {
      dynet::ParameterCollection model;
      dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
      dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
      dynet::autobatch_flag = 0;

      debugMem(__FILE__, __LINE__);

      std::vector<std::thread> threads(threadCount);
      for (size_t t = 0; t < threadCount; ++t) {
        threads[t] = std::thread([&, t]() {
          std::cout << "Thread " << t << " started!" << std::endl;
          dynet::ComputationGraph cg;
          debugMem(__FILE__, __LINE__);
          dynet::VanillaLSTMBuilder lstmBuilder(protoLstmBuilder);
          debugMem(__FILE__, __LINE__);
          dynet::LookupParameter lookupParameter(protoLookupParameter);
          debugMem(__FILE__, __LINE__);
          lstmBuilder.new_graph(cg);
          debugMem(__FILE__, __LINE__);

          std::vector<dynet::Expression> losses;
          debugMem(__FILE__, __LINE__);
          for (size_t j = 0; j < inputDim; ++j) {
            debugMem(__FILE__, __LINE__);
            lstmBuilder.start_new_sequence();
            debugMem(__FILE__, __LINE__);
            for (size_t k = 0; k < inputDim; ++k) {
              debugMem(__FILE__, __LINE__);
              dynet::Expression x = dynet::lookup(cg, lookupParameter, j * inputDim + k);
              debugMem(__FILE__, __LINE__);
              lstmBuilder.add_input(x);
              debugMem(__FILE__, __LINE__);
            }
            debugMem(__FILE__, __LINE__);
            losses.push_back(dynet::squared_norm(lstmBuilder.final_h()[layers - 1]));
            debugMem(__FILE__, __LINE__);
          }
          debugMem(__FILE__, __LINE__);
          losses.push_back(losses[0] + losses[inputDim - 1]);
          debugMem(__FILE__, __LINE__);
          dynet::Expression z = dynet::sum(losses);
          debugMem(__FILE__, __LINE__);
//          auto z_value = z.value();
          debugMem(__FILE__, __LINE__);
//          auto z_value_scalar = dynet::as_scalar(z_value);
          debugMem(__FILE__, __LINE__);
//          results[t].push_back(z_value_scalar);
          std::cout << "Thread " << t << " finished!" << std::endl;
        });
      }

      for (size_t t = 0; t < threadCount; ++t) threads[t].join();
//      for (size_t t = 0; t < threadCount; ++t)
//        for (size_t i = 1; i < results[t].size(); ++i)
//          if (abs(results[t][0] - results[t][i]) >= 0.0001)
//            std::cerr << "Parallel test failed!" << std::endl;
    }
  }

  debugMem(__FILE__, __LINE__);
  dynet::cleanup();
  debugMem(__FILE__, __LINE__);
  std::cout << "Program finished!" << std::endl;
}
