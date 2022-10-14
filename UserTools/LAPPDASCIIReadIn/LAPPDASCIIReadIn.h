#ifndef LAPPDASCIIReadIn_H
#define LAPPDASCIIReadIn_H

#include <string>
#include <iostream>
#include <bitset>
#include <map>
#include <fstream>
#include "Tool.h"


/**
 * \class LAPPDASCIIReadIn
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDASCIIReadIn: public Tool {


 public:

  LAPPDASCIIReadIn(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool ReadPedestals(int boardNo); ///< Read in the Pedestal Files
  bool MakePedestals(); ///< Make a Pedestal File


 private:

    ifstream DataFile;
    ifstream PedFile;
    string PedFileName1;
    string PedFileName2;
    string OutputWavLabel;
    int DoPedSubtract;
    int NChannels;
    int Nsamples;
    int TrigChannel;
    int LAPPDPSECReadInVerbosity;
    int eventNo;
    int oldLaser;
    std::map<unsigned long, vector<Waveform<double>>>* LAPPDWaveforms;
    std::map<unsigned long, vector<int>> *PedestalValues;

    double SampleSize;
    int LAPPDchannelOffset;


};


#endif
