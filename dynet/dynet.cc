#include <iomanip>
#include <mutex>

#include "dynet/aligned-mem-pool.h"
#include "dynet/devices.h"
#include "dynet/dynet.h"
#include "dynet/dynet-helper.h"
#include "dynet/exec.h"
#include "dynet/expr.h"
#include "dynet/param-nodes.h"
#include "dynet/timing.h"

namespace dynet {

  float* kSCALAR_MINUSONE;
  float* kSCALAR_ONE;
  float* kSCALAR_ZERO;

  class CgTracker {
  protected:
    int n_hgs = 0;
    unsigned n_cumul_hgs = 0;

  public:
    int get_active_count() {
      return n_hgs;
    }

    unsigned inc_active_count() {
      n_hgs++;
      n_cumul_hgs++;
      return n_cumul_hgs;
    }

    void dec_active_count() {
      n_hgs--;
    }

    unsigned get_cumulative_count() {
      return n_cumul_hgs;
    }
  };

  CgTracker cgTracker;
  std::mutex cgTrackerMutex;

  int get_number_of_active_graphs() {
    std::lock_guard<std::mutex> guard(cgTrackerMutex);
    return cgTracker.get_active_count();
  };

  unsigned get_current_graph_id() {
    std::lock_guard<std::mutex> guard(cgTrackerMutex);
    return cgTracker.get_cumulative_count();
  };

  Node::~Node() {}
  size_t Node::aux_storage_size() const { return 0; }

  // perform the forward/backward passes in one or multiple calls
  // TODO: This is a lot of code for something simple. Can it be shortened?
  void Node::forward(const std::vector<const Tensor*>& xs,
    Tensor& fx) const {
    if (this->supports_multibatch() || fx.d.batch_elems() == 1) {
      forward_impl(xs, fx);
    }
    else {
      size_t i;
      std::vector<Tensor> xs_elems(xs.size());
      std::vector<const Tensor*> xs_ptrs(xs.size());
      std::vector<size_t> xs_sizes(xs.size());
      for (i = 0; i < xs.size(); ++i) {
        xs_elems[i] = xs[i]->batch_elem(0);
        xs_ptrs[i] = &xs_elems[i];
        xs_sizes[i] = xs_elems[i].d.size();
      }
      Tensor fx_elem(fx.batch_elem(0));
      size_t fx_size = fx_elem.d.size();
      forward_impl(xs_ptrs, fx_elem);
      for (unsigned b = 1; b < fx.d.batch_elems(); ++b) {
        for (i = 0; i < xs.size(); ++i)
          if (xs[i]->d.bd > 1)
            xs_elems[i].v += xs_sizes[i];
        fx_elem.v += fx_size;
        forward_impl(xs_ptrs, fx_elem);
      }
    }
  }

  void Node::backward(const std::vector<const Tensor*>& xs,
    const Tensor& fx,
    const Tensor& dEdf,
    unsigned xs_i,
    Tensor& dEdxi) const {
    if (this->supports_multibatch() || fx.d.batch_elems() == 1) {
      backward_impl(xs, fx, dEdf, xs_i, dEdxi);
    }
    else {
      size_t i;
      std::vector<Tensor> xs_elems(xs.size());
      std::vector<const Tensor*> xs_ptrs(xs.size());
      std::vector<size_t> xs_sizes(xs.size());
      for (i = 0; i < xs.size(); ++i) {
        xs_elems[i] = xs[i]->batch_elem(0);
        xs_ptrs[i] = &xs_elems[i];
        xs_sizes[i] = xs_elems[i].d.size();
      }
      Tensor fx_elem(fx.batch_elem(0));
      size_t fx_size = fx_elem.d.size();
      Tensor dEdf_elem(dEdf.batch_elem(0));
      size_t dEdf_size = dEdf_elem.d.size();
      Tensor dEdxi_elem(dEdxi.batch_elem(0));
      size_t dEdxi_size = dEdxi_elem.d.size();
      backward_impl(xs_ptrs, fx_elem, dEdf_elem, xs_i, dEdxi_elem);
      for (unsigned b = 1; b < fx.d.batch_elems(); ++b) {
        for (i = 0; i < xs.size(); ++i)
          if (xs[i]->d.bd > 1)
            xs_elems[i].v += xs_sizes[i];
        fx_elem.v += fx_size;
        dEdf_elem.v += dEdf_size;
        if (dEdxi.d.bd > 1)
          dEdxi_elem.v += dEdxi_size;
        backward_impl(xs_ptrs, fx_elem, dEdf_elem, xs_i, dEdxi_elem);
      }
    }
  }

