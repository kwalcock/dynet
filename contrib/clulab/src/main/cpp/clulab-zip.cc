#include <algorithm> // std::min
#include <cstdlib> // atof
#include <limits> // std::numeric_limits
#include <memory> // unique_ptr

#include <dynet/except.h>

#include "clulab-zip.h"

namespace dynet {

ZipReader::ZipReader(const std::string filename_, const std::string zipname_) : filename(filename_),
  zipname(zipname_), name(zipname + ":" + filename), zipFile(0), eof(false), fail(false) {
 zipFile = unzOpen64(zipname.c_str());
 reset();
}

void ZipReader::reset() {
 std::cout << "filename: " << filename << std::endl;
 std::cout << "filename: " << filename.c_str() << std::endl;
 int test;
 std::cin >> test;
 fail = !(
 zipFile != 0 &&
  unzLocateFile(zipFile, filename.c_str(), 1) == UNZ_OK &&
  unzOpenCurrentFile(zipFile) == UNZ_OK
  );
}

ZipReader::~ZipReader() {
 if (zipFile != 0)
  unzClose(zipFile);
 zipFile = 0;
}

void ZipReader::skip(size_t count, int allocSize, char * buffer) {
 while (count > 0) {
  int readSize = std::min(count, static_cast<size_t>(allocSize));
  int readResult = unzReadCurrentFile(zipFile, buffer, count);
  // <0 is negative of error code.
  //  0 is eof, which isn't expected.
  // >0 is OK, but here we're insisting on filling the entire buffer.
  if (readResult != readSize) {
   fail = true;
   DYNET_RUNTIME_ERR("Could not skip ahead in " << getName());
  }
  count -= readSize;
 }
}

bool ZipReader::operator!() {
 return fail || eof;
}

const std::string & ZipReader::getName() {
 return name;
}

void ZipReader::skip(size_t count) {
 const int staticSize = 1024;
 if (count <= staticSize) {
  char buffer[staticSize];
  skip(count, staticSize, buffer);
 }
 else {
  // Find the maximum number that can be read with a reliable return value.
  const int maxAllocSize = std::min(count, static_cast<size_t>(std::numeric_limits<int>::max()));
  const int allocSize = std::min(maxAllocSize, 1024 * 1024);
  std::unique_ptr<char[]> buffer(new char[allocSize]);
  skip(count, allocSize, buffer.get());
 }
}

bool ZipReader::getLine(std::string & line) {
 char buffer;
 line.clear();
 while (true) {
  int readResult = unzReadCurrentFile(zipFile, &buffer, 1);
  if (readResult == 0) {
   eof = true;
   break;
  }
  else if (readResult < 0) {
   fail = true;
   DYNET_RUNTIME_ERR("Could not read line from " << getName());
  }
  else if (buffer == '\n')
   break;
  line.append(1, buffer);
 }
 return !eof;
}

void ZipReader::getFloats(std::vector<float> & values) {
 std::vector<float>::size_type count = values.size();
 // This is pedantic, but see also skip().  This will include the trailing space.
 const int allocSize = FLOAT32_WIDTH;
 const int readSize = allocSize;
 char buffer[allocSize];
 for (std::vector<float>::size_type i = 0; i < count; i++) {
  int readResult = unzReadCurrentFile(zipFile, buffer, readSize);
  if (readResult != readSize) {
   fail = true;
   DYNET_RUNTIME_ERR("Could not read floats from " << getName());
  }
  buffer[allocSize - 1] = '\0';
  values[i] = atof(buffer);
 }
 skip(1); // Finish out the line, which is only the eol in this case.
}

ZipFileLoader::ZipFileLoader(const std::string & filename, const std::string &zipname) :
  BaseFileLoader(), filename(filename), zipname(zipname), zipReader(filename, zipname) { }

ZipFileLoader::~ZipFileLoader() {}

void ZipFileLoader::populate(ParameterCollection & model, const std::string & key) {
 zipReader.reset();
 basePopulate(zipReader, model, key);
}

void ZipFileLoader::populate(Parameter & param, const std::string & key) {
 zipReader.reset();
 basePopulate(zipReader, param, key);
}

void ZipFileLoader::populate(LookupParameter & lookup_param, const std::string & key) {
 zipReader.reset();
 basePopulate(zipReader, lookup_param, key);
}

Parameter ZipFileLoader::load_param(ParameterCollection & model, const std::string & key) {
 zipReader.reset();
 return baseLoadParam(zipReader, model, key);
}

LookupParameter ZipFileLoader::load_lookup_param(ParameterCollection & model, const std::string & key) {
 zipReader.reset();
 return baseLoadLookupParam(zipReader, model, key);
}

} // namespace dynet
