#ifndef LAPPDBaseline_H
#define LAPPDBaseline_H


#include <string>
#include <iostream>

#include "TH1.h"
#include "TF1.h"
#include "Tool.h"

class LAPPDBaseline: public Tool {


    public:

        LAPPDBaseline();
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

        double execludeAmpDifference;

};


#endif
