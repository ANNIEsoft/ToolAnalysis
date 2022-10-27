#ifndef LAPPDPlotWaveForms2D_H
#define LAPPDPlotWaveForms2D_H

#include <string>
#include <iostream>
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"

#include "TString.h"

#include "TTree.h"


#include "Tool.h"


/**
 * \class LAPPDPlotWaveForms2D
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDPlotWaveForms2D: public Tool {


 public:

  LAPPDPlotWaveForms2D(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

    TFile* mtf;
    int miter;

    string InputWavLabel;

    //TTree* outtree;
    int NChannel;
    int TrigChannel;
    int NHistos;
    double Deltat;
    //TH1D** hAmp;
    //TH1D** hQ;
    //TH1D** hTime;
    bool isFiltered;
    bool isIntegrated;
    bool isSim;
    bool isBLsub;
    bool SaveByChannel;
    bool SaveSingleStrip;
    bool requireT0signal;
    int psno;
    int trigno;
    TH1D* PHD;
    Geometry* _geom;



};


#endif
