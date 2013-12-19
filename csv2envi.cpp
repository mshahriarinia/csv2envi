#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdint.h>

void checkArgs(int argc, char* argv[]);
void writeBinary(char* fileProperties[]);
void writeHeader(char* fileProperties[]);

using namespace std;

int main(int argc, char* argv[])
{
  checkArgs(argc, argv);
  writeBinary(argv);
  writeHeader(argv);
  return 0;
}

/**
 * Checks the format of the input arguments.
 * @param argc Argument count
 * @param argv Argument vector
 */
void checkArgs(int argc, char* argv[])
{
  bool argsOk = true;
  
  if (argc == 9)
  {
    int lines = atoi(argv[3]);
    int samples = atoi(argv[4]);
    int bands = atoi(argv[5]);
    int precision = atoi(argv[6]);
    string interleave = argv[7];
    int byteOrder = atoi(argv[8]);
    
    if (lines < 1 || samples < 1 || bands < 1) argsOk = false;
    else if ((precision < 1 || precision > 5)
             && precision != 12) argsOk = false;
    else if (interleave.compare("bsq") != 0 && interleave.compare("bil") != 0
             && interleave.compare("bip") != 0) argsOk = false;
    else if (byteOrder < 0 || byteOrder > 1) argsOk = false;
  }
  else argsOk = false;
  
  if (argsOk == false)
  {
    cout << "Error: improper program arguments." << endl;
    cout << endl;
    
    cout << "arguments" << endl;
    cout << "---------" << endl;
    cout << "inputFilePath     full path to input file" << endl;
    cout << "outputFilePath    full path to output file" << endl;
    cout << "lines             number of lines, int > 0" << endl;
    cout << "samples           number of samples, int > 0" << endl;
    cout << "bands             number of bands, int > 0" << endl;
    cout << "precision         1 (1-byte unsigned integer)" << endl;
    cout << "                  2 (2-byte signed integer)" << endl;
    cout << "                  3 (4-byte signed integer)" << endl;
    cout << "                  4 (4-byte float)" << endl;
    cout << "                  5 (8-byte double)" << endl;
    cout << "                  12 (2-byte unsigned integer)" << endl;
    cout << "interleave        bsq (band sequential)" << endl;
    cout << "                  bil (band interleave by line)" << endl;
    cout << "                  bip (band interleave by pixel)" << endl;
    cout << "byteOrder         0 (little-endian)" << endl;
    cout << "                  1 (big-endian)" << endl;
    cout << endl;
    
    exit(1);
  }
  
  return;
}

/**
 * Writes the binary file according to the ENVI image properties.
 * @param fileProperties ENVI image properties
 */
