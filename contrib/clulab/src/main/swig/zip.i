#ifdef SWIG_USE_ZIP

%{
#include "clulab-zip.h"
%}

class ZipFileLoader : public BaseFileLoader {
 public:
  ZipFileLoader(const std::string & filename, const std::string & zipname);
  virtual ~ZipFileLoader() { }
  void populate(ParameterCollection & model, const std::string & key = "") override;
  void populate(Parameter & param, const std::string & key = "") override;
  void populate(LookupParameter & lookup_param, const std::string & key = "") override;
  Parameter load_param(ParameterCollection & model, const std::string & key) override;
  LookupParameter load_lookup_param(ParameterCollection & model, const std::string & key) override;
}; // class ZipFileLoader

#endif