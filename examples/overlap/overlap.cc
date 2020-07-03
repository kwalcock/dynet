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

class OverlappingLstm {

public:
  int t;

  OverlappingLstm(int t, dynet::VanillaLSTMBuilder &protoLstmBuilder, dynet::LookupParameter &protoLookupParameter):
      t(t)  {
    std::cout << "Thread " << t << " started!" << std::endl;
  }

  std::vector<float> test(dynet::VanillaLSTMBuilder &protoLstmBuilder, dynet::LookupParameter &protoLookupParameter, int layers, int inputDim) {
    dynet::ComputationGraph cg;
    dynet::VanillaLSTMBuilder lstmBuilder(protoLstmBuilder);
    dynet::LookupParameter lookupParameter(protoLookupParameter);
    lstmBuilder.new_graph(cg);

    std::vector<dynet::Expression> losses;
    for (size_t j = 0; j < inputDim; ++j) {
      lstmBuilder.start_new_sequence();
      for (size_t k = 0; k < inputDim; ++k) {
        dynet::Expression x = dynet::lookup(cg, lookupParameter, j * inputDim + k);
        lstmBuilder.add_input(x);
      }
      losses.push_back(dynet::squared_norm(lstmBuilder.final_h()[layers - 1]));
    }
    losses.push_back(losses[0] + losses[inputDim - 1]);
    dynet::Expression z = dynet::sum(losses);
    std::vector<float> results;
    results.push_back(dynet::as_scalar(z.value()));
    std::cout << "Thread " << t << " finished!" << std::endl;
    return results;
  }
};

int main(int _argc, char** _argv) {
  const int threadCount = 1;

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

  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;

  std::vector<std::vector<float>> results(threadCount);
  dynet::ParameterCollection model;
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, { inputDim });
  dynet::autobatch_flag = 0;

  // This should work OK if they are nested, but not if they are not.
  OverlappingLstm overlappingLstm0(0, protoLstmBuilder, protoLookupParameter);
//  OverlappingLstm overlappingLstm1(1, protoLstmBuilder, protoLookupParameter);
  results[0] = overlappingLstm0.test(protoLstmBuilder, protoLookupParameter, layers, inputDim);
//  results[1] = overlappingLstm1.test(layers, inputDim);

  for (size_t t = 0; t < threadCount; ++t)
    for (size_t i = 1; i < results[t].size(); ++i)
      if (abs(results[t][0] - results[t][i]) >= 0.0001)
        std::cerr << "Parallel test failed!" << std::endl;
  std::cout << "Program finished!" << std::endl;
}
