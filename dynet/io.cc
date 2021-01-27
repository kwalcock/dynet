#include "dynet/mem_debug.h"
#include "dynet/io.h"
#include "dynet/tensor.h"
#include "dynet/except.h"
#include "dynet/str-util.h"

#include <algorithm> // find_if

// Normally DyNet style permits using namespace std, but to make compatibility
// possible with some external code, it is simpler if types are fully
// qualified in dynet/io.cc. Please do not uncomment the following:

// using namespace std;  // DO NOT UNCOMMENT

namespace dynet {

namespace {

bool valid_key(const std::string & s) {
  if (s.size() == 0) return true;
  if (s == "/") return false;
  auto it = std::find_if(s.begin(), s.end(),
                         [] (char ch) { return ch == ' ' || ch == '#';});
  return it == s.end();
}

bool valid_pc_key(const std::string & s) {
  if (s.size() == 0) return true;
  if (!(startswith(s, "/"))) return false;
  return valid_key(s);
}

bool grad_is_zero(const ParameterStorageBase & p){
  return !p.has_grad();
}

void read_param_header(std::string line, std::string &type, std::string &name, Dim& dim,size_t& byte_count, bool& zero_grad){
  // Read header
  std::istringstream iss(line);
  iss >> type >> name >> dim >> byte_count;
  // Check whether gradient is 0
  // Check for EOF (for backward compatibility)
  std::string grad;
  if (!iss.eof()){
    iss >> grad;
    if (grad == "ZERO_GRAD")
      zero_grad = true;
    else
      zero_grad = false;
  }
}

} // anyonymous namespace

Saver::~Saver() {}
Loader::~Loader() {}

TextFileSaver::TextFileSaver(const std::string & filename, bool append) :
        p_datastream(
            DYNET_NEW(std::ofstream(
                filename.c_str(),
                (append ? std::ios_base::app : std::ios_base::out) | std::ios_base::binary))),
        datastream(*p_datastream) {
  if(!datastream)
    DYNET_RUNTIME_ERR("Could not write model to " << filename);
  datastream.precision(FLOAT32_PRECISION);
  datastream << std::scientific << std::showpos;
}

TextFileSaver::~TextFileSaver() {}

void TextFileSaver::save(const ParameterCollection & model,
                         const std::string & key) {
  if (!valid_pc_key(key))
    DYNET_INVALID_ARG("Key should start with '/' and could not include ' ' or '#': " << key);
  std::string key_ = key;
  if (key_.size() != 0 && key_.back() != '/') key_ += "/";
  const ParameterCollectionStorage & storage = model.get_storage();
  if(key.size() == 0) {
    for (auto & p : storage.params) save(*p, key);
    for (auto & p : storage.lookup_params) save(*p, key);
  } else {
    size_t strip_size = model.get_fullname().size();
    for (auto & p : storage.params) 
      save(*p, key_ + p->name.substr(strip_size));
    for (auto & p : storage.lookup_params) 
      save(*p, key_ + p->name.substr(strip_size));
  }
}

void TextFileSaver::save(const Parameter & param,
                         const std::string & key) {
  if (!valid_key(key))
    DYNET_INVALID_ARG("Key could not include ' ' or '#': " << key);
  save(*param.p, key);
}

void TextFileSaver::save(const LookupParameter & param,
                         const std::string & key) {
  if (!valid_key(key))
    DYNET_INVALID_ARG("Key could not include ' ' or '#': " << key);
  save(*param.p, key);
}

void TextFileSaver::save(const ParameterStorage & p,
                         const std::string & key) {
  datastream << "#Parameter# " << (key.size() > 0 ? key : p.name) << ' ' << p.dim << ' ';
  // We additionally add a newline at the end of the line, so the total size is as below.
  size_t strsize = static_cast<size_t>(p.dim.size()) * FLOAT32_WIDTH + 1;
  bool zero_grad = grad_is_zero(p);
  if(zero_grad)
    datastream << strsize << " ZERO_GRAD";
  else
    datastream << strsize*2 << " FULL_GRAD";
  datastream << std::endl << dynet::as_scale_vector(p.values, p.owner->get_weight_decay().current_weight_decay()) << std::endl;
  if(!zero_grad)
    datastream << dynet::as_vector(p.g) << std::endl;
}

void TextFileSaver::save(const LookupParameterStorage & p,
                         const std::string & key) {
  datastream << "#LookupParameter# " << (key.size() > 0 ? key : p.name) << ' ' << p.all_dim << ' ';
  size_t strsize = static_cast<size_t>(p.all_dim.size()) * FLOAT32_WIDTH + 1;
  bool zero_grad = grad_is_zero(p);
  if(zero_grad)
    datastream << strsize << " ZERO_GRAD";
  else
    datastream << strsize*2 << " FULL_GRAD";
  datastream << std::endl << dynet::as_scale_vector(p.all_values, p.owner->get_weight_decay().current_weight_decay()) << std::endl;
  if(!zero_grad)
    datastream << dynet::as_vector(p.all_grads) << std::endl;
}

DataReader::~DataReader() { }

class StreamReader : public DataReader {
 protected:
  std::ifstream stream;
  const std::string & filename;

