#ifndef LAPPDBaselineSubtract_H
#define LAPPDBaselineSubtract_H

#include <string>
#include <iostream>

#include "TH1.h"
#include "TF1.h"
#include "Tool.h"

class LAPPDBaselineSubtract: public Tool {


    public:

        LAPPDBaselineSubtract();
        bool Initialise(std::string configfile,DataModel &data);
        bool Execute();
        bool Finalise();


    private:

        Waveform<double> SubtractSine(Waveform<double> iwav);
        int LAPPDchannelOffset;
        int BLSVerbosityLevel;
        bool isSim;
        int DimSize;
        int TrigChannel;
        double Deltat;
        double LowBLfitrange;
        double HiBLfitrange;
        double TrigLowBLfitrange;
        double TrigHiBLfitrange;
        string BLSInputWavLabel;
        string BLSOutputWavLabel;

};


#endif
