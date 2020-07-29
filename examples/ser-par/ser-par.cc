#include "dynet/mem_debug.h"

#include <dynet/dynet.h>
#include <dynet/lstm.h>
#include <dynet/tensor.h>

#include <memory>
#include <mutex>
#include <strstream>
#include <thread>


std::string mkFilename(int index, int t, char* id) {
  std::strstream stream;
  stream << "cg_i" << index << "_t" << t << "_" << id << ".txt" << std::ends;
  std::string string = stream.str();
  return string;
}

void dumpCgs(dynet::ComputationGraph** cgs, int index, int t, char* id) {
  cgs[t]->dump(mkFilename(index, t, id), true, true, false);
}

void myDebugMem(char* file, int line) {
//  debugMem(file, line);
}

int main(int _argc, char** _argv) {
  // Guarantee that despite lack of debugMem statements there is a MemDebug
  // object in memory.  The global one in mem_debug.cc seems to disappear.
  MemDebug myMemDebug;

  myDebugMem(__FILE__, __LINE__);

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

  dynet::initialize(argc, argv);
  myDebugMem(__FILE__, __LINE__);

  // This guarantees a memory leak which when displayed at program termination
  // verifies that leak detection is active.
  char* kwa = (char*) malloc(4);
  kwa[0] = 'H';
  kwa[1] = 'i';
  kwa[2] = '!';
  kwa[3] = '\0';

  const int layers = 2;
  const unsigned int inputDim = 1;
  const unsigned int hiddenDim = 10;

  std::mutex coutMutex;

  dynet::ParameterCollection model; //  (0.0f); // no weight decay
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
  dynet::autobatch_flag = 0;

  for (int i = 0; i < 20; ++i)
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
//        threads[t] = std::thread([&, t]() {
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " started!" << std::endl;
          }
          lstmBuilders[t]->new_graph(*cgs[t], false); // Do not update things
          dumpCgs(cgs, i, t, "a");
          std::vector<dynet::Expression> losses;
          for (size_t j = 0; j < inputDim; ++j) {
            lstmBuilders[t]->start_new_sequence();
            for (size_t k = 0; k < inputDim; ++k) {
              dynet::Expression x = dynet::lookup(*cgs[t], *lookupParameters[t], j * inputDim + k);
              lstmBuilders[t]->add_input(x);
            }
            dumpCgs(cgs, i, t, "b");
            losseses[t]->push_back(dynet::squared_norm(lstmBuilders[t]->final_h()[layers - 1]));
            dumpCgs(cgs, i, t, "c");
          }
//          losses.push_back(losses[0] + losses[inputDim - 1]);

          auto l0_value_scalar = dynet::as_scalar((*losseses[t])[0].value());
          dumpCgs(cgs, i, t, "d");

//          if (std::abs(l0_value_scalar - 0.00966324471) > 0.0001)
//          if (std::abs(l0_value_scalar - 0.00220352481) > 0.0001)
          if (std::abs(l0_value_scalar - 0.000341659179) > 0.0001)
            std::cout << "Wrong answer!" << std::endl;
          else
            std::cout << "Right answer!" << std::endl;

          results[t].push_back(l0_value_scalar);
          {
            std::lock_guard<std::mutex> guard(coutMutex);
            std::cout << "Thread " << t << " finished!" << std::endl;
          }
//        });
      }

      //for (size_t t = 0; t < threadCount; ++t) threads[t].join();
//    }

//    for (size_t t = 1; t < threadCount; ++t)
//      for (size_t i = 1; i < results[t].size(); ++i)
//        if (abs(results[0][0] - results[t][0]) >= 0.0001)
    if (abs(results[0][0] - results[1][0]) >= 0.0001)
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