  void Node::autobatch_reshape_concatonly(const ComputationGraph & cg,
    const std::vector<VariableIndex> & batch_ids,
    const std::vector<int> & concat,
    std::vector<const Tensor*>& xs,
    Tensor& fx) const {
    size_t bid = 0;
    for (auto vid : batch_ids)
      bid += cg.nodes[vid]->dim.bd;
    const Node* exemplar = cg.nodes[batch_ids[0]];
    fx.d = exemplar->dim; fx.d.bd = bid;
    for (size_t i = 0; i < xs.size(); ++i) {
      const_cast<Tensor*>(xs[i])->d = cg.nodes[exemplar->args[i]]->dim;
      if (concat[i])
        const_cast<Tensor*>(xs[i])->d.bd = bid;
    }
  }

  ComputationGraph::ComputationGraph() : ComputationGraph(autobatch_flag) { }

  ComputationGraph::ComputationGraph(bool batched) {
    {
      std::lock_guard<std::mutex> guard(cgTrackerMutex);
      if (cgTracker.get_active_count() > 0) {
        std::cerr << "Memory allocator assumes only a single ComputationGraph at a time.\n";
        throw std::runtime_error("Attempted to create >1 CG");
      }
      graph_id = cgTracker.inc_active_count();
      std::cout << "Computation Graph " << graph_id << " was created." << std::endl;
    }
    if (batched)
      ee.reset(new BatchedExecutionEngine(*this));
    else
      ee.reset(new SimpleExecutionEngine(*this));
    immediate_compute = false;
    check_validity = false;
  }

  ComputationGraph::~ComputationGraph() {
    {
      std::lock_guard<std::mutex> guard(cgTrackerMutex);
      cgTracker.dec_active_count();
      std::cout << "Computation Graph " << graph_id << " was destroyed." << std::endl;
    }
    this->clear();
  }

  bool ComputationGraph::is_stale() {
    std::lock_guard<std::mutex> guard(cgTrackerMutex);

    // This needs to change if have more than one CG.
    return cgTracker.get_active_count() != 1 || graph_id != cgTracker.get_cumulative_count();
  }

  void ComputationGraph::clear() {
    parameter_nodes.clear();
    for (auto n : nodes) {
      // Make sure we never use a stale ComputationGraph.
      n->set_cg(nullptr);
      delete n;
    }
    nodes.clear(); // This should not be necessary.
    ee->invalidate();
  }

  VariableIndex ComputationGraph::add_function_node(Node *node) {
    Device* device = node->device;
    if (device == nullptr)
      device = node->arity() > 0 ? nodes[node->args[0]]->device : dynet::default_device;
    if (device->type == DeviceType::GPU && !node->has_cuda_implemented)
      DYNET_NO_CUDA_IMPL_ERROR(node->as_dummy_string())
    return add_node(node, device);
  }

  CGCheckpoint ComputationGraph::_get_checkpoint() {
    CGCheckpoint p;
    p.node_idx = nodes.size();
    p.par_node_idx = parameter_nodes.size();
    p.device_mem_checkpoint = default_device->mark(this);
    return p;
  }

  void ComputationGraph::_revert(CGCheckpoint p) {
    default_device->revert(p.device_mem_checkpoint);
    // clear all nodes at position >= p.node_idx
    if ((int)nodes.size() > p.node_idx) {
      for (int i = p.node_idx; i < (int)nodes.size(); i++)
        delete nodes[i]; // the deletion of nodes.
      nodes.resize(p.node_idx);
      ee->invalidate(p.node_idx - 1); // clear precomputed forward values
    }
    // clear all parameter nodes at position >= p.par_node_idx
    if ((int)parameter_nodes.size() > p.par_node_idx) {
      parameter_nodes.resize(p.par_node_idx);
    }
  }

  void ComputationGraph::checkpoint() {
    checkpoints.push_back(_get_checkpoint());
  }

