#include "dynet/mem_debug.h"

#include <dynet/dynet.h>
#include <dynet/lstm.h>
#include <dynet/tensor.h>

#include <memory>
#include <mutex>
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
  MemDebug myMemDebug;

//  debugMem(__FILE__, __LINE__);

  const int threadCount = 2;

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;

  char* args[] = {
    "",
    "--dynet-seed", "10",
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1"
  };
  char** argv = &args[0];
  int argc = 7;

//  debugMem(__FILE__, __LINE__);
  int* test = new int[10];
//  debugMem(__FILE__, __LINE__);
  delete test;
//  debugMem(__FILE__, __LINE__);

  dynet::initialize(argc, argv);
//  debugMem(__FILE__, __LINE__);

  char* kwa = (char*) malloc(4);
  kwa[0] = 'H';
  kwa[1] = 'i';
  kwa[2] = '!';
  kwa[3] = '\0';

  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;

  std::mutex coutMutex;

  dynet::ParameterCollection model; //  (0.0f); // no weight decay
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
  dynet::autobatch_flag = 0;

  for (int i = 0; i < 100; ++i)
  {
    std::vector<std::vector<float>> results(threadCount);
    {

//      debugMem(__FILE__, __LINE__);

      std::vector<std::thread> threads(threadCount);
      for (size_t t = 0; t < threadCount; ++t) {
        threads[t] = std::thread([&, t]() {
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " started!" << std::endl;
          }
          dynet::ComputationGraph cg;
          dynet::VanillaLSTMBuilder lstmBuilder(protoLstmBuilder);
          dynet::LookupParameter lookupParameter(protoLookupParameter);
          lstmBuilder.new_graph(cg, false); // Do not update things

          dynet::Tensor luValues = lookupParameter.values()->at(0);
          dynet::Tensor luValues0 = luValues.batch_elem(0);
          float* floats = luValues0.v;
          {
            for (int i = 0; i < inputDim * hiddenDim; ++i) {
              std::lock_guard<std::mutex> guard(coutMutex);
              float f = floats[i];
              std::cout << f << " ";
            }
            std::cout << std::endl << std::endl;
          }

          std::vector<dynet::Expression> losses;
          for (size_t j = 0; j < inputDim; ++j) {
            lstmBuilder.start_new_sequence();
            for (size_t k = 0; k < inputDim; ++k) {
              dynet::Expression x = dynet::lookup(cg, lookupParameter, j * inputDim + k);
              lstmBuilder.add_input(x);
            }
            losses.push_back(dynet::squared_norm(lstmBuilder.final_h()[layers - 1]));
          }
//          losses.push_back(losses[0] + losses[inputDim - 1]);

          auto l0_value_scalar = dynet::as_scalar(losses[0].value());
          
          if (std::abs(l0_value_scalar - 0.00966324471) > 0.0001)
            std::cout << "Wrong answer!" << std::endl;
          else
            std::cout << "Right answer!" << std::endl;
          auto l1_value_scalar = dynet::as_scalar(losses[1].value());
//          auto l2_value_scalar = dynet::as_scalar(losses[2].value());
//          auto l3_value_scalar = dynet::as_scalar(losses[3].value());


          // Use this one if have internal sum.
//          if (std::abs(z_value_scalar - 0.0250536269) > 0.0001) {
//          if (std::abs(z_value_scalar - 0.0145669077) > 0.0001) {            
          if (std::abs(l1_value_scalar - 0.00408018893) > 0.0001) {
              std::cout << "Wrong answer!" << std::endl;
          }
          else
            std::cout << "Right answer!" << std::endl;

//          dynet::Expression z = dynet::sum(losses);
//          auto z_value = z.value(); // Sometimes crashes here.
//          auto z_value_scalar = dynet::as_scalar(z_value);

//          results[t].push_back(z_value_scalar);
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " finished!" << std::endl;
          }
        });
      }

      for (size_t t = 0; t < threadCount; ++t) threads[t].join();
    }




//    for (size_t t = 1; t < threadCount; ++t)
//      for (size_t i = 1; i < results[t].size(); ++i)
//        if (abs(results[0][0] - results[t][0]) >= 0.0001)
//          std::cerr << "Parallel test failed!" << std::endl;
//    std::cout << std::endl;
  }

//  debugMem(__FILE__, __LINE__);
  dynet::cleanup();
//  debugMem(__FILE__, __LINE__);
  std::cout << "Program finished!" << std::endl;
}
