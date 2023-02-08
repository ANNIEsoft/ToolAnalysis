#ifndef LAPPDLocateHit_H
#define LAPPDLocateHit_H

#include <string>
#include <iostream>

#include "TString.h"
#include "Tool.h"
#include "TFile.h"
#include "Geometry.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMarker.h"
#include "TAttMarker.h"
#include "TStyle.h"
#include "TColor.h"
#include "TText.h"
#include "TH3D.h"
#include "TTree.h"


/**
 * \class LAPPDLocateHit
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDLocateHit: public Tool {


 public:

  LAPPDLocateHit(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool FindlocalPeak();
  bool FindlocalPeaks(Waveform<double> &wav,Waveform<double> &pwav, vector<double> &peakHeight, vector<int> &peakTimeLoc, vector<double> &peakArea ,vector<double> &pulseArea, vector<double> &pulseHeight, int strip, double lowT, double highT,int samplingFactor);
  double MatchingProduct(double a, double b, double sigma);

 private:
   //TH3D *positionHist3D;
   TH2D *positionHistOff;
  TH2D *positionHistIn;
   TH2D *timeEventsTotal;
    TH2D *AmpVSnAreaS;
    TH2D *AmpVSnAreaD;
    TH2D *AmpVSpAreaS;
    TH2D *AmpVSpAreaD;
    TH2D *nAmpVSpAmp;
    TH2D *nAmpVSpAreaS;
    TH2D *nAmpVSpASoT;

    TH3D *BeamTvsPIn;
    TH3D *BeamTvsPOut;

    TTree* PositionTree;
    TFile *positionTreeFile;
      TFile *positionFile;
      int hBN;
      int beamPlotOption;
   string LocateHitLabel;
  string OutputWavLabel;
  string InputWavLabel;
  string RawInputWavLabel;
  string DisplayPDFSuffix;
       Geometry* geom;
       int miter;
       double peakLowThreshold;
       double peakHighThreshold;
       int EventItr;
       int LAPPDLocateHitVerbosity;
       double sigma;
        double StripLength;
        double signalSpeed;
        double nnlsTimeStep;
        double Timebinsize2D;
        double BeamTime;
        int AbnormalEvents;
        int SingleEventPlot;
        int plotOption;
        int colorNumber;
        double sizeNumber;
        double colorScaleN;
        double colorNumberstart;
        double colorNumberend;

        int plotStripVert;

        double testRatio1;
        double testRatio2;

};


#endif
