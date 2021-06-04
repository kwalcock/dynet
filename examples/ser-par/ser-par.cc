#include "dynet/mem_debug.h"

#include <dynet/dynet.h>
#include <dynet/io.h>
#include <dynet/lstm.h>
#include <dynet/tensor.h>

#include <memory>
#include <mutex>
#include <cstring>
#include <strstream>
#include <thread>
#include <vector>

std::string mkFilename(int index, int t, const char* id) {
  std::strstream stream;
  stream << "cg_i" << index << "_t" << t << "_" << id << ".txt" << std::ends;
  std::string string = stream.str();
  return string;
}

void dumpCgs(std::vector<dynet::ComputationGraph*>& cgs, int index, int t, const char* id) {
//  cgs[t]->dump(mkFilename(index, t, id), true, true, false);
}

void myDebugMem(const char* file, int line) {
//  debugMem(file, line);
}

bool run(int _argc, char** _argv) {
  myDebugMem(__FILE__, __LINE__);

  unsigned int seed = 42; // For repeatability, this is used repeatedly.
  const int threadCount = 36;
  const float evenExpectedValue = 0.0819774419; // When reseeded.
  const float oddExpectedValue = 0.0907375515; // When reseeded.
  bool failed = false;

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;
  std::string seedStr = std::to_string(seed);
  const char* seedPtr = seedStr.c_str();

  const char* args[] = {
    "",
    "--dynet-seed", seedPtr,
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1",
    "--dynet-forward-only", "1"
  };
  char** argv = (char**) &args[0];
  int argc = 9;

  dynet::initialize(argc, argv);
  myDebugMem(__FILE__, __LINE__);

  // This guarantees a memory leak which when displayed at program termination
  // verifies that leak detection is active.
  char* leak = (char*) malloc(4);
  strcpy(leak, "Hi!");

  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;
  const unsigned int lookupLength = 12;
  const unsigned int sequenceCount = 5;
  const unsigned int sequenceLength = 4;

  std::mutex coutMutex;

  dynet::ParameterCollection model; //  (0.0f); // no weight decay
  // This next one doesn't need to be copied.
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(lookupLength, { inputDim });
  for (int i = 0; i < lookupLength; i++)
    protoLookupParameter.initialize(i, { 14.5f - i, 2.3f + i, -7.9f + 2.0f * i }); // inputDim here
  dynet::reset_rng(seed);
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);

  // Store this parameter collection for use elsewhere?
  dynet::TextFileSaver textFileSaver("ser-par.rnn", false);
  textFileSaver.save(model);

  for (int outerLoop = 0; outerLoop < 20; ++outerLoop)
  {
    std::vector<std::vector<float>> results(threadCount);
    // This block assured that the variables below were destructed.
    {
      myDebugMem(__FILE__, __LINE__);

      std::vector<dynet::ComputationGraph*> cgs;
      std::vector<dynet::VanillaLSTMBuilder*> lstmBuilders;

      for (int i = 0; i < threadCount; i++) {
        dynet::ComputationGraph* cg = new dynet::ComputationGraph();
        dynet::VanillaLSTMBuilder* lstmBuilder = new dynet::VanillaLSTMBuilder(protoLstmBuilder);

        cgs.push_back(cg);
        lstmBuilders.push_back(lstmBuilder);
      }

      std::vector<std::thread> threads(threadCount);
      for (size_t t = 0; t < threadCount; ++t) {
        // Comment out for serial processing.
        threads[t] = std::thread([&, t]() {
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " started!" << std::endl;
          }
          for (int innerLoop = 0; innerLoop < 10; ++innerLoop) {
            lstmBuilders[t]->new_graph(*cgs[t], false); // Do not update things
            std::vector<dynet::Expression> losses;
            for (size_t j = 0; j < sequenceCount; ++j) {
              lstmBuilders[t]->start_new_sequence();
              for (size_t k = 0; k < sequenceLength; ++k) {
                // Comparisons below are done for the first loss, which is for j = 0,
                // so these indexes must be different even when j is 0.
                int index = (innerLoop % 2 == 0) ? (5 * k + 2 * j) % lookupLength : (3 * k - 2 * j) % lookupLength;
                dynet::Expression x = dynet::lookup(*cgs[t], protoLookupParameter, index);
                lstmBuilders[t]->add_input(x);
              }
              losses.push_back(dynet::squared_norm(lstmBuilders[t]->final_h()[layers - 1]));
            }

            // Are all losses the same?
            auto l0_value_scalar = dynet::as_scalar(losses[0].value());
//            for (size_t index = 1; index < losses.size(); ++index) {
//              auto newl0_value_scalar = dynet::as_scalar(losses[index].value());
//              if (newl0_value_scalar != l0_value_scalar)
//                std::cerr << "The value changed!";
//            }

            {
              std::lock_guard<std::mutex> guard(coutMutex);
              float expectedValue = (innerLoop % 2 == 0) ? evenExpectedValue : oddExpectedValue;
              if (std::abs(l0_value_scalar - expectedValue) > 0.0001) {
                std::cout << "Wrong answer!" << " " << l0_value_scalar << std::endl;
                failed = true;
              }
              else
                std::cout << "Right answer!" << std::endl;
            }
            results[t].push_back(l0_value_scalar);
          }
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " finished!" << std::endl;
          }
        });
      }

      for (size_t t = 0; t < threadCount; ++t) threads[t].join();

      for (int i = 0; i < threadCount; i++) {
        delete cgs.back(); cgs.pop_back();
        delete lstmBuilders.back(); lstmBuilders.pop_back();
      }
    }

    for (size_t t = 0; t < threadCount; ++t) {
      for (size_t i = 0; i < results[t].size(); i += 2)
        if (abs(results[0][0] - results[t][i]) >= 0.0001) {
          std::cerr << "Parallel test failed!" << std::endl;
          failed = true;
        }
      for (size_t i = 1; i < results[t].size(); i += 2)
        if (abs(results[0][1] - results[t][i]) >= 0.0001) {
          std::cerr << "Parallel test failed!" << std::endl;
          failed = true;
        }
    }
    std::cout << std::endl;
  }
  return failed;
}

int main(int _argc, char** _argv) {
  //  MemDebug myMemDebug;
  bool failed = run(_argc, _argv);

  if (failed)
    std::cerr << "Program failed!" << std::endl;
  else
    std::cerr << "Program passed!" << std::endl;

  myDebugMem(__FILE__, __LINE__);
  dynet::cleanup();
  myDebugMem(__FILE__, __LINE__);

  std::cout << "Program finished!" << std::endl;
}