 public:
  StreamReader(const std::string & filename) :
    stream(filename, std::ios_base::in | std::ios_base::binary), filename(filename) {
  }

  ~StreamReader() {
   stream.close(); // Won't this be done automatically?
  }

  bool operator!() {
   return !stream;
  }

  const std::string & getName() {
   return filename;
  }

  void skip(size_t count) {
   stream.seekg(count, std::ios_base::cur);
   if (stream.bad())
    DYNET_RUNTIME_ERR("Could not skip ahead in " << getName());
  }

  bool getLine(std::string & line) {
   std::getline(stream, line);
   if (stream.bad())
    DYNET_RUNTIME_ERR("Could not read line from " << getName());
   // EOF should only be encountered in this method and when there is simply no next line.
   // None of the other StreamReader methods should encounter eof unless there is a problem.
   return !stream.eof();
  }

  void getFloats(std::vector<float> & values) {
   // This next line doesn't work because of the trailing /n.  The reading seems not to automatically
   // stop after reading values.size() values and will fail on the next one.  Instead, we keep count.
   //stream >> values;
   std::vector<float>::size_type count = values.size();
   for (std::vector<float>::size_type i = 0; i < count; i++) {
    stream >> values[i];
    if (stream.bad())
     DYNET_RUNTIME_ERR("Could not read floats from " << getName());
   }
   skip(2); // Finish out the line.
  }
};

BaseFileLoader::~BaseFileLoader() { }

void BaseFileLoader::basePopulate(DataReader & dataReader, ParameterCollection & model, const std::string & key = "") {
  if(!dataReader) DYNET_RUNTIME_ERR("Could not read model from " << dataReader.getName());
  std::string line, type, name; // Know how much to expect of line
  bool zero_grad = false;
  Dim dim;
  size_t byte_count = 0;
  std::vector<float> values;
  Tensor *value_t, *grad_t;
  size_t param_id = 0, lookup_id = 0;
  ParameterCollectionStorage & storage = model.get_storage();
  std::string key_ = key;
  if (key_.size() != 0 && key_.back() != '/') key_ += "/";
  while(dataReader.getLine(line)) {
    read_param_header(line, type, name, dim, byte_count, zero_grad);
    // Skip ones that don't match
    if(key.size() != 0 && name.substr(0, key_.size()) != key_) {
      dataReader.skip(byte_count);
      continue;
    // Load a parameter
    } else if(type == "#Parameter#") {
      values.resize(dim.size());
      if(param_id >= storage.params.size())
        DYNET_RUNTIME_ERR("Too many parameters to load in populated model at " << name);
      ParameterStorage & param = *storage.params[param_id++];
      if(param.dim != dim)
        DYNET_RUNTIME_ERR("Dimensions of parameter " << name << " looked up from file (" << dim << 
                            ") do not match parameters to be populated (" << param.dim << ")");
      value_t = &param.values;
      grad_t = &param.g;
    // Load a lookup parameter
    } else if(type == "#LookupParameter#") {
      values.resize(dim.size());
      if(lookup_id >= storage.lookup_params.size())
        DYNET_RUNTIME_ERR("Too many lookup parameters in populated model at " << name);
      LookupParameterStorage & param = *storage.lookup_params[lookup_id++];
      if(param.all_dim != dim)
        DYNET_RUNTIME_ERR("Dimensions of lookup parameter " << name << " lookup up from file (" << dim << 
                            ") do not match parameters to be populated (" << param.all_dim << ")");
      value_t = &param.all_values;
      grad_t = &param.all_grads;
    } else {
      DYNET_RUNTIME_ERR("Bad parameter specification in model: " << line);
    }
    dataReader.getFloats(values);
    TensorTools::set_elements(*value_t, values);
    if(!zero_grad){
     dataReader.getFloats(values);
     TensorTools::set_elements(*grad_t, values);
    } else {
      TensorTools::zero(*grad_t);
    }
  }
  if(param_id != storage.params.size() || lookup_id != storage.lookup_params.size())
    DYNET_RUNTIME_ERR("Number of parameter/lookup parameter objects loaded from file (" << 
                      param_id << '/' << lookup_id << ") did not match number to be populated (" <<
                      storage.params.size() << '/' << storage.lookup_params.size() << ')');
}

void BaseFileLoader::basePopulate(DataReader & dataReader, Parameter & param, const std::string & key = "") {
  if(key == "")
    DYNET_INVALID_ARG("TextFileLoader.populate() requires non-empty key");
  if(!dataReader) DYNET_RUNTIME_ERR("Could not read model from " << dataReader.getName());
  std::string line, type, name;
  bool zero_grad=false;
  Dim dim;
  size_t byte_count = 0;
  while(dataReader.getLine(line)) {
    read_param_header(line, type, name, dim, byte_count, zero_grad);
    if(type == "#Parameter#" && name == key) {
      if(param.p->dim != dim)
        DYNET_RUNTIME_ERR("Attempted to populate parameter where arguments don't match (" << param.p->dim << " != " << dim << ")");
      std::vector<float> values(dim.size());
      dataReader.getFloats(values);
      TensorTools::set_elements(param.get_storage().values, values);
      if(!zero_grad){
        dataReader.getFloats(values);
        TensorTools::set_elements(param.get_storage().g, values);
      } else {
        TensorTools::zero(param.get_storage().g);
      }
      return;
    } else {
      dataReader.skip(byte_count);
    }
  }
  DYNET_RUNTIME_ERR("Could not find key " << key << " in the model file");
}

void BaseFileLoader::basePopulate(DataReader & dataReader, LookupParameter & lookup_param, const std::string & key = "") {
  if(key == "")
    DYNET_INVALID_ARG("TextFileLoader.populate() requires non-empty key");
  if(!dataReader) DYNET_RUNTIME_ERR("Could not read model from " << dataReader.getName());
  std::string line, type, name;
  bool zero_grad=false;
  Dim dim;
  size_t byte_count = 0;
  while(dataReader.getLine(line)) {
    read_param_header(line, type, name, dim, byte_count, zero_grad);
    if(type == "#LookupParameter#" && name == key) {
      if(lookup_param.p->all_dim != dim)
        DYNET_RUNTIME_ERR("Attempted to populate lookup parameter where arguments don't match (" << lookup_param.p->all_dim << " != " << dim << ")");
      std::vector<float> values(dim.size());
      dataReader.getFloats(values);
      TensorTools::set_elements(lookup_param.get_storage().all_values, values);
      if(!zero_grad){
        dataReader.getFloats(values);
        TensorTools::set_elements(lookup_param.get_storage().all_grads, values);
      } else {
        TensorTools::zero(lookup_param.get_storage().all_grads);
      }
      return;
    } else {
      dataReader.skip(byte_count);
    }
  }
  DYNET_RUNTIME_ERR("Could not find key " << key << " in the model file");
}

Parameter BaseFileLoader::baseLoadParam(DataReader & dataReader, ParameterCollection & model, const std::string & key) {
  if(key == "")
    DYNET_INVALID_ARG("TextFileLoader.load_param() requires non-empty key");
  if(!dataReader) DYNET_RUNTIME_ERR("Could not read model from " << dataReader.getName());
  std::string line, type, name;
  bool zero_grad=false;
  Dim dim;
  size_t byte_count = 0;
  while(dataReader.getLine(line)) {
    read_param_header(line, type, name, dim, byte_count, zero_grad);
    if(type == "#Parameter#" && name == key) {
      Parameter param = model.add_parameters(dim);
      param.get_storage().name = name;
      std::vector<float> values(dim.size());
      dataReader.getFloats(values);
      TensorTools::set_elements(param.get_storage().values, values);
      if(!zero_grad){
        dataReader.getFloats(values);
        TensorTools::set_elements(param.get_storage().g, values);
      } else {
        TensorTools::zero(param.get_storage().g);
      }
      return param;
    } else {
      dataReader.skip(byte_count);
    }
  }
  DYNET_RUNTIME_ERR("Could not find key " << key << " in the model file");
}

LookupParameter BaseFileLoader::baseLoadLookupParam(DataReader & dataReader, ParameterCollection & model, const std::string & key) {
  if(key == "")
    DYNET_INVALID_ARG("TextFileLoader.load_lookup_param() requires non-empty key");
  if(!dataReader) DYNET_RUNTIME_ERR("Could not read model from " << dataReader.getName());
  std::string line, type, name;
  bool zero_grad=false;
  Dim dim;
  size_t byte_count = 0;
  while(dataReader.getLine(line)) {
    read_param_header(line, type, name, dim, byte_count, zero_grad);
    if(type == "#LookupParameter#" && name == key) {
      std::vector<float> values(dim.size());
      size_t size = dim[dim.nd-1]; dim.nd--;
      LookupParameter lookup_param = model.add_lookup_parameters(size, dim);
      lookup_param.get_storage().name = name;
      dataReader.getFloats(values);
      TensorTools::set_elements(lookup_param.get_storage().all_values, values);
      if(!zero_grad){
        dataReader.getFloats(values);
        TensorTools::set_elements(lookup_param.get_storage().all_grads, values);
      } else {
        TensorTools::zero(lookup_param.get_storage().all_grads);
      }
      return lookup_param;
    } else {
      dataReader.skip(byte_count);
    }
  }
  DYNET_RUNTIME_ERR("Could not find key " << key << " in the model file");
}

TextFileLoader::TextFileLoader(const std::string & filename) : dataname(filename) { }

TextFileLoader::~TextFileLoader() {}

void TextFileLoader::populate(ParameterCollection & model, const std::string & key) {
  StreamReader dataReader(dataname);
  basePopulate(dataReader, model, key);
}

void TextFileLoader::populate(Parameter & param, const std::string & key) {
  StreamReader dataReader(dataname);
  basePopulate(dataReader, param, key);
}

void TextFileLoader::populate(LookupParameter & lookup_param, const std::string & key) {
  StreamReader dataReader(dataname);
  basePopulate(dataReader, lookup_param, key);
}

Parameter TextFileLoader::load_param(ParameterCollection & model, const std::string & key) {
  StreamReader dataReader(dataname);
  return baseLoadParam(dataReader, model, key);
}

LookupParameter TextFileLoader::load_lookup_param(ParameterCollection & model, const std::string & key) {
  StreamReader dataReader(dataname);
  return baseLoadLookupParam(dataReader, model, key);
}

} // namespace dynet
