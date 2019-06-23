#include "dynet/io.h"
#include "dynet/lstm.h"

#include <clulab-zip.h>

using namespace std;
using namespace dynet;

const unsigned RNN_STATE_SIZE = 50;
const unsigned NONLINEAR_SIZE = 32;
const unsigned RNN_LAYERS = 1;
const unsigned CHAR_RNN_LAYERS = 1;
const unsigned CHAR_EMBEDDING_SIZE = 32;
const unsigned CHAR_RNN_STATE_SIZE = 16;

const unsigned w2iSize = 100;
const unsigned t2iSize = 230;
const unsigned c2iSize = 123;
const unsigned embeddingDim = 300;
const unsigned embeddingSize = embeddingDim + 2 * CHAR_RNN_STATE_SIZE;

class ClulabModel {
 public:
  ParameterCollection parameters;
  LookupParameter lookupParameters;
  LSTMBuilder fwBuilder;
  LSTMBuilder bwBuilder;
  Parameter H;
  Parameter O;
  LookupParameter T;
  LookupParameter charLookupParameters;
  LSTMBuilder charFwBuilder;
  LSTMBuilder charBwBuilder;
 
  ClulabModel() :
    lookupParameters(parameters.add_lookup_parameters(w2iSize, { embeddingDim })),
    fwBuilder(LSTMBuilder(RNN_LAYERS, embeddingSize, RNN_STATE_SIZE, parameters)),
    bwBuilder(LSTMBuilder(RNN_LAYERS, embeddingSize, RNN_STATE_SIZE, parameters)),
    H(parameters.add_parameters({NONLINEAR_SIZE, 2 * RNN_STATE_SIZE})),
    O(parameters.add_parameters({ t2iSize, NONLINEAR_SIZE })),
    T(parameters.add_lookup_parameters(t2iSize, { NONLINEAR_SIZE })),
    charLookupParameters(parameters.add_lookup_parameters(c2iSize, { CHAR_EMBEDDING_SIZE })),
    charFwBuilder(LSTMBuilder(CHAR_RNN_LAYERS, CHAR_EMBEDDING_SIZE, CHAR_RNN_STATE_SIZE, parameters)),
    charBwBuilder(LSTMBuilder(CHAR_RNN_LAYERS, CHAR_EMBEDDING_SIZE, CHAR_RNN_STATE_SIZE, parameters)) {
  }
};

void WriteToFile(string& filename, ParameterCollection& parameters, string& key) {
 TextFileSaver saver(filename);
 saver.save(parameters, key);
}

void ReadFromFile(string& filename, ParameterCollection& parameters, string& key) {
 TextFileLoader loader(filename);
 loader.populate(parameters, key);
}

void ReadFromZip(string& filename, string& zipname, ParameterCollection& parameters, string& key) {
 ZipFileLoader loader(filename, zipname);
 loader.populate(parameters, key);
}

void help(int& argc, char**& argv) {

}

int main() {
 int argc = 2;
 char *args[] = {
  "--dynet-seed",
  "2522620396",
  nullptr
 };
 char **argv = &args[0];
 dynet::initialize(argc, argv);

 string origFilename = "model.rnn";
 string zipFilename = "model.jar";
 string copyFromTextFilename = "modelTextCopy.rnn";
 string copyFromZipFilename = "modelZipCopy.rnn";
 string key = "/key";

 ClulabModel model;

 WriteToFile(origFilename, model.parameters, key);

 // Pause here and zip the file.

 ClulabModel copyFromText;
 ReadFromFile(origFilename, copyFromText.parameters, key);

 ClulabModel copyFromZip;
 ReadFromZip(origFilename, zipFilename, copyFromZip.parameters, key);

 WriteToFile(copyFromTextFilename, copyFromText.parameters, key);

 WriteToFile(copyFromZipFilename, copyFromZip.parameters, key);
}