void writeBinary(char* fileProperties[])
{
  string inputFilePath = fileProperties[1];
  string outputFilePath = fileProperties[2];
  int lines = atoi(fileProperties[3]);
  int samples = atoi(fileProperties[4]);
  int bands = atoi(fileProperties[5]);
  int precision = atoi(fileProperties[6]);
  string interleave = fileProperties[7];
  int byteOrder = atoi(fileProperties[8]);  
  
  // Strip off extension from output file name.
  int dot = outputFilePath.find_last_of(".");
  if (dot != string::npos) outputFilePath = outputFilePath.substr(0, dot);
  
  // Allocate memory for reading in the csv file.
  char* buffer = NULL;
  int bufferSize;
  switch(precision)
  {
    case 1:  // 1-byte unsigned integer
      bufferSize = lines * samples * bands; break;
      
    case 2:  // 2-byte signed integer
      bufferSize = 2 * lines * samples * bands; break;
      
    case 3:  // 4-byte signed integer
      bufferSize = 4 * lines * samples * bands; break;
      
    case 4:  // 4-byte float
      bufferSize = 4 * lines * samples * bands; break;
      
    case 5:  // 8-byte double
      bufferSize = 8 * lines * samples * bands; break;
      
    case 12: // 2-byte unsigned integer
      bufferSize = 2 * lines * samples * bands; break;
  }
  buffer = (char*) malloc(bufferSize);
  
  // Failed to allocate memory.
  if (buffer == NULL)
  {
    cout << "Failed to allocate memory for buffer." << endl;
    exit(1);
  }
  
  // Read input file into buffer according to the interleave.
  // Discard first line, the attribute name.
  int position;
  char line[100];
  ifstream inputFile(inputFilePath.c_str(), ios::in);
  inputFile.getline(line, 100);
  
  for (int i = 0; i < lines; ++i) {
    for (int j = 0; j < samples; ++j) {
      for (int k = 0; k < bands; ++k)
      {
        inputFile.getline(line, 100);
        
        // Determine where to place the array values in the buffer.
        if (interleave.compare("bsq") == 0)
          position = i * samples + j + k * lines * samples;
        else if (interleave.compare("bil") == 0)
          position = i * samples * bands + j + k * samples;
        else  /* bip */
          position = i * samples * bands + j * bands + k;
        
        // Write to buffer.
        // byteOrder == 0 is little endian
        // byteOrder == 1 is big endian
        switch(precision)
        {
          case 1:  // 1-byte unsigned integer
            buffer[position] = (uint8_t) atoi(line);
            break;
            
          case 2:  // 2-byte signed integer
            if (byteOrder == 0)
              for (int l = 0; l < 2; ++l)
                buffer[2 * position + l] = (int16_t) atoi(line) >> 8 * l;
            else
              for (int l = 0; l < 2; ++l)
                buffer[2 * position + l] = (int16_t) atoi(line) >> 8 * (1 - l);
            break;
            
          case 3:  // 4-byte signed integer
            if (byteOrder == 0)
              for (int l = 0; l < 4; ++l)
                buffer[4 * position + l] = (int32_t) atoi(line) >> 8 * l;
            else
              for (int l = 0; l < 4; ++l)
                buffer[4 * position + l] = (int32_t) atoi(line) >> 8 * (3 - l);
            break;
            
          case 4:  // 4-byte float
            {
            float f = (float) atof(line);
            unsigned char *c = reinterpret_cast<unsigned char *>(&f);
            
            if (byteOrder == 0)
              for (int l = 0; l < 4; ++l)
                buffer[4 * position + l] = c[3 - l];
            else
              for (int l = 0; l < 4; ++l)
                buffer[4 * position + l] = c[l];
            }
            break;
            
          case 5:  // 8-byte double
            {
            double d = (double) atof(line);
            unsigned char *c = reinterpret_cast<unsigned char *>(&d);
            
            if (byteOrder == 0)
              for (int l = 0; l < 4; ++l)
                buffer[8 * position + l] = c[7 - l];
            else
              for (int l = 0; l < 4; ++l)
                buffer[8 * position + l] = c[l];
            }
            break;
            
          case 12: // 2-byte unsigned integer
            if (byteOrder == 0)
              for (int l = 0; l < 2; ++l)
                buffer[2 * position + l] = (uint16_t) atoi(line) >> 8 * l;
            else
              for (int l = 0; l < 2; ++l)
                buffer[2 * position + l] = (uint16_t) atoi(line) >> 8 * (1 - l);
            break;
        }
      }
    }
  }
  
  // Write buffer to file.
  outputFilePath += "." + interleave;
  ofstream outputFile(outputFilePath.c_str(), ios::binary);
  outputFile.write(buffer, bufferSize);
  
  // Clean up.
  inputFile.close();
  outputFile.close();
  free(buffer);
}

/**
 * Writes the header file corresponding to the binary file.
 * @param fileProperties ENVI image properties
 */
void writeHeader(char* fileProperties[])
{
  string outputFilePath = fileProperties[2];
  string lines = fileProperties[3];
  string samples = fileProperties[4];
  string bands = fileProperties[5];
  string precision = fileProperties[6];
  string interleave = fileProperties[7];
  string byteOrder = fileProperties[8];
  
  // Strip off extension from output file name.
  int dot = outputFilePath.find_last_of(".");
  if (dot != string::npos) outputFilePath = outputFilePath.substr(0, dot);
  
  outputFilePath += ".hdr";
  ofstream outputFile(outputFilePath.c_str(), ios::out);
  
  outputFile << "ENVI" << endl;
  outputFile << "lines = " << lines << endl;
  outputFile << "samples = " << samples << endl;
  outputFile << "bands = " << bands << endl;
  outputFile << "data type = " << precision << endl;
  outputFile << "interleave = " << interleave << endl;
  outputFile << "byte order = " << byteOrder << endl;
  
  outputFile.close();
}

