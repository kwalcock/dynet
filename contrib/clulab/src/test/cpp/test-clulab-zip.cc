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
  H(parameters.add_parameters({ NONLINEAR_SIZE, 2 * RNN_STATE_SIZE })),
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

void WriteToFile(string& filename, ParameterCollection& parametersA, string& keyA,
  ParameterCollection& parametersB, string& keyB) {
 TextFileSaver saver(filename);
 saver.save(parametersA, keyA);
 saver.save(parametersB, keyB);
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
 const char *args[] = {
  "--dynet-seed",
  "2522620396",
  nullptr
 };
 char **argv = const_cast<char**>(&args[0]);
 dynet::initialize(argc, argv);

 string origFilenameA = "modelA.rnn";
 string origFilenameB = "modelB.rnn";
 string origFilenameAB = "modelAB.rnn";
 string zipFilenameA = "modelA.zip";
 string zipFilenameB = "modelB.zip";
 string zipFilenameAB = "modelAB.zip";
 string copyFromTextFilenameA1 = "modelTextCopyA1.rnn";
 string copyFromTextFilenameA2 = "modelTextCopyA2.rnn";
 string copyFromTextFilenameB1 = "modelTextCopyB1.rnn";
 string copyFromTextFilenameB2 = "modelTextCopyB2.rnn";
 string copyFromTextFilenameAB = "modelTextCopyAB.rnn";
 string copyFromZipFilenameA1 = "modelZipCopyA1.rnn";
 string copyFromZipFilenameA2 = "modelZipCopyA2.rnn";
 string copyFromZipFilenameB1 = "modelZipCopyB1.rnn";
 string copyFromZipFilenameB2 = "modelZipCopyB2.rnn";
 string copyFromZipFilenameAB = "modelZipCopyAB.rnn";
 string keyA = "/keyA";
 string keyB = "/keyB";

 // There are now two different models.  They should be different
 // from one another because of the random initialization.
 ClulabModel modelA;
 ClulabModel modelB;

 // They are stored both separately and together.
 WriteToFile(origFilenameA, modelA.parameters, keyA);
 WriteToFile(origFilenameB, modelB.parameters, keyB);
 WriteToFile(origFilenameAB, modelA.parameters, keyA, modelB.parameters, keyB);

 std::cout
  << "Please zip the files as such: " << std::endl
  << "  " << origFilenameA << "  -> " << zipFilenameA << std::endl
  << "  " << origFilenameB << "  -> " << zipFilenameB << std::endl
  << "  " << origFilenameAB << " -> " << zipFilenameAB << std::endl
  << "and enter a string to continue. ";

 std::string response;
 std::cin >> response;

 ClulabModel copyFromTextA1;
 ReadFromFile(origFilenameA, copyFromTextA1.parameters, keyA);
 ClulabModel copyFromTextB1;
 ReadFromFile(origFilenameB, copyFromTextB1.parameters, keyB);
 // These should be the same as their 1 counterparts.
 ClulabModel copyFromTextA2;
 ReadFromFile(origFilenameAB, copyFromTextA2.parameters, keyA);
 ClulabModel copyFromTextB2;
 ReadFromFile(origFilenameAB, copyFromTextB2.parameters, keyB);

 ClulabModel copyFromZipA1;
 ReadFromZip(origFilenameA, zipFilenameA, copyFromZipA1.parameters, keyA);
 ClulabModel copyFromZipB1;
 ReadFromZip(origFilenameB, zipFilenameB, copyFromZipB1.parameters, keyB);
 // These should be the same as their 1 counterparts.
 ClulabModel copyFromZipA2;
 ReadFromZip(origFilenameAB, zipFilenameAB, copyFromZipA2.parameters, keyA);
 ClulabModel copyFromZipB2;
 ReadFromZip(origFilenameAB, zipFilenameAB, copyFromZipB2.parameters, keyB);

 WriteToFile(copyFromTextFilenameA1, copyFromTextA1.parameters, keyA);
 WriteToFile(copyFromTextFilenameA2, copyFromTextA2.parameters, keyA);
 WriteToFile(copyFromTextFilenameB1, copyFromTextB1.parameters, keyB);
 WriteToFile(copyFromTextFilenameB2, copyFromTextB2.parameters, keyB);

 WriteToFile(copyFromZipFilenameA1, copyFromZipA1.parameters, keyA);
 WriteToFile(copyFromZipFilenameA2, copyFromZipA2.parameters, keyA);
 WriteToFile(copyFromZipFilenameB1, copyFromZipB1.parameters, keyB);
 WriteToFile(copyFromZipFilenameB2, copyFromZipB2.parameters, keyB);

 std::cout << std::endl
   << "All the A files should be the same as each other." << std::endl
   << "All the B files should be the same as each other." << std::endl
   << "The A and B files should not be the same." << std::endl
   << "Enter a string to continue. ";

 std::cin >> response;
}
