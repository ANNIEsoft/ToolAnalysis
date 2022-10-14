#ifndef LAPPDFindT0_H
#define LAPPDFindT0_H

#include <string>
#include <iostream>

#include "Tool.h"


/**
 * \class LAPPDFindT0
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDFindT0: public Tool {


    public:

        LAPPDFindT0(); ///< Simple constructor
        bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
        bool Execute(); ///< Execute function used to perform Tool purpose.
        bool Finalise(); ///< Finalise function used to clean up resources.
        double Tfit(std::vector<double>* wf);

    private:

        //vector for save
        vector<bool> vec_T0signalInWindow;
        vector<double> vec_deltaT;
        vector<int> vec_T0Bin;

        int LAPPDchannelOffset;
        int TrigChannel;
        int T0offset;
        double T0signalmax,T0signalthreshold;
        double T0signalmaxOld,T0signalthresholdOld;
        int T0channelNo;
        int FindT0VerbosityLevel;
        int globalshiftT0;
        string InputWavLabel;
        string OutputWavLabel;
        int trigearlycut,triglatecut;
        int oldLaser;
};


#endif
