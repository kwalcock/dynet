#include "dynet/debug.h"

#include <dynet/dynet.h>
#include <dynet/lstm.h>

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
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();

  const int threadCount = 8;

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;

  char* args[] = {
    "",
    "--dynet-seed", "10",
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1"
  };
  char** argv = &args[0];
  int argc = 7;

  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
  int* test = new int[10];
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
  delete test;
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();

  dynet::initialize(argc, argv);
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();

  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;

  {
    std::vector<std::vector<float>> results(threadCount);
    {
//      dynet::ParameterCollection model;
//      dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
//      dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
//      dynet::autobatch_flag = 0;

      std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();

      /*
      std::vector<std::thread> threads(threadCount);
      for (size_t t = 0; t < threadCount; ++t) {
        threads[t] = std::thread([&, t]() {
          std::cout << "Thread " << t << " started!" << std::endl;
          dynet::ComputationGraph cg;
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          dynet::VanillaLSTMBuilder lstmBuilder(protoLstmBuilder);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          dynet::LookupParameter lookupParameter(protoLookupParameter);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          lstmBuilder.new_graph(cg);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;

          std::vector<dynet::Expression> losses;
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          for (size_t j = 0; j < inputDim; ++j) {
            std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
            lstmBuilder.start_new_sequence();
            std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
            for (size_t k = 0; k < inputDim; ++k) {
              std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
              dynet::Expression x = dynet::lookup(cg, lookupParameter, j * inputDim + k);
              std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
              lstmBuilder.add_input(x);
              std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
            }
            std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
            losses.push_back(dynet::squared_norm(lstmBuilder.final_h()[layers - 1]));
            std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          }
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          losses.push_back(losses[0] + losses[inputDim - 1]);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
          dynet::Expression z = dynet::sum(losses);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
//          auto z_value = z.value();
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
//          auto z_value_scalar = dynet::as_scalar(z_value);
          std::cout << "Thread " << t << " line " << __LINE__ << std::endl;
//          results[t].push_back(z_value_scalar);
          std::cout << "Thread " << t << " finished!" << std::endl;
        });
      }
*/
//      for (size_t t = 0; t < threadCount; ++t) threads[t].join();
//      for (size_t t = 0; t < threadCount; ++t)
//        for (size_t i = 1; i < results[t].size(); ++i)
//          if (abs(results[t][0] - results[t][i]) >= 0.0001)
//            std::cerr << "Parallel test failed!" << std::endl;
    }
  }

  std::cout << "Program finished!" << std::endl;

  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
  dynet::cleanup();
  std::cout << "Memory leaks at line " << __LINE__ << std::endl; _CrtDumpMemoryLeaks();
}
