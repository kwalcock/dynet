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
using namespace dynet;
using namespace std;

int main(int _argc, char** _argv) {
  cout << "Program started!" << endl;

  char* args[] = {
    "--dynet-seed", "10",
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "1"
  };
  char** argv = &args[0];
  int argc = 6;

  dynet::initialize(argc, argv);

  const int threadCount = 2;
  const int layers = 2;
  const unsigned int inputDim = 3;
  const unsigned int hiddenDim = 10;

  vector<vector<float>> results(threadCount);
  dynet::ParameterCollection model;
  dynet::VanillaLSTMBuilder protoLstmBuilder(layers, inputDim, hiddenDim, model);
  dynet::LookupParameter protoLookupParameter = model.add_lookup_parameters(hiddenDim, {inputDim});
  dynet::autobatch_flag = 0;
  
  vector<thread> threads(threadCount);
  for (size_t t = 0; t < threadCount; ++t) {
    threads[t] = thread([&, t]() {
      cout << "Thread started!" << endl;
      dynet::ComputationGraph cg;
      dynet::VanillaLSTMBuilder lstmBuilder(protoLstmBuilder);
      dynet::LookupParameter lookupParameter(protoLookupParameter);
      lstmBuilder.new_graph(cg);
        
      vector<Expression> losses;
      for (size_t j = 0; j < inputDim; ++j) {
        lstmBuilder.start_new_sequence();
        for (size_t k = 0; k < inputDim; ++k) {
          Expression x = dynet::lookup(cg, lookupParameter, j * inputDim + k);
          lstmBuilder.add_input(x);
        }
        losses.push_back(squared_norm(lstmBuilder.final_h()[layers - 1]));
      }
      losses.push_back(losses[0] + losses[inputDim - 1]);
      Expression z = dynet::sum(losses);
      results[t].push_back(as_scalar(z.value()));
      cout << "Thread finished!" << endl;
    });
  }

  for (size_t t = 0; t < threadCount; ++t) threads[t].join();
  for (size_t t = 0; t < threadCount; ++t)
    for (size_t i = 1; i < results[t].size(); ++i)
      if (abs(results[t][0] - results[t][i]) >= 0.0001)
        cerr << "Parallel test failed!" << endl;
  cout << "Program finished!" << endl;
}