  void ComputationGraph::revert() {
    if (checkpoints.empty()) return;
    _revert(checkpoints.back());
    checkpoints.pop_back();
  }

  Dim& ComputationGraph::get_dimension(VariableIndex index) const {
    return nodes[index]->dim;
  }

  VariableIndex ComputationGraph::add_node(Node* node, Device* device) {
    VariableIndex new_node_index(nodes.size());
    node->device = device;
    nodes.push_back(node);
    set_dim_for_new_node(new_node_index);
    return new_node_index;
  }

  VariableIndex ComputationGraph::add_input(real s, Device *device) {
    ScalarInputNode* new_node = new ScalarInputNode(s);
    return add_node(new_node, device);
  }

  VariableIndex ComputationGraph::add_input(const real* ps, Device *device) {
    ScalarInputNode* new_node = new ScalarInputNode(ps);
    return add_node(new_node, device);
  }

  VariableIndex ComputationGraph::add_input(const Dim& d, const std::vector<float>& pm, Device *device) {
    InputNode* new_node = new InputNode(d, pm);
    return add_node(new_node, device);
  }

  VariableIndex ComputationGraph::add_input(const Dim& d, const std::vector<float>* pm, Device *device) {
    InputNode* new_node = new InputNode(d, pm);
    return add_node(new_node, device);
  }

  VariableIndex ComputationGraph::add_input(const Dim& d, const std::vector<unsigned int>& ids,
    const std::vector<float>& data, Device *device, float defdata) {
    SparseInputNode* new_node = new SparseInputNode(d, ids, data, defdata);
    return add_node(new_node, device);
  }

  VariableIndex ComputationGraph::add_parameter_node(Node* node, Device* device) {
    VariableIndex new_node_index(add_node(node, device));
    parameter_nodes.push_back(new_node_index);
    return new_node_index;
  }

