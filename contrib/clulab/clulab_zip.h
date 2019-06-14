#ifndef CLULAB_ZIP_H_
#define CLULAB_ZIP_H_

#include <string> // std::string
#include <vector> // std::vector

#include <dynet/io.h>
#include <unzip.h>

namespace clulab {

using namespace dynet;

class ZipReader : public DataReader {
 protected:
  const std::string & filename;
  const std::string & zipname;
  const std::string & name;
  unzFile zipFile;
  bool eof;
  bool fail;

  void skip(size_t count, int allocSize, char * buffer);

 public:
  ZipReader(const std::string & filename, const std::string & zipname);
  ~ZipReader();
  bool operator!();
  const std::string & getName();
  void skip(size_t count);
  bool getLine(std::string & line);
  void getFloats(std::vector<float> & values);
};

class ZipFileLoader : public BaseFileLoader {
 public:
  ZipFileLoader(const std::string & filename, const std::string & zipname);
  ~ZipFileLoader() override;
  void populate(ParameterCollection & model, const std::string & key = "") override;
  void populate(Parameter & param, const std::string & key = "") override;
  void populate(LookupParameter & lookup_param, const std::string & key = "") override;
  Parameter load_param(ParameterCollection & model, const std::string & key) override;
  LookupParameter load_lookup_param(ParameterCollection & model, const std::string & key) override;

 private:
  std::string filename;
  std::string zipname;
}; // class ZipFileLoader

} // namespace dynet

#endif
