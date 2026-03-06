#ifndef LAPPDLoadTXT_H
#define LAPPDLoadTXT_H

#include <string>
#include <iostream>

#include "Tool.h"

#include <map>
#include <bitset>
#include <fstream>
#include "Tool.h"

/**
 * \class LAPPDLoadTXT
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */
class LAPPDLoadTXT : public Tool
{

public:
    LAPPDLoadTXT();                                           ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    bool ReadPedestal(int boardNo);                           ///< Read in the Pedestal Files
    void ReadData();

private:
    int LAPPDLoadTXTVerbosity;
    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    string PedFileNameTXT;
    int NBoards;
    string DataFileName;

    ifstream DataFile;

    std::map<unsigned long, vector<int>> *PedestalValues;
    std::map<unsigned long, vector<Waveform<double>>> *LAPPDWaveforms;
    vector<unsigned short> metaData;
    vector<string> metaDataString;

    int DoPedSubtract;

    string OutputWavLabel;

    int eventNo;

    int NChannels;
    int Nsamples;
    int TrigChannel;
    double SampleSize;
    int LAPPDchannelOffset;

    int oldLaser;
};

#endif
