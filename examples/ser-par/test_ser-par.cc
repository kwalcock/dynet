#include <dynet/dynet.h>
#include <dynet/expr.h>
#include <dynet/exec.h>
#include <dynet/lstm.h>
#include <dynet/fast-lstm.h>
#include <dynet/gru.h>
#include <dynet/grad-check.h>
#include <dynet/param-init.h>

#include <thread>

using namespace dynet;
using namespace std;

int main(int _argc, char** _argv) {
  cout << "Program started!" << endl;

  char* args[] = {
    "--dynet-seed", "10",
    "--dynet-mem", "1",
    "--dynet-dynamic-mem", "0"
  };
  char** argv = &args[0];
  int argc = 6;

  dynet::initialize(argc, argv);

  const int threadCount = 1;

  vector<vector<float>> results(threadCount);
  dynet::ParameterCollection model;
  dynet::VanillaLSTMBuilder lstm_proto(2, 3, 10, model);
  dynet::LookupParameter lp_proto = model.add_lookup_parameters(10, {3});
  dynet::autobatch_flag = 0;
  
  vector<thread> threads(threadCount);
  for (size_t t = 0; t < threadCount; ++t) {
    threads[t] = thread([&, t]() {
      cout << "Thread started!" << endl;
      dynet::ComputationGraph cg;
      dynet::VanillaLSTMBuilder lstm(lstm_proto);
      dynet::LookupParameter lp(lp_proto);
      lstm.new_graph(cg);
        
      vector<Expression> losses;
      for (size_t j = 0; j < 3; ++j) {
        lstm.start_new_sequence();
        for (size_t k = 0; k < 3; ++k) {
          Expression x = dynet::lookup(cg, lp, j*3 + k);
          lstm.add_input(x);
        }
        losses.push_back(squared_norm(lstm.final_h()[1]));
      }
      losses.push_back(losses[0] + losses[2]);
      Expression z = dynet::sum(losses);
      results[t].push_back(as_scalar(z.value()));
      cout << "Thread finished!" << endl;
    });
  }

  for (size_t t = 0; t < threadCount; ++t) { threads[t].join(); }
  for (size_t t = 0; t < threadCount; ++t) {
    for (size_t i = 1; i < results[t].size(); ++i) {
    if (abs(results[t][0] - results[t][i]) >= 0.0001)
      cerr << "parallel test failed" << endl;
    }
  }
  cout << "Program finished!" << endl;
}
