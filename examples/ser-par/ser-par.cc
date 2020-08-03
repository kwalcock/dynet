#include "dynet/mem_debug.h"

#include <dynet/dynet.h>
#include <dynet/lstm.h>
#include <dynet/tensor.h>

#include <memory>
#include <mutex>
#include <cstring>
#include <strstream>
#include <thread>


std::string mkFilename(int index, int t, const char* id) {
  std::strstream stream;
  stream << "cg_i" << index << "_t" << t << "_" << id << ".txt" << std::ends;
  std::string string = stream.str();
  return string;
}

void dumpCgs(dynet::ComputationGraph** cgs, int index, int t, const char* id) {
//  cgs[t]->dump(mkFilename(index, t, id), true, true, false);
}

void myDebugMem(const char* file, int line) {
//  debugMem(file, line);
}

int main(int _argc, char** _argv) {
  MemDebug myMemDebug;

  myDebugMem(__FILE__, __LINE__);

  const int threadCount = 2;

  std::cout << "Program started for " << threadCount << " threads!" << std::endl;

  const char* args[] = {
    "",
    "--dynet-seed", "10",
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
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });

  for (int i = 0; i < 100; ++i)
  {
    std::vector<std::vector<float>> results(threadCount);
    // This block assured that the variables below were destructed.
//    {
      myDebugMem(__FILE__, __LINE__);
      // This is usually done internally to a thread and not in duplicate like this.
      // However, this way the cgs can be analyzed after the computation.
      dynet::ComputationGraph cg0, cg1;
      dynet::VanillaLSTMBuilder lstmBuilder0(protoLstmBuilder), lstmBuilder1(protoLstmBuilder);
      dynet::LookupParameter lookupParameter0(protoLookupParameter), lookupParameter1(protoLookupParameter);
      std::vector<dynet::Expression> losses0;
      std::vector<dynet::Expression> losses1;

      dynet::ComputationGraph* cgs[] = { &cg0, &cg1 };
      dynet::VanillaLSTMBuilder* lstmBuilders[] = { &lstmBuilder0, &lstmBuilder1 };
      dynet::LookupParameter* lookupParameters[] = { &lookupParameter0, &lookupParameter1 };
      std::vector<dynet::Expression>* losseses[] = { &losses0, &losses1 };

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
              dynet::Expression x = dynet::lookup(*cgs[t], *lookupParameters[t], j * inputDim + k);
              lstmBuilders[t]->add_input(x);
            }
//            if (t == 1) dumpCgs(cgs, i, 0, "b1-0a");
            dumpCgs(cgs, i, t, "b");
//            if (t == 1) dumpCgs(cgs, i, 0, "b1-0b");
            losseses[t]->push_back(dynet::squared_norm(lstmBuilders[t]->final_h()[layers - 1]));
//            if (t == 1) dumpCgs(cgs, i, 0, "c1-0a");
            dumpCgs(cgs, i, t, "c");
//            if (t == 1) dumpCgs(cgs, i, 0, "c1-0b");
          }
//          losses.push_back(losses[0] + losses[inputDim - 1]);

          auto l0_value_scalar = dynet::as_scalar((*losseses[t])[0].value());
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0a");
          dumpCgs(cgs, i, t, "d");
//          if (t == 1) dumpCgs(cgs, i, 0, "d1-0b");

//          if (std::abs(l0_value_scalar - 0.00966324471) > 0.0001)
//          if (std::abs(l0_value_scalar - 0.00220352481) > 0.0001)
          if (std::abs(l0_value_scalar - 0.000341659179) > 0.0001)
            std::cout << "Wrong answer!" << " " << l0_value_scalar << std::endl;
          else
            std::cout << "Right answer!" << std::endl;

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
//    }

    for (size_t t = 1; t < threadCount; ++t)
//      for (size_t i = 1; i < results[t].size(); ++i)
//        if (abs(results[0][0] - results[t][0]) >= 0.0001)
    if (abs(results[0][0] - results[t][0]) >= 0.0001)
      std::cerr << "Parallel test failed!" << std::endl;
    dumpCgs(cgs, i, 0, "e");
    dumpCgs(cgs, i, 1, "e");
    std::cout << std::endl;
  }

  myDebugMem(__FILE__, __LINE__);
  dynet::cleanup();
  myDebugMem(__FILE__, __LINE__);
  std::cout << "Program finished!" << std::endl;
}
