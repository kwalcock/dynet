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

int main(int _argc, char** _argv) {
  MemDebug myMemDebug;

  myDebugMem(__FILE__, __LINE__);

  unsigned int seed = 42; // For repeatability, this is used repeatedly.
  const int threadCount = 36;
  const float expectedValue = 0.0313863866; // When reseeded.

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;
  std::string seedStr = std::to_string(seed);
  const char* seedPtr = seedStr.c_str();

  const char* args[] = {
    "",
    "--dynet-seed", seedPtr,
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1",
    "--dynet-forward-only", "1" // Does this imply the above?
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
  const unsigned int inputDim = 1;
  const unsigned int hiddenDim = 10;

  std::mutex coutMutex;

  dynet::ParameterCollection model; //  (0.0f); // no weight decay
  // This next one doesn't need to be copied.
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
  for (int i = 0; i < hiddenDim; i++)
    protoLookupParameter.initialize(i, { 14.5f - i });
  dynet::reset_rng(seed);
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);

  // Store this parameter collection for use elsewhere?
  dynet::TextFileSaver textFileSaver("ser-par.rnn", false);
  textFileSaver.save(model);

  for (int i = 0; i < 20; ++i)
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
//          if (t == 1) dumpCgs(cgs, i, 0, "a1-00"); // Does new_graph cause a problem? No.
          lstmBuilders[t]->new_graph(*cgs[t], false); // Do not update things
//          if (t == 1) dumpCgs(cgs, i, 0, "a1-0a"); // No problem caused.
          dumpCgs(cgs, i, t, "a");
//          if (t == 1) dumpCgs(cgs, i, 0, "a1-0b"); // A problem has been caused.
          std::vector<dynet::Expression> losses;
          for (size_t j = 0; j < inputDim; ++j) {
            lstmBuilders[t]->start_new_sequence();
            for (size_t k = 0; k < inputDim; ++k) {
              dynet::Expression x = dynet::lookup(*cgs[t], protoLookupParameter, j * inputDim + k);
              lstmBuilders[t]->add_input(x);
            }
//            if (t == 1) dumpCgs(cgs, i, 0, "b1-0a");
            dumpCgs(cgs, i, t, "b");
//            if (t == 1) dumpCgs(cgs, i, 0, "b1-0b");
            losses.push_back(dynet::squared_norm(lstmBuilders[t]->final_h()[layers - 1]));
//            if (t == 1) dumpCgs(cgs, i, 0, "c1-0a");
            dumpCgs(cgs, i, t, "c");
//            if (t == 1) dumpCgs(cgs, i, 0, "c1-0b");
          }
//          losses.push_back(losses[0] + losses[inputDim - 1]);

          auto l0_value_scalar = dynet::as_scalar(losses[0].value());
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0a");
          dumpCgs(cgs, i, t, "d");
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0b");

          {
            std::lock_guard<std::mutex> guard(coutMutex);
            if (std::abs(l0_value_scalar - expectedValue) > 0.0001)
              std::cout << "Wrong answer!" << " " << l0_value_scalar << std::endl;
            else
              std::cout << "Right answer!" << std::endl;
          }

          results[t].push_back(l0_value_scalar);
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " finished!" << std::endl;
          }
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0a");
          dumpCgs(cgs, i, t, "d1");
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0b");
        });
      }

      for (size_t t = 0; t < threadCount; ++t) threads[t].join();

      for (int i = 0; i < threadCount; i++) {
        delete cgs.back(); cgs.pop_back();
        delete lstmBuilders.back(); lstmBuilders.pop_back();
      }
    }

    for (size_t t = 0; t < threadCount; ++t)
      for (size_t i = 0; i < results[t].size(); ++i)
        if (abs(results[0][0] - results[t][i]) >= 0.0001)
          std::cerr << "Parallel test failed!" << std::endl;
//    dumpCgs(cgs, i, 0, "e");
//    dumpCgs(cgs, i, 1, "e");
    std::cout << std::endl;
  }

  myDebugMem(__FILE__, __LINE__);
  dynet::cleanup();
  myDebugMem(__FILE__, __LINE__);
  std::cout << "Program finished!" << std::endl;
}
