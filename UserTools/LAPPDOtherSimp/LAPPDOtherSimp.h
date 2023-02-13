#ifndef LAPPDOtherSimp_H
#define LAPPDOtherSimp_H

#include <string>
#include <iostream>
#include "TString.h"
#include "Tool.h"
#include "TString.h"
#include "TTree.h"
#include "Geometry.h"
#include "TH2D.h"
#include "TFile.h"

/**
 * \class LAPPDOtherSimp
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDOtherSimp: public Tool {


 public:

  LAPPDOtherSimp(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  bool FindMaxInRange(vector<vector<double>> &Vec0, vector<vector<double>> &Vec1, int range1, int range2, int rangeStrip, vector<double> &rangeAmp0, vector<double> &rangeAmp1, vector<double> &rangeTime0, vector<double> &rangeTime1 );
  bool ConvertMapToVec( map<unsigned long, vector<Waveform<double>>> &WaveData, vector<vector<double>> &Vec0, vector<vector<double>> &Vec1, Geometry *geom);
  bool BaselineNoise2DHisto(map<unsigned long, vector<Waveform<double>>> &WaveData, TH2D &BaselineHisto, Geometry *geom);
  bool CombineBsNoHisto(TH2D &BaselineHisto, TH2D &BSHistoCombine);
  bool BaselineTreeFill(map<unsigned long, vector<Waveform<double>>> &WaveData, TTree &OSTree, Geometry *geom,int &stripN,double &sAmp,double &sTime);
  bool GetGMax(vector<vector<double>> &Vec0, double &maxv);

 private:

   Geometry* geom;
   TFile* OtherSimpFile;
   TTree* OSTree;
   TString TFname;
   int nnlsOption;
   int range1;
   int range2;
   int rangeStrip1;
   int rangeStrip2;
   int rangeStrip3;
   int rangeStrip4;
   int rangeStrip5;
   string InputWavLabel;
   string OutputWavLabel;
   TH2D* BSHistoCombine;
   int stripN;
   double sAmp;
   double sTime;
   int baselineOption;



};


#endif