  VariableIndex ComputationGraph::add_parameters(Parameter p) {
    ParameterNode* new_node = new ParameterNode(p);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_parameters(LookupParameter p) {
    ParameterNode* new_node = new ParameterNode(p);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_parameters(Parameter p) {
    ConstParameterNode* new_node = new ConstParameterNode(p);
    return add_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_parameters(LookupParameter p) {
    ConstParameterNode* new_node = new ConstParameterNode(p);
    return add_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_lookup(LookupParameter p, const unsigned* pindex) {
    LookupNode* new_node = new LookupNode(p, pindex);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_lookup(LookupParameter p, unsigned index) {
    LookupNode* new_node = new LookupNode(p, index);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_lookup(LookupParameter p, const std::vector<unsigned>& indices) {
    LookupNode* new_node = new LookupNode(p, indices);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_lookup(LookupParameter p, const std::vector<unsigned>* indices) {
    LookupNode* new_node = new LookupNode(p, indices);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_lookup(LookupParameter p, const unsigned* pindex) {
    // Get rid of the following in favor of using parameter_nodes to see the needs_derivative expression.
    LookupNode* new_node = new LookupNode(p, pindex);
    return add_parameter_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_lookup(LookupParameter p, unsigned index) {
    LookupNode* new_node = new LookupNode(p, index);
    return add_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_lookup(LookupParameter p, const std::vector<unsigned>& indices) {
    LookupNode* new_node = new LookupNode(p, indices);
    return add_node(new_node, p.get_storage().device);
  }

  VariableIndex ComputationGraph::add_const_lookup(LookupParameter p, const std::vector<unsigned>* indices) {
    LookupNode* new_node = new LookupNode(p, indices);
    return add_node(new_node, p.get_storage().device);
  }

  // factory function should call this right after creating a new node object
  // to set its dimensions properly
  void ComputationGraph::set_dim_for_new_node(const VariableIndex& i) {
    if (i < 0 || nodes.size() <= i) {
      std::cerr << "Bad index in ComputationGraph::set_dim_for_new_node = " << i << std::endl;
      std::cin.get();
      std::runtime_error("Bad index in set_dim_for_new_node");
    }
    Node* node = nodes[i];
    std::vector<Dim> xds(node->arity());
    unsigned ai = 0;
    for (VariableIndex arg : node->args) {
      xds[ai] = nodes[arg]->dim;
      ++ai;
    }
    node->dim = node->dim_forward(xds);
    node->set_cg(this);
    if (immediate_compute) {
      const Tensor& value = incremental_forward(i);
      if (check_validity)
        if (!value.is_valid()) {
          std::cerr << "NaN or Inf detected\n";
          throw std::runtime_error("NaN or Inf detected");
        }
    }
  }

  const Tensor& ComputationGraph::incremental_forward(const Expression& last) { return ee->incremental_forward(last.i); }
  const Tensor& ComputationGraph::forward(const Expression& last) { return ee->forward(last.i); }
  const Tensor& ComputationGraph::incremental_forward(VariableIndex last) { return ee->incremental_forward(last); }
  const Tensor& ComputationGraph::forward(VariableIndex last) { return ee->forward(last); }
  const Tensor& ComputationGraph::get_value(VariableIndex i) { return ee->get_value(i); }
  const Tensor& ComputationGraph::get_value(const Expression& e) { return this->get_value(e.i); }
  const Tensor& ComputationGraph::get_gradient(VariableIndex i) { return ee->get_gradient(i); }
  const Tensor& ComputationGraph::get_gradient(const Expression& e) { return this->get_gradient(e.i); }
  void ComputationGraph::invalidate() { ee->invalidate(); }
  void ComputationGraph::backward(const Expression& last, bool full) { ee->backward(last.i, full); }
  void ComputationGraph::backward(VariableIndex i, bool full) { ee->backward(i, full); }

  void ComputationGraph::set_immediate_compute(bool ic) {
    immediate_compute = ic;
  }

  void ComputationGraph::set_check_validity(bool cv) {
    check_validity = cv;
  }

  void ComputationGraph::print_graphviz() const {
    std::cerr << "digraph G {\n  rankdir=LR;\n  nodesep=.05;\n";
    unsigned nc = 0;
    for (auto node : nodes) {
      std::vector<std::string> var_names;
      for (auto arg : node->args)
        var_names.push_back(std::string("v") + std::to_string((unsigned)arg));
      std::cerr << "  N" << nc << " [label=\"v" << nc << " = "
        << node->as_string(var_names);
      if (profiling_flag) {
        std::cerr << " (MEM: ";
        // if inplaced, note that no memory is used
        if (node->forward_inplaced()) {
          if (node->backward_inplaced()) {
            std::cerr << "         0 KiB (forward/backward inplaced))";
          }
          else {
            std::cerr << std::setprecision(4) << std::setw(11) << node->dim.size() * sizeof(real) / 1024.0
              << " KiB (forward inplaced))";
          }
        }
        else {
          std::cerr << std::setprecision(4) << std::setw(11) << (node->aux_storage_size() + 2 * node->dim.size() * sizeof(real)) / 1024.0
            << " KiB)";
        }
      }
      std::cerr << "\"];\n";
      for (auto arg : node->args)
        std::cerr << "  N" << ((unsigned)arg) << " -> N" << nc << ";\n";
      ++nc;
    }
    std::cerr << "}\n";

    if (profiling_flag > 1) {
      std::cerr << "\nAggregated nodes, sorted by memory consumption:\n";
      std::map<std::string, unsigned> nodes_map, count_map;
      double total_memory = 0;
      for (auto node : nodes) {
        unsigned mem = node->aux_storage_size() + 2 * node->dim.size() * sizeof(real);
        if (nodes_map.count(node->as_dummy_string()) == 0) {
          nodes_map[node->as_dummy_string()] = 0;
          count_map[node->as_dummy_string()] = 0;
        }
        nodes_map[node->as_dummy_string()] += mem;
        count_map[node->as_dummy_string()] += 1;
        total_memory += mem;
      }
      std::multimap<unsigned, std::string> nodes_map_dst = flip_map(nodes_map);
      for (auto &item : nodes_map_dst) {
        std::cerr << std::setprecision(4) << std::setw(11) << (item.first / 1024.0) << " KiB\t" << (100.0*(double)item.first / total_memory) << "%\t" << "called " << count_map[item.second] << "x\t" << item.second << std::endl;
      }
      std::cerr << std::setprecision(4) << std::setw(11) << (total_memory / 1024.0) << " KiB\t100%\t(total)" << std::endl;
      show_pool_mem_info();
    }
  }

}  // namespace dynet
