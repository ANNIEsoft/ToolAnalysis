#ifndef LAPPDStoreReadIn_H
#define LAPPDStoreReadIn_H

#include <string>
#include <iostream>
#include <map>
#include <bitset>
#include <fstream>
#include "Tool.h"

#define NUM_CH 30
#define NUM_PSEC 5
#define NUM_SAMP 256

using namespace std;
/**
 * \class LAPPDStoreReadIn
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDStoreReadIn: public Tool {


 public:

  LAPPDStoreReadIn(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool ReadPedestals(int boardNo); ///< Read in the Pedestal Files
  bool MakePedestals(); ///< Make a Pedestal File

  int getParsedData(vector<unsigned short> buffer, int ch_start);
  int getParsedMeta(vector<unsigned short> buffer, int BoardId);


 private:

    int Nboards;
    int channel_count;
    int retval;
    long entries;
    ifstream DataFile;
    ifstream PedFile;
    string NewFileName;
    string PedFileName, PedFileNameTXT;
    string OutputWavLabel;
    string InputWavLabel;
    string BoardIndexLabel;
    int DoPedSubtract;
    int NChannels;
    int Nsamples;
    int NUM_VECTOR_DATA;
    int NUM_VECTOR_PPS;
    int TrigChannel;
    int LAPPDStoreReadInVerbosity=0;
    int eventNo;
    std::map<unsigned long, vector<int>> *PedestalValues;
    streampos dataPosition;

    double SampleSize;
    int LAPPDchannelOffset;

    //temp maps for data parsing
    std::vector<unsigned short> Raw_buffer;
    std::vector<unsigned short> Parse_buffer;
    std::vector<int> ReadBoards;
    std::map<int, vector<unsigned short>> data;
    vector<unsigned short> meta;
    vector<unsigned short> pps;

};


#endif
