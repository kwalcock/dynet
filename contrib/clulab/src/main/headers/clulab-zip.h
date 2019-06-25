#ifndef CLULAB_ZIP_H_
#define CLULAB_ZIP_H_

#include <string> // std::string
#include <vector> // std::vector

#include <dynet/io.h>
#include <unzip.h>

namespace dynet {

class ZipReader : public DataReader {
 protected:
  const std::string filename;
  const std::string zipname;
  const std::string name;
  unzFile zipFile;
  bool eof;
  bool fail;

  void skip(size_t count, int allocSize, char * buffer);
 public:
  ZipReader(const std::string filename, const std::string zipname);
  virtual ~ZipReader();
  void reset();
  bool operator!() override;
  const std::string & getName() override;
  void skip(size_t count) override;
  bool getLine(std::string & line) override;
  void getFloats(std::vector<float> & values) override;
};

class ZipFileLoader : public BaseFileLoader {
 protected:
  std::string filename;
  std::string zipname;
  ZipReader zipReader;
 public:
  ZipFileLoader(const std::string & filename, const std::string & zipname);
  virtual ~ZipFileLoader();
  void populate(ParameterCollection & model, const std::string & key = "") override;
  void populate(Parameter & param, const std::string & key = "") override;
  void populate(LookupParameter & lookup_param, const std::string & key = "") override;
  Parameter load_param(ParameterCollection & model, const std::string & key) override;
  LookupParameter load_lookup_param(ParameterCollection & model, const std::string & key) override;
}; // class ZipFileLoader

} // namespace dynet

#endif
